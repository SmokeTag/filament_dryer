#ifndef DISPLAY_INTERFACE_H
#define DISPLAY_INTERFACE_H

#include "st7789_display.h"
#include <stdint.h>
#include <stdbool.h>

// Estrutura para dados da estufa (compartilhada)
typedef struct {
    float temperature;      // Temperatura atual (°C)
    float humidity;         // Umidade atual (%)
    float temp_target;      // Temperatura alvo (°C)
    float energy_24h;       // Consumo de energia 24h (Wh)
    float energy_current;   // Consumo atual (W)
    bool heater_on;         // Status do aquecedor
    bool fan_on;            // Status da ventoinha
    uint32_t uptime;        // Tempo ligado (segundos)
} dryer_data_t;

// Funções públicas do módulo de interface
void draw_static_interface(void);
void update_interface_smart(dryer_data_t *data, dryer_data_t *prev_data);
void display_init_screen(void);
void display_test_characters(void);

// Funções auxiliares específicas (podem ser privadas se necessário)
void update_temperature_display(float temperature, float target, float prev_temp, float prev_target);
void update_humidity_display(float humidity, float prev_humidity);
void update_energy_display(float current, float total_24h, float prev_current, float prev_total);
void update_status_display(bool heater_on, bool fan_on, bool prev_heater, bool prev_fan);
void update_uptime_display(uint32_t uptime, uint32_t prev_uptime);

#endif // DISPLAY_INTERFACE_H