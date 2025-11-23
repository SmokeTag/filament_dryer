#include "hardware_control.h"
#include "logger.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>

#define TAG "HwCtrl"

// Pico W devices use a GPIO on the WIFI chip for the LED
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#define PICO_OK 0

// Variáveis privadas do módulo
static uint32_t last_led_update = 0;
static bool led_state = false;

// Inicialização do módulo de controle de hardware
void hardware_control_init(void) {
    // Inicializar GPIO de controle
    gpio_init(HEATER_PIN);
    gpio_set_dir(HEATER_PIN, GPIO_OUT);
    
    // Estado inicial seguro
    hardware_control_heater(false);
    
    // Reset variáveis do LED
    last_led_update = 0;
    led_state = false;
    
    LOGI(TAG, "Initialized (Heater: GPIO %d)", HEATER_PIN);
}

// Controle do aquecedor
void hardware_control_heater(bool enable) {
    gpio_put(HEATER_PIN, enable);
}



// Calcular PWM atual baseado no estado do heater
void hardware_control_update_pwm(dryer_data_t *data, bool sensor_safe) {
    if (!sensor_safe) {
        // Modo segurança - PWM sempre 0%
        data->pwm_percent = 0.0;
    } else if (data->heater_on) {
        // Histerese: 100% quando ligado
        data->pwm_percent = 100.0;
        
        // TODO: Quando implementar PID, usar valor real do PID:
        // data->pwm_percent = pid_output_percent;
    } else {
        // Desligado
        data->pwm_percent = 0.0;
    }
}

// Controle do LED de status com diferentes padrões
void hardware_control_led_status(bool sensor_safe, bool heater_on) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // Determinar intervalo baseado no estado
    int led_interval;
    if (!sensor_safe) {
        // DHT22 falhou - pisca muito rápido (ALERTA)
        led_interval = 100;
    } else if (heater_on) {
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