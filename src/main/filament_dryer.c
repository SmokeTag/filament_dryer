// TODO: Think about heater on logic (possible good idea to define if heater >= 75% to make sure transistor is fully on)
// TODO: Think about adding a LED indicator
// TODO: Implementar controle de temperatura baseado em PID
// TODO: Adicionar visualização para falha do hotend na interface
// TODO: HARDWARE: Adicionar switch com saida do sensor ACS712 no pin 1, pin 2 no GPIO26 e pin 3 no GND
// SOFTWARE: entender que se GPIO26 ler 0V, o sensor está desligado. Não quero que isso seja tratado
// como erro crítico, o sistema pode funcionar sem o sensor de energia, removendo a detecção de falhas do hotend
// TODO: habilitar possibilidade de enviar estado OFF para display de consumo de energia quando o sensor estiver desconectado

/**
 * Filament Dryer Controller - Main Module
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
#include "button_controller.h"
#include "sensor_manager.h"
#include "hardware_control.h"
#include "temperature_control.h"
#include "logger.h"
#include "pico/time.h"
#include <stdio.h>
#include <string.h>

#define TAG "Main"

// Configurações principais do sistema
#define UPDATE_INTERVAL_MS 5000        // Atualiza a cada 5 segundos
#define TEMP_TARGET_DEFAULT 45         // Temperatura alvo padrão (°C)

// Inicialização de todos os módulos do sistema
void system_init(void) {
    // Módulos de hardware
    hardware_control_init();
    
    // Módulos de entrada
    button_controller_init();
    sensor_manager_init();
    
    // Display
    LOGI(TAG, "Initializing display...");
    st7789_init();
    LOGI(TAG, "Display initialized");
    
    LOGI(TAG, "Dryer system fully initialized");
}

// Função para processar dados dos sensores e atualizar dryer_data
static void process_sensor_data(sensor_data_t *sensor_data, dryer_data_t *dryer_data) {
    // Transferir dados básicos dos sensores
    dryer_data->temperature = sensor_data->temperature;
    dryer_data->humidity = sensor_data->humidity;
    dryer_data->sensor_safe = sensor_data->sensor_safe;
    dryer_data->energy_current = sensor_data->energy_current;
    dryer_data->heater_failure = sensor_data->heater_failure;
    dryer_data->total_sensor_failures += sensor_data->sensor_failure_event ? 1 : 0;
    dryer_data->total_unsafe_events += sensor_data->unsafe_event ? 1 : 0;
    strcpy(dryer_data->dht_status, sensor_data->dht_status);
}

int main() {
    // Variáveis para controlar atualizações
    uint32_t last_update = 0;
    uint32_t start_time = to_ms_since_boot(get_absolute_time());

    stdio_init_all();
    absolute_time_t deadline = make_timeout_time_ms(5000);
    while (!stdio_usb_connected() && !time_reached(deadline)) {
        sleep_ms(10); // Aguarda estabilizar USB
    }  
    
    printf("\n=== ESTUFA DE FILAMENTOS v2.0 ===\n");
    LOGI(TAG, "Starting system...");
    
    // Initialize the onboard LED
    int rc = hardware_control_led_init();
    if (rc != 0) {
        LOGE(TAG, "Failed to initialize LED");
    }
    
    // Inicializar todos os módulos do sistema
    system_init();
    
    // Inicializar dados da estufa
    dryer_data_t dryer_data = {
        .temperature = 10.0,
        .humidity = 50.0,
        .temp_target = TEMP_TARGET_DEFAULT,
        .energy_total = 0.0,
        .energy_current = 0.0,
        .heater_on = false,
        .sensor_safe = true,  // Assume sensor OK no início
        .uptime = 0,
        .pwm_percent = 0.0,
        .total_sensor_failures = 0,
        .total_unsafe_events = 0,
        .heater_failure = false,
        .dht_status = "Nenhum erro"
    };
    
    LOGI(TAG, "System started (Target: %.0f°C)", dryer_data.temp_target);
    
    // Tela de inicialização normal
    LOGI(TAG, "Starting initialization screen...");
    display_init_screen();
    LOGI(TAG, "Initialization screen completed, waiting 3s...");
    sleep_ms(3000);
    
    // Desenhar interface estática uma única vez
    LOGI(TAG, "Drawing static interface...");
    draw_static_interface();
    LOGI(TAG, "Static interface drawn");
    
    // Estrutura para guardar valores anteriores
    dryer_data_t prev_data = dryer_data;
    prev_data.temp_target = dryer_data.temp_target - 1.0; // Forçar atualização inicial
    prev_data.total_sensor_failures = -1; // Forçar atualização inicial
    prev_data.total_unsafe_events = -1; // Forçar atualização inicial
    
    // Controle de tela de erro
    static bool error_screen_displayed = false;
    
    LOGI(TAG, "Initial target temperature: %.0f°C", dryer_data.temp_target);
    
    LOGD(TAG, "Updating initial interface...");
    update_interface_smart(&dryer_data, &prev_data);
    LOGD(TAG, "Initial interface updated");
    
    LOGI(TAG, "Entering main loop...");
    
    while (true) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        // Atualiza dados a cada UPDATE_INTERVAL_MS
        if (current_time - last_update >= UPDATE_INTERVAL_MS) {
            last_update = current_time;
            
            // Salvar valores anteriores
            prev_data = dryer_data;
            
            // Atualizar tempo de funcionamento
            dryer_data.uptime = (current_time - start_time) / 1000;
            
            // Ler todos os sensores de uma vez usando o módulo sensor_manager
            sensor_data_t sensor_data;
            sensor_manager_update(&sensor_data, dryer_data.heater_on);
            
            // Processar dados dos sensores e atualizar dryer_data
            process_sensor_data(&sensor_data, &dryer_data);
            
            // Acumular energia total (aproximação simples)
            dryer_data.energy_total += (dryer_data.energy_current * UPDATE_INTERVAL_MS) / 3600000.0; // Wh
            
            // Controle automático de temperatura usando módulo temperature_control
            temperature_control_update(&dryer_data, dryer_data.sensor_safe);
            
            // Gerenciamento de tela baseado no status do sensor
            if (!dryer_data.sensor_safe && !error_screen_displayed) {
                // Sensor falhou - mostrar tela de erro crítica
                display_critical_error_screen();
                error_screen_displayed = true;
                LOGE(TAG, "CRITICAL: Error screen displayed - Sensor failed!");
            } else if (dryer_data.sensor_safe && error_screen_displayed) {
                // Sensor recuperou - voltar à interface normal
                draw_static_interface();
                error_screen_displayed = false;
                // Forçar atualização completa na próxima iteração
                prev_data.temp_target = 39;
                prev_data.heater_on = !dryer_data.heater_on;
                prev_data.pwm_percent = -1;
                prev_data.total_sensor_failures = -1;
                prev_data.total_unsafe_events = -1;
                update_interface_smart(&dryer_data, &prev_data);
                LOGI(TAG, "Main interface restored - Sensor recovered");
            } else if (dryer_data.sensor_safe && !error_screen_displayed) {
                // Operação normal - atualizar interface normalmente
                update_interface_smart(&dryer_data, &prev_data);
            }
            // Se sensor falhou E tela já está exibida, não fazer nada (manter tela de erro)
            
            // Log no serial com status de segurança e PWM
            const char* safety_status = dryer_data.sensor_safe ? "SAFE" : "UNSAFE";
            const char* heater_status = dryer_data.heater_failure ? "[HEATER FAIL]" : "";
            LOGI(TAG, "T:%.1f°C H:%.1f%% E:%.1fW Target:%.0f°C Heater:%s(%.0f%%) [%s]%s",
                   dryer_data.temperature, dryer_data.humidity, dryer_data.energy_current,
                   dryer_data.temp_target,
                   dryer_data.heater_on ? "ON" : "OFF", dryer_data.pwm_percent,
                   safety_status, heater_status);
        }
        
        // Verificar botão de ajuste de temperatura usando módulo button_controller
        bool temp_changed = button_controller_update(&dryer_data);
        
        // Se temperatura alvo mudou, atualizar display imediatamente
        if (temp_changed) {
            LOGI(TAG, "Target temperature changed to %.0f°C", dryer_data.temp_target);
            
            // Atualizar display imediatamente (sem esperar os 5s)
            update_temperature_display(dryer_data.temperature, dryer_data.temp_target,
                                        prev_data.temperature, prev_data.temp_target);
        }
        
        // LED de status usando módulo hardware_control
        hardware_control_led_status(dryer_data.sensor_safe, dryer_data.heater_on);
        
        sleep_ms(100);  // Loop principal a cada 100ms
    }
}