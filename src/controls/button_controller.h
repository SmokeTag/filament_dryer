#ifndef BUTTON_CONTROLLER_H
#define BUTTON_CONTROLLER_H

#include "display_interface.h"
#include <stdint.h>
#include <stdbool.h>

// Configurações do botão
#define BUTTON_PIN 16                      // GPIO para botão de ajuste temperatura
#define BUTTON_DEBOUNCE_MS 35              // Tempo de debounce (ms)
#define BUTTON_HOLD_THRESHOLD_MS 650       // Tempo para ativar modo rápido (ms)
#define BUTTON_FAST_REPEAT_MS 470          // Intervalo do incremento rápido (ms)

#define TEMP_MIN 40                        // Temperatura mínima (°C)
#define TEMP_MAX 85                        // Temperatura máxima (°C)
#define TEMP_STEP_SINGLE 1                 // Incremento simples (°C)
#define TEMP_STEP_FAST 5                   // Incremento rápido quando segurando (°C)

// Funções públicas do módulo
void button_controller_init(void);
bool button_controller_update(dryer_data_t *data);

#endif // BUTTON_CONTROLLER_H