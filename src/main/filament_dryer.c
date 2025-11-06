// TODO: Think about adding a LED indicator
// TODO: Implementar aviso de falha do sensor no display
// TODO: Implementar controle de temperatura baseado em PID
// TODO: considerar usar interrupções para o botão em vez de polling
// TODO: adicionar moldura ao redor das barras de status no display

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
#include "pico/time.h"
#include <stdio.h>
#include <string.h>

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
    printf("Main: Inicializando display...\n");
    st7789_init();
    printf("Main: Display inicializado!\n");
    
    printf("Main: Sistema da estufa completamente inicializado!\n");
}

int main() {
    // Initialize standard I/O for debugging
    stdio_init_all();
    sleep_ms(1500);  // Aguarda estabilizar USB
    
    printf("\n=== ESTUFA DE FILAMENTOS v2.0 (Modular) ===\n");
    printf("Main: Iniciando sistema...\n");
    
    // Initialize the onboard LED
    int rc = hardware_control_led_init();
    if (rc != 0) {
        printf("Main: Erro ao inicializar LED\n");
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
        .pwm_percent = 0.0
    };
    
    printf("Main: Sistema iniciado! Temperatura alvo: %.0f°C\n", dryer_data.temp_target);
    
    // Tela de inicialização com teste de caracteres
    printf("Main: Iniciando tela de teste de caracteres...\n");
    display_test_characters();
    printf("Main: Tela de teste concluída, aguardando 5s...\n");
    sleep_ms(5000);
    
    // Tela de inicialização normal
    printf("Main: Iniciando tela de inicialização...\n");
    display_init_screen();
    printf("Main: Tela de inicialização concluída, aguardando 3s...\n");
    sleep_ms(3000);
    
    // Desenhar interface estática uma única vez
    printf("Main: Desenhando interface estática...\n");
    draw_static_interface();
    printf("Main: Interface estática desenhada!\n");
    
    // Variáveis para controlar atualizações
    uint32_t last_update = 0;
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    
    // Estrutura para guardar valores anteriores
    dryer_data_t prev_data = dryer_data;
    prev_data.temp_target = dryer_data.temp_target - 1.0; // Forçar atualização inicial
    
    printf("Main: 🎯 Temperatura alvo inicial: %.0f°C\n", dryer_data.temp_target);
    
    printf("Main: Atualizando interface inicial...\n");
    update_interface_smart(&dryer_data, &prev_data);
    printf("Main: Interface inicial atualizada!\n");
    
    printf("Main: Entrando no loop principal...\n");
    
    while (true) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        // Atualiza dados a cada UPDATE_INTERVAL_MS
        if (current_time - last_update >= UPDATE_INTERVAL_MS) {
            last_update = current_time;
            
            // Salvar valores anteriores
            prev_data = dryer_data;
            
            // Atualizar tempo de funcionamento
            dryer_data.uptime = (current_time - start_time) / 1000;
            
            // Ler sensores usando o módulo sensor_manager
            sensor_data_t sensor_data;
            sensor_manager_update(&sensor_data);
            dryer_data.temperature = sensor_data.temperature;
            dryer_data.humidity = sensor_data.humidity;
            dryer_data.sensor_safe = sensor_data.sensor_safe;
            
            // Ler sensor de energia
            dryer_data.energy_current = sensor_manager_read_energy();
            
            // Acumular energia total (aproximação simples)
            dryer_data.energy_total += (dryer_data.energy_current * UPDATE_INTERVAL_MS) / 3600000.0; // Wh
            
            // Controle automático de temperatura usando módulo temperature_control
            temperature_control_update(&dryer_data, dryer_data.sensor_safe);
            
            // Atualizar display de forma inteligente (apenas o que mudou)
            update_interface_smart(&dryer_data, &prev_data);
            
            // Log no serial com status de segurança e PWM
            const char* safety_status = dryer_data.sensor_safe ? "SAFE" : "⚠️UNSAFE";
            printf("Main: T:%.1f°C H:%.1f%% E:%.1fW Target:%.0f°C Heater:%s(%.0f%%) [%s]\n",
                   dryer_data.temperature, dryer_data.humidity, dryer_data.energy_current,
                   dryer_data.temp_target,
                   dryer_data.heater_on ? "ON" : "OFF", dryer_data.pwm_percent,
                   safety_status);
        }
        
        // Verificar botão de ajuste de temperatura usando módulo button_controller
        bool temp_changed = button_controller_update(&dryer_data);
        
        // Se temperatura alvo mudou, atualizar display imediatamente
        if (temp_changed) {
            printf("Main: 🎮 Temperatura alvo mudou para %.0f°C\n", dryer_data.temp_target);
            
            // Atualizar display imediatamente (sem esperar os 5s)
            update_temperature_display(dryer_data.temperature, dryer_data.temp_target,
                                        prev_data.temperature, prev_data.temp_target);
        }
        
        // LED de status usando módulo hardware_control
        hardware_control_led_status(dryer_data.sensor_safe, dryer_data.heater_on);
        
        sleep_ms(100);  // Loop principal a cada 100ms
    }
}