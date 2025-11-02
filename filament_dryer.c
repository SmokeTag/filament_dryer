/**
 * Filament Dryer Controller
 * Sistema de estufa para secar filamentos de impressão 3D
 * 
 * Componentes:
 * - Display GMT020-02-7P TFT (240x320)
 * - Sensor DHT22 (temperatura e umidade)
 * - Sensor de consumo de energia
 * - Hotend + dissipador + ventoinha
 * 
 * Autor: Andre
 * Data: Novembro 2025
 */

#include "pico/stdlib.h"
#include "st7789_display.h"
#include "display_interface.h"
#include "hardware/adc.h"
#include "pico/time.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define PICO_OK 0

// Pico W devices use a GPIO on the WIFI chip for the LED,
// so when building for Pico W, CYW43_WL_GPIO_LED_PIN will be defined
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

// Configurações da estufa
#define UPDATE_INTERVAL_MS 5000  // Atualiza a cada 5 segundos
#define TEMP_TARGET_DEFAULT 45   // Temperatura alvo padrão (°C)
#define ENERGY_RESET_HOURS 24    // Reset do consumo a cada 24h

// Pinos dos sensores (ajuste conforme sua montagem)
#define DHT22_PIN 22            // GPIO para DHT22
#define ENERGY_SENSOR_PIN 26    // GPIO ADC para sensor de energia
#define HEATER_PIN 15           // GPIO para controle do hotend
#define FAN_PIN 14              // GPIO para controle da ventoinha

// A estrutura dryer_data_t agora está definida em display_interface.h

// Simulação do DHT22 (substitua pela biblioteca real do DHT22)
void read_dht22(float *temperature, float *humidity) {
    // TODO: Implementar leitura real do DHT22
    // Por enquanto, simulando valores realistas
    static float sim_temp = 25.0;
    static float sim_hum = 60.0;
    
    // Simula variação pequena nos valores
    sim_temp += ((float)(rand() % 21) - 10) / 10.0; // ±1°C
    sim_hum += ((float)(rand() % 11) - 5) / 10.0;   // ±0.5%
    
    // Limita valores realistas
    if (sim_temp < 20.0) sim_temp = 20.0;
    if (sim_temp > 80.0) sim_temp = 80.0;
    if (sim_hum < 10.0) sim_hum = 10.0;
    if (sim_hum > 90.0) sim_hum = 90.0;
    
    *temperature = sim_temp;
    *humidity = sim_hum;
}

// Simulação do sensor de energia (substitua pela implementação real)
float read_energy_sensor() {
    // TODO: Implementar leitura real do sensor de energia
    // Por enquanto, simula consumo de 0 a 100W
    return ((float)(rand() % 1000)) / 10.0; // 0-100W
}

// Controle do aquecedor
void control_heater(bool enable) {
    gpio_put(HEATER_PIN, enable);
}

// Controle da ventoinha
void control_fan(bool enable) {
    gpio_put(FAN_PIN, enable);
}

// Inicialização dos componentes
void dryer_init(void) {
    // Inicializar GPIOs
    gpio_init(HEATER_PIN);
    gpio_init(FAN_PIN);
    gpio_set_dir(HEATER_PIN, GPIO_OUT);
    gpio_set_dir(FAN_PIN, GPIO_OUT);
    
    // Inicializar ADC para sensor de energia
    adc_init();
    adc_gpio_init(ENERGY_SENSOR_PIN);
    
    // Estado inicial seguro
    control_heater(false);
    control_fan(false);
    
    printf("Sistema da estufa inicializado!\n");
}

// Perform initialisation
int pico_led_init(void) {
#if defined(PICO_DEFAULT_LED_PIN)
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // For Pico W devices we need to initialise the driver etc
    return cyw43_arch_init();
#endif
}

// Turn the led on or off
void pico_set_led(bool led_on) {
#if defined(PICO_DEFAULT_LED_PIN)
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // Ask the wifi "driver" to set the GPIO on or off
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#endif
}

// Funções de display movidas para display_interface.c

// Controle automático da temperatura
void temperature_control(dryer_data_t *data) {
    // Histerese de 2°C para evitar liga/desliga frequente
    if (data->temperature < (data->temp_target - 2.0)) {
        data->heater_on = true;
        data->fan_on = true;  // Ventilação forçada quando aquecendo
    } else if (data->temperature > (data->temp_target + 1.0)) {
        data->heater_on = false;
        data->fan_on = true;  // Continua ventilando para resfriar
    } else if (data->temperature > data->temp_target) {
        data->heater_on = false;
        // Ventoinha continua ligada se estava aquecendo
    }
    
    // Controla os hardware
    control_heater(data->heater_on);
    control_fan(data->fan_on);
}

int main() {
    // Initialize standard I/O for debugging
    stdio_init_all();
    sleep_ms(2000);  // Aguarda estabilizar USB
    
    printf("\n=== ESTUFA DE FILAMENTOS v1.0 ===\n");
    printf("Iniciando sistema...\n");
    
    // Initialize the onboard LED
    int rc = pico_led_init();
    if (rc != PICO_OK) {
        printf("Erro ao inicializar LED\n");
    }
    
    // Inicializar componentes da estufa
    dryer_init();
    
    // Initialize the TFT display
    printf("Inicializando display...\n");
    st7789_init();
    printf("Display inicializado!\n");
    
    // Inicializar dados da estufa
    dryer_data_t dryer_data = {
        .temperature = 25.0,
        .humidity = 50.0,
        .temp_target = TEMP_TARGET_DEFAULT,
        .energy_24h = 0.0,
        .energy_current = 0.0,
        .heater_on = false,
        .fan_on = false,
        .uptime = 0
    };
    
    printf("Sistema iniciado! Temperatura alvo: %.0f°C\n", dryer_data.temp_target);
    
    // Tela de inicialização com teste de caracteres
    display_test_characters();
    sleep_ms(5000);  // Mostra por 5 segundos
    
    // Tela de inicialização normal
    display_init_screen();
    sleep_ms(3000);
    
    // Desenhar interface estática uma única vez
    draw_static_interface();
    
    // Variáveis para controlar atualizações
    uint32_t last_update = 0;
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    
    // Estrutura para guardar valores anteriores
    dryer_data_t prev_data = dryer_data;
    
    // Primeira atualização completa (forçar diferenças para desenhar tudo)
    prev_data.temperature = -999;  // Força atualização
    prev_data.temp_target = -999;  // Força atualização
    prev_data.humidity = -999;     // Força atualização
    prev_data.energy_current = -999; // Força atualização
    prev_data.energy_24h = -999;   // Força atualização
    prev_data.heater_on = !dryer_data.heater_on; // Força atualização
    prev_data.fan_on = !dryer_data.fan_on;       // Força atualização
    prev_data.uptime = -1;         // Força atualização
    
    update_interface_smart(&dryer_data, &prev_data);
    
    while (true) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        // Atualiza dados a cada UPDATE_INTERVAL_MS
        if (current_time - last_update >= UPDATE_INTERVAL_MS) {
            last_update = current_time;
            
            // Salvar valores anteriores
            prev_data = dryer_data;
            
            // Atualizar tempo de funcionamento
            dryer_data.uptime = (current_time - start_time) / 1000;
            
            // Ler sensores
            read_dht22(&dryer_data.temperature, &dryer_data.humidity);
            dryer_data.energy_current = read_energy_sensor();
            
            // Acumular energia (aproximação simples)
            dryer_data.energy_24h += (dryer_data.energy_current * UPDATE_INTERVAL_MS) / 3600000.0; // Wh
            
            // Reset do consumo diário (simplificado - a cada 24h de uptime)
            if (dryer_data.uptime > 0 && (dryer_data.uptime % (24 * 3600)) == 0) {
                dryer_data.energy_24h = 0.0;
                printf("Reset do consumo diário\n");
            }
            
            // Controle automático de temperatura
            temperature_control(&dryer_data);
            
            // Atualizar display de forma inteligente (apenas o que mudou)
            update_interface_smart(&dryer_data, &prev_data);
            
            // Log no serial
            printf("T:%.1f°C H:%.1f%% E:%.1fW Target:%.0f°C Heater:%s Fan:%s\n",
                   dryer_data.temperature, dryer_data.humidity, dryer_data.energy_current,
                   dryer_data.temp_target,
                   dryer_data.heater_on ? "ON" : "OFF",
                   dryer_data.fan_on ? "ON" : "OFF");
        }
        
        // LED de status (pisca mais rápido quando aquecendo)
        int led_interval = dryer_data.heater_on ? 250 : 1000;
        static uint32_t last_led = 0;
        static bool led_state = false;
        
        if (current_time - last_led >= led_interval) {
            last_led = current_time;
            led_state = !led_state;
            pico_set_led(led_state);
        }
        
        sleep_ms(100);  // Loop principal a cada 100ms
    }
}
