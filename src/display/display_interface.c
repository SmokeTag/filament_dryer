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
    st7789_fill_rect(0, 40, DISPLAY_WIDTH, 2, BLUE);
    
    // === LABELS FIXOS ===
    st7789_draw_string(10, 55, "TEMPERATURA", YELLOW, BLACK);
    st7789_draw_string(15, 70, "Atual:", WHITE, BLACK);
    st7789_draw_string(15, 85, "Alvo:", WHITE, BLACK);
    st7789_draw_string(160, 85, "(BTN)", GREEN, BLACK);
    
    // Moldura da barra de temperatura
    st7789_fill_rect(14, 99, 202, 10, WHITE);  // Moldura externa
    st7789_fill_rect(15, 100, 200, 8, BLACK);   // Interior preto
    
    st7789_draw_string(10, 125, "UMIDADE", CYAN, BLACK);
    
    // Moldura da barra de umidade
    st7789_fill_rect(14, 154, 202, 10, WHITE);  // Moldura externa
    st7789_fill_rect(15, 155, 200, 8, BLACK);   // Interior preto
    
    st7789_draw_string(10, 180, "CONSUMO", MAGENTA, BLACK);
    st7789_draw_string(15, 195, "Atual:", WHITE, BLACK);
    st7789_draw_string(15, 210, "Total:", WHITE, BLACK);
    
    st7789_draw_string(10, 235, "STATUS", WHITE, BLACK);

    st7789_draw_string(10, 285, "UPTIME:", WHITE, BLACK);
    
    // Linha separadora inferior
    st7789_fill_rect(0, 300, DISPLAY_WIDTH, 2, BLUE);
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
        st7789_fill_rect(15, 100, 200, 8, BLACK); // Limpar interior
        st7789_fill_rect(216, 100, 200, 8, BLACK); // Limpar overflow
        
        // Calcular largura da barra baseada no target (200px = target)
        int temp_bar_width = (int)((temperature / target) * 200);
        
        if (temp_bar_width > 0) {
            uint16_t color = (temperature <= target) ? BLUE : RED;
            
            // Se não excede o target, desenhar só dentro da moldura
            if (temp_bar_width <= 200) {
                st7789_fill_rect(15, 100, temp_bar_width, 8, color);
            } else {
                // Excede target: preencher moldura + overflow
                st7789_fill_rect(15, 100, 200, 8, color);  // Parte normal
                st7789_fill_rect(216, 100, temp_bar_width - 201, 8, color); // Overflow
            }
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
        
        // Atualizar barra de umidade (preservar moldura)
        // Limpar apenas o interior da moldura
        st7789_fill_rect(15, 155, 200, 8, BLACK);
        
        int hum_bar_width = (int)((humidity / 100.0) * 200);
        if (hum_bar_width > 200) hum_bar_width = 200; // Limitar a 100%
        
        if (hum_bar_width > 0) {
            st7789_fill_rect(15, 155, hum_bar_width, 8, CYAN);
        }
    }
}

// Atualiza apenas os valores de energia
void update_energy_display(float current, float total, float prev_current, float prev_total) {
    char buffer[32];
    
    if (current != prev_current) {
        sprintf(buffer, "%.1fW  ", current);
        st7789_fill_rect(70, 195, 100, 8, BLACK);
        st7789_draw_string(70, 195, buffer, WHITE, BLACK);
    }
    
    if (total != prev_total) {
        sprintf(buffer, "%.2fkWh  ", total / 1000.0);
        st7789_fill_rect(70, 210, 120, 8, BLACK);
        st7789_draw_string(70, 210, buffer, YELLOW, BLACK);
    }
}

// Atualiza apenas o status com PWM
void update_status_display(bool heater_on, float pwm_percent, 
                          bool prev_heater, float prev_pwm) {
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
}

// Atualiza uptime com formato inteligente para longos períodos
void update_uptime_display(uint32_t uptime, uint32_t prev_uptime) {
    if (uptime != prev_uptime) {
        char buffer[32];
        
        uint32_t days = uptime / (24 * 3600);
        uint32_t hours = (uptime % (24 * 3600)) / 3600;
        uint32_t minutes = (uptime % 3600) / 60;
        
        // Formato automático baseado no tempo decorrido
        if (days > 0) {
            // Mais de 1 dia: mostra dias e horas
            sprintf(buffer, "%dd %02dh    ", days, hours);
        } else if (hours > 0) {
            // Mais de 1 hora: mostra horas e minutos  
            sprintf(buffer, "%02d:%02d    ", hours, minutes);
        } else {
            // Menos de 1 hora: mostra apenas minutos
            sprintf(buffer, "%02dm      ", minutes);
        }
        
        st7789_fill_rect(74, 285, 150, 8, BLACK);
        st7789_draw_string(74, 285, buffer, WHITE, BLACK);
    }
}

// Tela completa de erro crítico
void display_critical_error_screen(void) {
    // Tela vermelha de emergência
    st7789_fill_color(RED);
    
    // Título de erro em branco
    st7789_draw_string(60, 30, "ERRO CRITICO!", WHITE, RED);
    
    // Linha separadora
    st7789_fill_rect(20, 50, 200, 3, WHITE);
    
    // Mensagem principal
    st7789_draw_string(30, 80, "SENSOR DHT22 FALHOU", WHITE, RED);
    st7789_draw_string(50, 100, "SISTEMA UNSAFE", WHITE, RED);
    
    // Ações tomadas
    st7789_draw_string(20, 130, "AQUECEDOR DESLIGADO", YELLOW, RED);
    st7789_draw_string(20, 150, "MODO SEGURANCA ATIVO", YELLOW, RED);
    
    // Linha separadora
    st7789_fill_rect(20, 170, 200, 2, WHITE);
    
    // Instruções
    st7789_draw_string(30, 190, "VERIFIQUE CONEXOES", WHITE, RED);
    st7789_draw_string(40, 210, "SENSOR DHT22", WHITE, RED);
    
    // Status de recuperação
    st7789_draw_string(20, 240, "TENTANDO RECONECTAR...", CYAN, RED);
    
    // Linha inferior
    st7789_fill_rect(20, 270, 200, 2, WHITE);
    st7789_draw_string(40, 285, "SISTEMA REINICIARA", WHITE, RED);
    st7789_draw_string(30, 300, "QUANDO SENSOR VOLTAR", WHITE, RED);
}



// Função principal de atualização inteligente
void update_interface_smart(dryer_data_t *data, dryer_data_t *prev_data) {
    // Atualizar apenas se houve mudanças
    update_temperature_display(data->temperature, data->temp_target, 
                             prev_data->temperature, prev_data->temp_target);
    
    update_humidity_display(data->humidity, prev_data->humidity);
    
    update_energy_display(data->energy_current, data->energy_total,
                         prev_data->energy_current, prev_data->energy_total);
    
    update_status_display(data->heater_on, data->pwm_percent,
                         prev_data->heater_on, prev_data->pwm_percent);
    
    update_uptime_display(data->uptime, prev_data->uptime);
}

// Tela de inicialização
void display_init_screen(void) {
    st7789_fill_color(BLACK);
    st7789_draw_string(30, 100, "ESTUFA FILAMENTOS", WHITE, BLACK);
    st7789_draw_string(80, 120, "Iniciando...", WHITE, BLACK);
    st7789_draw_string(40, 150, "Aquecendo sistema", WHITE, BLACK);
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
    
    // Teste individual do caractere problemático
    st7789_draw_string(10, 145, "ASCII 91: [", GREEN, BLACK);
    st7789_draw_string(10, 160, "ASCII 93: ]", RED, BLACK);
    st7789_draw_char(90, 160, ']', WHITE, BLACK);  // Teste direto do char
    st7789_draw_char(100, 160, (char)93, CYAN, BLACK);  // Cast explícito
}