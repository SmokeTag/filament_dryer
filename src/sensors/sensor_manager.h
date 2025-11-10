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
    bool sensor_failure_event;  // TRUE se houve uma falha de leitura neste ciclo
    bool unsafe_event;          // TRUE se o sistema entrou em modo unsafe neste ciclo
    float energy_current;       // Consumo de energia atual (W)
    char dht_status[64]; // Descrição da última falha do sensor
} sensor_data_t;

// Funções públicas do módulo
void sensor_manager_init(void);
void sensor_manager_update(sensor_data_t *sensor_data);

#endif // SENSOR_MANAGER_H