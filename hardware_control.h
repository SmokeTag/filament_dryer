#ifndef HARDWARE_CONTROL_H
#define HARDWARE_CONTROL_H

#include "display_interface.h"
#include <stdint.h>
#include <stdbool.h>

// Pinos de controle de hardware
#define HEATER_PIN 15           // GPIO para controle do hotend
#define FAN_PIN 14              // GPIO para controle da ventoinha

// Funções públicas do módulo
void hardware_control_init(void);
void hardware_control_heater(bool enable);
void hardware_control_fan(bool enable);
void hardware_control_update_pwm(dryer_data_t *data, bool sensor_safe);
void hardware_control_led_status(bool sensor_safe, bool heater_on);

// Funções do LED onboard
int hardware_control_led_init(void);
void hardware_control_set_led(bool led_on);

#endif // HARDWARE_CONTROL_H