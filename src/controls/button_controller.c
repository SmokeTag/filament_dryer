#include "button_controller.h"
#include "logger.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>

#define TAG "BtnCtrl"

// Variáveis privadas do módulo
static bool button_state = false;
static bool button_prev_state = false;
static uint32_t button_last_change = 0;
static uint32_t button_press_start = 0;
static bool button_debounced = false;
static bool button_was_pressed = false;
static bool button_in_fast_mode = false;
static uint32_t button_last_fast_increment = 0;

// Inicialização do módulo de controle de botão
void button_controller_init(void) {
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);  // Pull-up interno (botão conecta ao GND)
    
    // Reset de todas as variáveis
    button_state = false;
    button_prev_state = false;
    button_last_change = 0;
    button_press_start = 0;
    button_debounced = false;
    button_was_pressed = false;
    button_in_fast_mode = false;
    button_last_fast_increment = 0;
    
    LOGI(TAG, "Initialized (Button: GPIO %d)", BUTTON_PIN);
}

// Debouncing digital do botão
static bool button_read_debounced(void) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    bool raw_state = !gpio_get(BUTTON_PIN);  // Inverte porque usa pull-up
    
    // Detectar mudança de estado
    if (raw_state != button_prev_state) {
        button_last_change = current_time;
        button_prev_state = raw_state;
    }
    
    // Aplicar debouncing
    if (current_time - button_last_change >= BUTTON_DEBOUNCE_MS) {
        if (button_debounced != raw_state) {
            button_debounced = raw_state;
            
            // Detectar início do pressionamento
            if (button_debounced && !button_state) {
                button_press_start = current_time;
            }
            
            button_state = button_debounced;
            return true;  // Mudança de estado detectada
        }
    }
    
    return false;  // Sem mudança
}

// Atualizar controle de botão e ajustar temperatura
bool button_controller_update(dryer_data_t *data) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    bool state_changed = button_read_debounced();
    bool temp_changed = false;
    
    // Detectar início do pressionamento
    if (state_changed && button_state) {
        // Botão foi pressionado
        button_was_pressed = true;
        button_in_fast_mode = false;
        button_press_start = current_time;
        
        LOGD(TAG, "Button pressed - waiting for release...");
        return false;  // Sem mudança de temperatura ainda
    }
    
    // Detectar fim do pressionamento
    if (state_changed && !button_state) {
        // Botão foi solto - calcular ação baseada no tempo
        uint32_t press_duration = current_time - button_press_start;
        bool was_in_fast_mode = button_in_fast_mode;  // Salvar estado antes de resetar
        button_was_pressed = false;
        button_in_fast_mode = false;
        
        if (press_duration < BUTTON_HOLD_THRESHOLD_MS) {
            // Pressão curta: +1°C
            data->temp_target += TEMP_STEP_SINGLE;
            if (data->temp_target > TEMP_MAX) {
                data->temp_target = TEMP_MIN;
            }
            LOGI(TAG, "Short press: +1°C -> %.0f°C", data->temp_target);
            temp_changed = true;
        } else {
            // Pressão longa: SEM incremento adicional se já estava no modo rápido
            if (was_in_fast_mode) {
                LOGD(TAG, "Fast mode ended - %.0f°C", data->temp_target);
                // temp_changed fica false - não houve mudança ao soltar
            } else {
                // Pressão longa mas não chegou a ativar modo rápido - dar +5°C
                data->temp_target += TEMP_STEP_FAST;
                if (data->temp_target > TEMP_MAX) {
                    data->temp_target = TEMP_MIN;
                }
                LOGI(TAG, "Long press: +5°C -> %.0f°C", data->temp_target);
                temp_changed = true;
            }
        }
    }
    
    // Botão ainda pressionado - verificar se ativa modo rápido
    if (button_state && button_was_pressed) {
        uint32_t press_duration = current_time - button_press_start;
        
        // Ativar modo rápido após BUTTON_HOLD_THRESHOLD_MS
        if (press_duration >= BUTTON_HOLD_THRESHOLD_MS) {
            if (!button_in_fast_mode) {
                button_in_fast_mode = true;
                button_last_fast_increment = current_time;
                LOGI(TAG, "Fast mode activated!");
                
                // Primeiro incremento rápido imediato ao ativar modo
                data->temp_target += TEMP_STEP_FAST;
                if (data->temp_target > TEMP_MAX) {
                    data->temp_target = TEMP_MIN;
                }
                LOGI(TAG, "Fast mode: +5°C -> %.0f°C", data->temp_target);
                temp_changed = true;
            }
            
            // Incrementos rápidos adicionais a cada BUTTON_FAST_REPEAT_MS
            if (current_time - button_last_fast_increment >= BUTTON_FAST_REPEAT_MS) {
                button_last_fast_increment = current_time;
                
                data->temp_target += TEMP_STEP_FAST;
                if (data->temp_target > TEMP_MAX) {
                    data->temp_target = TEMP_MIN;
                }
                
                LOGD(TAG, "Fast mode: +5°C -> %.0f°C", data->temp_target);
                temp_changed = true;
            }
        }
    }
    
    return temp_changed;  // Retorna se houve mudança de temperatura
}