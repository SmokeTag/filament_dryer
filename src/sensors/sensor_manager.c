#include "sensor_manager.h"
#include "dht22.h"
#include "acs712.h"
#include "logger.h"
#include "hardware/adc.h"
#include "pico/time.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "SensorMgr"

// Variáveis privadas do módulo DHT22
static uint32_t last_dht22_read = 0;
static float last_temperature = 25.0;
static float last_humidity = 50.0;
static bool dht22_initialized = false;
static uint32_t dht22_error_count = 0;
static uint32_t acs712_error_count = 0;

// Inicialização do módulo de sensores
void sensor_manager_init(void) {
    // Inicializar ADC para sensor de energia
    adc_init();
    acs712_init(ENERGY_SENSOR_PIN);
    
    // Reset das variáveis DHT22
    last_dht22_read = 0;
    last_temperature = 25.0;
    last_humidity = 50.0;
    dht22_initialized = false;
    dht22_error_count = 0;
    acs712_error_count = 0;
    
    LOGI(TAG, "Initialized (DHT22: GPIO %d, ACS712: GPIO %d)", 
           DHT22_PIN, ENERGY_SENSOR_PIN);
}

// Leitura do DHT22
static void read_dht22_sensor(sensor_data_t *sensor_data) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // Inicializar eventos como false por padrão
    sensor_data->sensor_failure_event = false;
    sensor_data->unsafe_event = false;
    strcpy(sensor_data->dht_status, "Nenhum erro");
    
    // Inicializar DHT22 na primeira chamada
    if (!dht22_initialized) {
        dht22_init(DHT22_PIN);
        dht22_initialized = true;
        LOGI(TAG, "DHT22 initialized (GPIO %d)", DHT22_PIN);
        last_dht22_read = current_time;
        sensor_data->sensor_safe = true;
        LOGI(TAG, "DHT22 ready for readings");
    }
    
    // Verificar se já passou tempo suficiente desde a última leitura
    if (current_time - last_dht22_read >= DHT22_READ_INTERVAL_MS) {
        last_dht22_read = current_time;
        
        float new_temp, new_hum;
        dht22_result_t result = dht22_read(&new_temp, &new_hum);
        
        if (result == DHT22_OK) {
            // SENSOR OK - Sistema pode operar normalmente
            last_temperature = new_temp;
            last_humidity = new_hum;
            dht22_error_count = 0; // Reset contador de erros
            sensor_data->sensor_safe = true;
            
        } else {
            // ERRO CRÍTICO - Reportar evento de falha
            dht22_error_count++;
            sensor_data->sensor_failure_event = true;  // Reportar evento de falha
            
            // Armazenar mensagem da última falha
            snprintf(sensor_data->dht_status, sizeof(sensor_data->dht_status), 
                    "%s", dht22_error_string(result));
            
            LOGE(TAG, "DHT22 CRITICAL ERROR #%lu: %s", 
                   dht22_error_count, dht22_error_string(result));
            
            // PARADA DE SEGURANÇA se muitos erros consecutivos
            if (dht22_error_count >= DHT22_MAX_CONSECUTIVE_ERRORS) {
                sensor_data->sensor_safe = false;
                if (dht22_error_count == DHT22_MAX_CONSECUTIVE_ERRORS) {
                    // Apenas logar na primeira vez que atingir o limite
                    sensor_data->unsafe_event = true;  // Reportar evento unsafe
                    LOGE(TAG, "CRITICAL: DHT22 SENSOR FAILURE!");
                    LOGE(TAG, "Heater disabled for safety");
                    LOGE(TAG, "Check sensor connections");
                    LOGE(TAG, "Consecutive errors: %lu", dht22_error_count);
                }
            }
        }
    }
    
    // IMPORTANTE: Preencher estrutura com últimos valores mesmo com erro
    sensor_data->temperature = last_temperature;
    sensor_data->humidity = last_humidity;
    sensor_data->error_count = dht22_error_count;
}

// Leitura do sensor de energia (ACS712)
static float sensor_manager_read_energy(void) {
    // Retorna potência em Watts (P = V * I)
    // Hotend em 12V de alimentação
    acs712_status_t status;
    float current = acs712_read_current(&status);
    
    if (status.code == ACS712_DISCONNECTED) {
        // Sensor desconectado - retornar 0W silenciosamente
        LOGW(TAG, "ACS712: sensor disconnected (GPIO %d)", status.gpio_pin);
        return 0.0f;
    }
    
    if (status.code == ACS712_HIGH_VOLTAGE_WARNING) {
        LOGW(TAG, "High voltage detected on GPIO %d (%.2fV > 2.6V). Reverse ACS712 polarity for Pico safety!", 
             status.gpio_pin, status.voltage);
    }

    return current * 12.0f;
}

// Detecção de falha do hotend
static void check_heater_failure(sensor_data_t *sensor_data, bool heater_on) {
    // Inicializar como sem falha
    sensor_data->heater_failure = false;
    
    // Se aquecedor está ligado, verificar se está consumindo energia
    if (heater_on) {
        if (sensor_data->energy_current < ACS712_MIN_ENERGY_THRESHOLD) {
            // Aquecedor ligado mas SEM corrente - possível falha
            acs712_error_count++;
            
            LOGW(TAG, "ACS712: Heater ON but no current detected (%.2fW < %.2fW threshold) - Error #%lu/%lu", 
                sensor_data->energy_current, ACS712_MIN_ENERGY_THRESHOLD, 
                acs712_error_count, (uint32_t)ACS712_MAX_CONSECUTIVE_ERRORS);
            
            if (acs712_error_count >= ACS712_MAX_CONSECUTIVE_ERRORS) {
                sensor_data->heater_failure = true;
                sensor_data->sensor_safe = false; // Trigger modo unsafe
                
                if (acs712_error_count == ACS712_MAX_CONSECUTIVE_ERRORS) {
                    sensor_data->unsafe_event = true;
                    LOGE(TAG, "CRITICAL: HEATING SYSTEM FAILURE!");
                    LOGE(TAG, "Possible failure: HOTEND or MOSFET IRLZ44N");
                    LOGE(TAG, "Check components and connections");
                }
            }
        } else {
            // Corrente detectada - reset contador
            acs712_error_count = 0;
        }
    } else {
        // Aquecedor desligado - reset contador
        acs712_error_count = 0;
    }
    
    sensor_data->heater_error_count = acs712_error_count;
}

// Atualizar todos os sensores
void sensor_manager_update(sensor_data_t *sensor_data, bool heater_on) {
    read_dht22_sensor(sensor_data);
    
    // Ler sensor de energia e incluir na mesma estrutura
    sensor_data->energy_current = sensor_manager_read_energy();
    
    // Verificar falha do sistema de aquecimento
    check_heater_failure(sensor_data, heater_on);
}