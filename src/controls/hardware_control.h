#ifndef HARDWARE_CONTROL_H
#define HARDWARE_CONTROL_H

#include "display_interface.h"
#include <stdint.h>
#include <stdbool.h>

// Pinos de controle de hardware
#define HEATER_PIN 27           // GPIO para controle do hotend

// Funções públicas do módulo
void hardware_control_init(void);
void hardware_control_heater(bool enable);  // Deprecated - usar hardware_control_heater_pwm
void hardware_control_heater_pwm(float duty_cycle_percent);
void hardware_control_update_pwm(dryer_data_t *data, bool sensor_safe, float pid_output);
bool hardware_control_heater_is_active(float pwm_percent);
void hardware_control_led_status(bool sensor_safe, float pwm_percent);

// Funções do LED onboard
int hardware_control_led_init(void);
void hardware_control_set_led(bool led_on);

#endif // HARDWARE_CONTROL_H