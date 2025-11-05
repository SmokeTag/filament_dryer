#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

// Configurações dos sensores
#define DHT22_PIN 22                       // GPIO para DHT22
#define ENERGY_SENSOR_PIN 26               // GPIO ADC para sensor de energia
#define DHT22_READ_INTERVAL_MS 2000        // DHT22 precisa de pelo menos 2s entre leituras
#define DHT22_MAX_CONSECUTIVE_ERRORS 2     // Máximo de erros consecutivos antes de PARADA DE SEGURANÇA

// Estrutura de dados dos sensores
typedef struct {
    float temperature;
    float humidity;
    bool sensor_safe;
    uint32_t error_count;
    uint32_t success_count;
} sensor_data_t;

// Funções públicas do módulo
void sensor_manager_init(void);
void sensor_manager_update(sensor_data_t *sensor_data);
bool sensor_manager_is_safe(void);
float sensor_manager_read_energy(void);

#endif // SENSOR_MANAGER_H