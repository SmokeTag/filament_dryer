#include "hardware_control.h"
#include "logger.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/pwm.h"
#include <stdio.h>

#define TAG "HwCtrl"

// Configuração do PWM
#define PWM_FREQUENCY_HZ 5000    // 5 kHz - ideal para hotend
#define PWM_WRAP_VALUE 12500     // Clock padrão (125MHz) / (2.0 * 5000Hz) = 12500
#define PWM_ACTIVE_THRESHOLD 5.0f // PWM > 5% considera heater ativo

// Pico W devices use a GPIO on the WIFI chip for the LED
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#define PICO_OK 0

// Variáveis privadas do módulo
static uint32_t last_led_update = 0;
static bool led_state = false;
static uint pwm_slice_num = 0;
static uint pwm_channel = 0;


// Inicialização do módulo de controle de hardware
void hardware_control_init(void) {
    // Configurar GPIO para função PWM
    gpio_set_function(HEATER_PIN, GPIO_FUNC_PWM);
    
    // Descobrir qual slice PWM está conectado ao HEATER_PIN
    pwm_slice_num = pwm_gpio_to_slice_num(HEATER_PIN);
    pwm_channel = pwm_gpio_to_channel(HEATER_PIN);
    
    // Configurar PWM
    pwm_config config = pwm_get_default_config();
    
    // Configurar divisor de clock para obter a frequência desejada
    // Clock base = 125MHz, queremos 5kHz
    // div = 125MHz / (wrap * freq) = 125MHz / (12500 * 5kHz) = 2.0
    pwm_config_set_clkdiv(&config, 2.0f);
    pwm_config_set_wrap(&config, PWM_WRAP_VALUE);
    
    // Aplicar configuração
    pwm_init(pwm_slice_num, &config, true);
    
    // Estado inicial seguro (0% duty cycle)
    pwm_set_chan_level(pwm_slice_num, pwm_channel, 0);
    
    // Reset variáveis do LED
    last_led_update = 0;
    led_state = false;
    
    LOGI(TAG, "Initialized (Heater PWM: GPIO %d, Slice %d, Channel %d, Freq: %d Hz)", 
         HEATER_PIN, pwm_slice_num, pwm_channel, PWM_FREQUENCY_HZ);
}

// Controle do aquecedor com PWM
void hardware_control_heater_pwm(float duty_cycle_percent) {
    // Garantir que duty_cycle está no range válido (0-100%)
    if (duty_cycle_percent < 0.0f) {
        LOGW(TAG, "Duty cycle abaixo de 0%%, ajustando para 0%%");
        duty_cycle_percent = 0.0f;
    } else if (duty_cycle_percent > 100.0f) {
        LOGW(TAG, "Duty cycle acima de 100%%, ajustando para 100%%");
        duty_cycle_percent = 100.0f;
    }
    
    // Converter percentual para nível PWM
    uint16_t level = (uint16_t)((duty_cycle_percent / 100.0f) * PWM_WRAP_VALUE);
    
    // Aplicar ao PWM
    pwm_set_chan_level(pwm_slice_num, pwm_channel, level);
}

// Controle do aquecedor (compatibilidade - deprecated)
void hardware_control_heater(bool enable) {
    // Converte bool para PWM: true = 100%, false = 0%
    LOGW(TAG, "hardware_control_heater() is deprecated, use hardware_control_heater_pwm()");
    hardware_control_heater_pwm(enable ? 100.0f : 0.0f);
}



// Atualizar PWM do heater baseado no duty cycle calculado
void hardware_control_update_pwm(dryer_data_t *data, bool sensor_safe, float pid_output) {
    if (!sensor_safe) {
        // Modo segurança - PWM sempre 0%
        data->pwm_percent = 0.0;
    } else {
        // Usar saída do PID diretamente
        data->pwm_percent = pid_output;
    }
    
    // Aplicar o duty cycle ao hardware
    hardware_control_heater_pwm(data->pwm_percent);
}

// Verifica se o heater está ativo baseado no PWM atual
bool hardware_control_heater_is_active(float pwm_percent) {
    return pwm_percent > PWM_ACTIVE_THRESHOLD;
}

// Controle do LED de status com diferentes padrões
void hardware_control_led_status(bool sensor_safe, float pwm_percent) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // Verificar se heater está ativo
    bool heater_active = hardware_control_heater_is_active(pwm_percent);
    
    // Determinar intervalo baseado no estado
    int led_interval;
    if (!sensor_safe) {
        // DHT22 falhou - pisca muito rápido (ALERTA)
        led_interval = 100;
    } else if (heater_active) {
        // Aquecendo - pisca moderado
        led_interval = 250;
    } else {
        // Normal - pisca lento
        led_interval = 1000;
    }
    
    if (current_time - last_led_update >= led_interval) {
        last_led_update = current_time;
        led_state = !led_state;
        hardware_control_set_led(led_state);
    }
}

// Inicialização do LED onboard
int hardware_control_led_init(void) {
#if defined(PICO_DEFAULT_LED_PIN)
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    LOGI(TAG, "LED initialized (Default GPIO)");
    return PICO_OK;
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // For Pico W devices we need to initialise the driver etc
    LOGI(TAG, "LED initialized (Pico W CYW43)");
    return cyw43_arch_init();
#else
    LOGW(TAG, "No LED defined for this board");
    return PICO_OK;
#endif
}

// Controle do LED onboard
void hardware_control_set_led(bool led_on) {
#if defined(PICO_DEFAULT_LED_PIN)
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // Ask the wifi "driver" to set the GPIO on or off
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#endif
}