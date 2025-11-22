#include "sensor_manager.h"
#include "dht22.h"
#include "acs712.h"
#include "hardware/adc.h"
#include "pico/time.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Variáveis privadas do módulo DHT22
static uint32_t last_dht22_read = 0;
static float last_temperature = 25.0;
static float last_humidity = 50.0;
static bool dht22_initialized = false;
static uint32_t dht22_error_count = 0;

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
    
    printf("Sensor Manager: Inicializado (DHT22: GPIO %d, Energy: GPIO %d)\n", 
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
        printf("*** Sensor Manager: DHT22 inicializado no GPIO %d ***\n", DHT22_PIN);
        last_dht22_read = current_time;
        sensor_data->sensor_safe = true;
        printf("Sensor Manager: DHT22 pronto para leituras!\n");
    }
    
    // Verificar se já passou tempo suficiente desde a última leitura
    if (current_time - last_dht22_read >= DHT22_READ_INTERVAL_MS) {
        last_dht22_read = current_time;
        
        float new_temp, new_hum;
        dht22_result_t result = dht22_read(&new_temp, &new_hum);
        
        if (result == DHT22_OK) {
            // ✅ SENSOR OK - Sistema pode operar normalmente
            last_temperature = new_temp;
            last_humidity = new_hum;
            dht22_error_count = 0; // Reset contador de erros
            sensor_data->sensor_safe = true;
            
        } else {
            // ❌ ERRO CRÍTICO - Reportar evento de falha
            dht22_error_count++;
            sensor_data->sensor_failure_event = true;  // Reportar evento de falha
            
            // Armazenar mensagem da última falha
            snprintf(sensor_data->dht_status, sizeof(sensor_data->dht_status), 
                    "%s", dht22_error_string(result));
            
            printf("Sensor Manager: ❌ DHT22 ERRO CRÍTICO #%lu: %s\n", 
                   dht22_error_count, dht22_error_string(result));
            
            // PARADA DE SEGURANÇA se muitos erros consecutivos
            if (dht22_error_count >= DHT22_MAX_CONSECUTIVE_ERRORS) {
                sensor_data->sensor_safe = false;
                if (dht22_error_count == DHT22_MAX_CONSECUTIVE_ERRORS) {
                    // Apenas logar na primeira vez que atingir o limite
                    sensor_data->unsafe_event = true;  // Reportar evento unsafe
                    printf("Sensor Manager: 🚨 ALERTA DE SEGURANÇA: DHT22 FALHOU! 🚨\n");
                    printf("Sensor Manager: 🔥 AQUECEDOR DESABILITADO POR SEGURANÇA\n");
                    printf("Sensor Manager: 🔧 VERIFIQUE CONEXÕES DO SENSOR\n");
                    printf("Sensor Manager: 📊 Erros consecutivos: %lu\n", dht22_error_count);
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
    // Hotend é 12V
    float current = acs712_read_current();
    return current * 12.0f;
}

// Atualizar todos os sensores
void sensor_manager_update(sensor_data_t *sensor_data) {
    read_dht22_sensor(sensor_data);
    
    // Ler sensor de energia e incluir na mesma estrutura
    sensor_data->energy_current = sensor_manager_read_energy();
}