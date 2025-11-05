#include "sensor_manager.h"
#include "dht22.h"
#include "hardware/adc.h"
#include "pico/time.h"
#include <stdio.h>
#include <stdlib.h>

// Variáveis privadas do módulo DHT22
static uint32_t last_dht22_read = 0;
static float last_temperature = 25.0;
static float last_humidity = 50.0;
static bool dht22_initialized = false;
static uint32_t dht22_error_count = 0;
static uint32_t dht22_success_count = 0;
static bool dht22_sensor_ok = false;

// Inicialização do módulo de sensores
void sensor_manager_init(void) {
    // Inicializar ADC para sensor de energia
    adc_init();
    adc_gpio_init(ENERGY_SENSOR_PIN);
    
    // Reset das variáveis DHT22
    last_dht22_read = 0;
    last_temperature = 25.0;
    last_humidity = 50.0;
    dht22_initialized = false;
    dht22_error_count = 0;
    dht22_success_count = 0;
    dht22_sensor_ok = false;
    
    printf("Sensor Manager: Inicializado (DHT22: GPIO%d, Energy: GPIO%d)\n", 
           DHT22_PIN, ENERGY_SENSOR_PIN);
}

// Leitura do DHT22
static void read_dht22_sensor(float *temperature, float *humidity) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // Inicializar DHT22 na primeira chamada
    if (!dht22_initialized) {
        dht22_init(DHT22_PIN);
        dht22_initialized = true;
        printf("*** Sensor Manager: DHT22 inicializado no GPIO %d ***\n", DHT22_PIN);
        last_dht22_read = current_time;
        dht22_sensor_ok = true;
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
            dht22_success_count++;
            dht22_error_count = 0; // Reset contador de erros
            dht22_sensor_ok = true;
            
        } else {
            // ❌ ERRO CRÍTICO - Incrementar contador
            dht22_error_count++;
            printf("Sensor Manager: ❌ DHT22 ERRO CRÍTICO #%lu: %s\n", 
                   dht22_error_count, dht22_error_string(result));
            
            // PARADA DE SEGURANÇA se muitos erros consecutivos
            if (dht22_error_count >= DHT22_MAX_CONSECUTIVE_ERRORS) {
                dht22_sensor_ok = false;
                printf("Sensor Manager: 🚨 ALERTA DE SEGURANÇA: DHT22 FALHOU! 🚨\n");
                printf("Sensor Manager: 🔥 AQUECEDOR DESABILITADO POR SEGURANÇA\n");
                printf("Sensor Manager: 🔧 VERIFIQUE CONEXÕES DO SENSOR\n");
                printf("Sensor Manager: 📊 Erros consecutivos: %lu\n", dht22_error_count);
            }
        }
    }
    
    // IMPORTANTE: Retornar últimos valores mesmo com erro
    *temperature = last_temperature;
    *humidity = last_humidity;
}

// Atualizar todos os sensores
void sensor_manager_update(sensor_data_t *sensor_data) {
    read_dht22_sensor(&sensor_data->temperature, &sensor_data->humidity);
    sensor_data->sensor_safe = dht22_sensor_ok;
    sensor_data->error_count = dht22_error_count;
    sensor_data->success_count = dht22_success_count;
}

// Verificar status de segurança do sensor
bool sensor_manager_is_safe(void) {
    return dht22_sensor_ok;
}

// Simulação do sensor de energia (substitua pela implementação real)
float sensor_manager_read_energy(void) {
    // TODO: Implementar leitura real do sensor de energia
    // Por enquanto, simula consumo de 0 a 100W
    return ((float)(rand() % 1000)) / 10.0; // 0-100W
}