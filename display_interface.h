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
    float energy_total;     // Consumo de energia total (Wh)
    float energy_current;   // Consumo atual (W)
    bool heater_on;         // Status do aquecedor
    bool sensor_safe;       // Status crítico do sensor DHT22
    uint32_t uptime;        // Tempo ligado (segundos)
    float pwm_percent;      // Potência PWM aplicada (0-100%)
} dryer_data_t;

// Funções públicas do módulo de interface
void draw_static_interface(void);
void update_interface_smart(dryer_data_t *data, dryer_data_t *prev_data);
void display_init_screen(void);
void display_test_characters(void);

// Funções auxiliares específicas (podem ser privadas se necessário)
void update_temperature_display(float temperature, float target, float prev_temp, float prev_target);
void update_humidity_display(float humidity, float prev_humidity);
void update_energy_display(float current, float total, float prev_current, float prev_total);
void update_status_display(bool heater_on, float pwm_percent, 
                          bool prev_heater, float prev_pwm);
void update_uptime_display(uint32_t uptime, uint32_t prev_uptime);

#endif // DISPLAY_INTERFACE_H