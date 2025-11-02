#include "display_interface.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// Desenha a interface estática (uma vez só)
void draw_static_interface(void) {
    // Limpar tela apenas uma vez
    st7789_fill_color(BLACK);
    
    // === CABEÇALHO (fixo) ===
    st7789_draw_string(50, 10, "ESTUFA FILAMENTOS", WHITE, BLACK);
    st7789_draw_string(80, 25, "v1.0", CYAN, BLACK);
    
    // Linha separadora
    for (int x = 10; x < 230; x += 8) {
        st7789_draw_string(x, 40, "-", BLUE, BLACK);
    }
    
    // === LABELS FIXOS ===
    st7789_draw_string(10, 55, "TEMPERATURA", YELLOW, BLACK);
    st7789_draw_string(15, 70, "Atual:", WHITE, BLACK);
    st7789_draw_string(15, 85, "Alvo:", WHITE, BLACK);
    st7789_draw_string(160, 85, "(BTN)", GREEN, BLACK);
    
    st7789_draw_string(10, 125, "UMIDADE", CYAN, BLACK);
    
    st7789_draw_string(10, 180, "CONSUMO", MAGENTA, BLACK);
    st7789_draw_string(15, 195, "Atual:", WHITE, BLACK);
    st7789_draw_string(15, 210, "24h:", WHITE, BLACK);
    
    st7789_draw_string(10, 235, "STATUS", WHITE, BLACK);
    
    // Linha separadora inferior
    for (int x = 10; x < 230; x += 8) {
        st7789_draw_string(x, 300, "-", BLUE, BLACK);
    }
}

// Atualiza apenas os valores de temperatura
void update_temperature_display(float temperature, float target, float prev_temp, float prev_target) {
    char buffer[32];
    
    // Só atualiza se mudou
    // Usar epsilon para comparação de floats
    if (fabs(temperature - prev_temp) > 0.05) {  // Mudança > 0.05°C
        sprintf(buffer, "%.1fC  ", temperature); // Espaços extras para limpar
        // Apaga área anterior e desenha novo
        st7789_fill_rect(70, 70, 80, 8, BLACK);
        st7789_draw_string(70, 70, buffer, WHITE, BLACK);
        
        // Atualizar barra de temperatura
        st7789_fill_rect(15, 100, 200, 8, BLACK); // Limpar barra anterior
        int temp_bar_width = (int)((temperature / 80.0) * 200);
        if (temp_bar_width > 200) temp_bar_width = 200;
        if (temp_bar_width > 0) {
            uint16_t color = (temperature < target) ? BLUE : RED;
            st7789_fill_rect(15, 100, temp_bar_width, 8, color);
        }
    }
    
    // Usar epsilon para temperatura alvo também
    if (fabs(target - prev_target) > 0.05) {  // Mudança > 0.05°C
        sprintf(buffer, "%.0fC   ", target);  // Espaços extras para limpar
        st7789_fill_rect(60, 85, 90, 8, BLACK);  // Área maior para limpeza
        st7789_draw_string(60, 85, buffer, GREEN, BLACK);
    }
}

// Atualiza apenas os valores de umidade
void update_humidity_display(float humidity, float prev_humidity) {
    if (humidity != prev_humidity) {
        char buffer[32];
        sprintf(buffer, "%.1f%%  ", humidity);
        
        // Limpar e atualizar valor
        st7789_fill_rect(15, 140, 100, 8, BLACK);
        st7789_draw_string(15, 140, buffer, WHITE, BLACK);
        
        // Atualizar barra de umidade
        st7789_fill_rect(15, 155, 200, 8, BLACK);
        int hum_bar_width = (int)((humidity / 100.0) * 200);
        if (hum_bar_width > 0) {
            st7789_fill_rect(15, 155, hum_bar_width, 8, CYAN);
        }
    }
}

// Atualiza apenas os valores de energia
void update_energy_display(float current, float total_24h, float prev_current, float prev_total) {
    char buffer[32];
    
    if (current != prev_current) {
        sprintf(buffer, "%.1fW  ", current);
        st7789_fill_rect(70, 195, 100, 8, BLACK);
        st7789_draw_string(70, 195, buffer, WHITE, BLACK);
    }
    
    if (total_24h != prev_total) {
        sprintf(buffer, "%.2fkWh  ", total_24h / 1000.0);
        st7789_fill_rect(50, 210, 120, 8, BLACK);
        st7789_draw_string(50, 210, buffer, YELLOW, BLACK);
    }
}

// Atualiza apenas o status com PWM
void update_status_display(bool heater_on, bool fan_on, float pwm_percent, 
                          bool prev_heater, bool prev_fan, float prev_pwm) {
    char buffer[32];
    
    // Status do heater
    if (heater_on != prev_heater) {
        st7789_fill_rect(15, 250, 80, 8, BLACK);
        if (heater_on) {
            st7789_draw_string(15, 250, "AQUECENDO", RED, BLACK);
        } else {
            st7789_draw_string(15, 250, "STANDBY  ", GREEN, BLACK);
        }
    }
    
    // Indicador PWM (abaixo do status)
    if (pwm_percent != prev_pwm) {
        sprintf(buffer, "PWM: %3.0f%%    ", pwm_percent);
        st7789_fill_rect(15, 265, 80, 8, BLACK);
        
        // Cor baseada na potência
        uint16_t pwm_color;
        if (pwm_percent > 75.0) {
            pwm_color = RED;        // Alta potência
        } else if (pwm_percent > 25.0) {
            pwm_color = YELLOW;     // Média potência
        } else if (pwm_percent > 0.0) {
            pwm_color = GREEN;      // Baixa potência
        } else {
            pwm_color = WHITE;      // Desligado
        }
        
        st7789_draw_string(15, 265, buffer, pwm_color, BLACK);
    }
    
    // Status da ventoinha
    if (fan_on != prev_fan) {
        st7789_fill_rect(120, 250, 100, 8, BLACK);
        if (fan_on) {
            st7789_draw_string(120, 250, "VENTILANDO", CYAN, BLACK);
        } else {
            st7789_draw_string(120, 250, "PARADO    ", WHITE, BLACK);
        }
    }
}

// Atualiza apenas o uptime
void update_uptime_display(uint32_t uptime, uint32_t prev_uptime) {
    if (uptime != prev_uptime) {
        char buffer[32];
        int hours = uptime / 3600;
        int minutes = (uptime % 3600) / 60;
        sprintf(buffer, "Uptime: %02d:%02d  ", hours, minutes);
        
        st7789_fill_rect(10, 285, 150, 8, BLACK);
        st7789_draw_string(10, 285, buffer, WHITE, BLACK);
    }
}

// Função principal de atualização inteligente
void update_interface_smart(dryer_data_t *data, dryer_data_t *prev_data) {
    // Atualizar apenas se houve mudanças
    update_temperature_display(data->temperature, data->temp_target, 
                             prev_data->temperature, prev_data->temp_target);
    
    update_humidity_display(data->humidity, prev_data->humidity);
    
    update_energy_display(data->energy_current, data->energy_24h,
                         prev_data->energy_current, prev_data->energy_24h);
    
    update_status_display(data->heater_on, data->fan_on, data->pwm_percent,
                         prev_data->heater_on, prev_data->fan_on, prev_data->pwm_percent);
    
    update_uptime_display(data->uptime, prev_data->uptime);
}

// Tela de inicialização
void display_init_screen(void) {
    st7789_fill_color(BLACK);
    st7789_draw_string(30, 100, "ESTUFA FILAMENTOS", WHITE, BLACK);
    st7789_draw_string(80, 120, "Iniciando...", GREEN, BLACK);
    st7789_draw_string(40, 150, "Aquecendo sistema", YELLOW, BLACK);
}

// Teste de caracteres
void display_test_characters(void) {
    st7789_fill_color(BLACK);
    
    // Teste básico de caracteres
    st7789_draw_string(10, 20, "TESTE DE CARACTERES", WHITE, BLACK);
    st7789_draw_string(10, 35, "ABCDEFGHIJKLMNOPQRS", GREEN, BLACK);
    st7789_draw_string(10, 50, "TUVWXYZ0123456789", GREEN, BLACK);
    st7789_draw_string(10, 65, "abcdefghijklmnopqrs", CYAN, BLACK);
    st7789_draw_string(10, 80, "tuvwxyz!@#$%^&*()", CYAN, BLACK);
    
    // Teste específico dos colchetes
    st7789_draw_string(10, 100, "Colchetes: [BTN]", YELLOW, BLACK);
    st7789_draw_string(10, 115, "Parenteses: (BTN)", RED, BLACK);
    st7789_draw_string(10, 130, "Chars: []{}()<>", WHITE, BLACK);
}