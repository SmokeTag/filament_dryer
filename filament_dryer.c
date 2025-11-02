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
#include "dht22.h"
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
#define TEMP_MIN 40              // Temperatura mínima (°C)
#define TEMP_MAX 85              // Temperatura máxima (°C)
#define TEMP_STEP_SINGLE 1       // Incremento simples (°C)
#define TEMP_STEP_FAST 5         // Incremento rápido quando segurando (°C)
#define ENERGY_RESET_HOURS 24    // Reset do consumo a cada 24h

// Configurações do botão
#define BUTTON_DEBOUNCE_MS 35    // Tempo de debounce (ms)
#define BUTTON_HOLD_THRESHOLD_MS 650   // Tempo para ativar modo rápido (ms)
#define BUTTON_FAST_REPEAT_MS 470      // Intervalo do incremento rápido (ms)

// Pinos dos sensores (ajuste conforme sua montagem)
#define DHT22_PIN 22            // GPIO para DHT22
#define ENERGY_SENSOR_PIN 26    // GPIO ADC para sensor de energia
#define HEATER_PIN 15           // GPIO para controle do hotend
#define FAN_PIN 14              // GPIO para controle da ventoinha
#define BUTTON_PIN 16           // GPIO para botão de ajuste temperatura

// A estrutura dryer_data_t agora está definida em display_interface.h

// Variáveis para controle de leitura do DHT22
static uint32_t last_dht22_read = 0;
static float last_temperature = 25.0;  // Valores padrão
static float last_humidity = 50.0;
static bool dht22_initialized = false;
static uint32_t dht22_error_count = 0;
static uint32_t dht22_success_count = 0;
static bool dht22_sensor_ok = false;    // Status crítico do sensor

// Variáveis para controle do botão com debouncing
static bool button_state = false;           // Estado atual do botão
static bool button_prev_state = false;      // Estado anterior do botão
static uint32_t button_last_change = 0;     // Última mudança de estado
static uint32_t button_press_start = 0;     // Início do pressionamento
static bool button_debounced = false;       // Estado após debouncing
static bool button_was_pressed = false;     // Flag para detectar press único
static bool button_in_fast_mode = false;    // Modo incremento rápido ativo
static uint32_t button_last_fast_increment = 0;  // Último incremento rápido

// Variáveis para controle PWM
static float current_pwm_percent = 0.0;     // Potência atual aplicada (0-100%)

#define DHT22_READ_INTERVAL_MS 2000     // DHT22 precisa de pelo menos 2s entre leituras
#define DHT22_MAX_CONSECUTIVE_ERRORS 3  // Máximo de erros consecutivos antes de PARADA DE SEGURANÇA

// Leitura CRÍTICA do DHT22 - SEM SIMULAÇÃO
// Se o sensor falhar, o aquecedor DEVE ser desligado por segurança
void read_dht22(float *temperature, float *humidity) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // Inicializar DHT22 na primeira chamada
    if (!dht22_initialized) {
        dht22_init(DHT22_PIN);
        dht22_initialized = true;
        printf("*** DHT22 SENSOR CRÍTICO inicializado no GPIO %d ***\n", DHT22_PIN);
        // SEM sleep_ms aqui - será controlado pelo intervalo de leitura
        last_dht22_read = current_time; // Marcar tempo para próxima leitura
        dht22_sensor_ok = true; // Assumir OK inicialmente
        printf("DHT22 pronto para leituras!\n");
    }
    
    // Verificar se já passou tempo suficiente desde a última leitura
    if (current_time - last_dht22_read >= DHT22_READ_INTERVAL_MS) {
        last_dht22_read = current_time;
        
        float new_temp, new_hum;
        dht22_result_t result = dht22_read(&new_temp, &new_hum);
        
        if (result == DHT22_OK) {
            // ✅ SENSOR OK - Sistema pode operar normalmente
            last_temperature = new_temp;
            last_humidity = new_hum;
            dht22_success_count++;
            dht22_error_count = 0; // Reset contador de erros
            dht22_sensor_ok = true;
            
            // Log ocasional de sucesso
            if (dht22_success_count % 20 == 1) {
                printf("DHT22 ✅ OK: %.1f°C, %.1f%% (sucessos: %lu)\n", 
                       new_temp, new_hum, dht22_success_count);
            }
        } else {
            // ❌ ERRO CRÍTICO - Incrementar contador
            dht22_error_count++;
            printf("❌ DHT22 ERRO CRÍTICO #%lu: %s\n", 
                   dht22_error_count, dht22_error_string(result));
            
            // PARADA DE SEGURANÇA se muitos erros consecutivos
            if (dht22_error_count >= DHT22_MAX_CONSECUTIVE_ERRORS) {
                dht22_sensor_ok = false;
                printf("\n🚨 ALERTA DE SEGURANÇA: DHT22 FALHOU! 🚨\n");
                printf("🔥 AQUECEDOR DESABILITADO POR SEGURANÇA\n");
                printf("🔧 VERIFIQUE CONEXÕES DO SENSOR\n");
                printf("📊 Erros consecutivos: %lu\n\n", dht22_error_count);
            }
        }
    }
    
    // IMPORTANTE: Retornar últimos valores mesmo com erro
    // O controle de segurança é feito via dht22_sensor_ok
    *temperature = last_temperature;
    *humidity = last_humidity;
}

// Nova função para verificar status de segurança do sensor
bool is_dht22_sensor_safe(void) {
    return dht22_sensor_ok;
}

// Função para tentar recuperar sensor após erro
void dht22_recovery_attempt(void) {
    if (!dht22_sensor_ok && dht22_error_count >= DHT22_MAX_CONSECUTIVE_ERRORS) {
        printf("🔄 Tentativa de recuperação do DHT22...\n");
        dht22_error_count = 0; // Reset para dar nova chance
        dht22_sensor_ok = true; // Permitir nova tentativa
    }
}

// Inicialização do botão
void button_init(void) {
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);  // Pull-up interno (botão conecta ao GND)
}

// Debouncing digital do botão
bool button_read_debounced(void) {
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

// Verificar se botão foi pressionado (edge detection) - DEPRECATED
// Agora usamos lógica avançada diretamente em adjust_target_temperature()
bool button_pressed(void) {
    if (button_read_debounced()) {
        return button_state;  // True se foi pressionado
    }
    return false;
}

// Ajustar temperatura alvo com botão (modo melhorado - incrementa ao soltar)
void adjust_target_temperature(dryer_data_t *data) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    bool state_changed = button_read_debounced();
    
    // Detectar início do pressionamento
    if (state_changed && button_state) {
        // Botão foi pressionado - apenas marcar início (sem incrementar ainda)
        button_was_pressed = true;
        button_in_fast_mode = false;
        button_press_start = current_time;
        
        printf("🔘 Botão pressionado - aguardando soltar...\n");
        return;
    }
    
    // Detectar fim do pressionamento (AQUI QUE INCREMENTA)
    if (state_changed && !button_state) {
        // Botão foi solto - calcular ação baseada no tempo
        uint32_t press_duration = current_time - button_press_start;
        button_was_pressed = false;
        button_in_fast_mode = false;
        
        if (press_duration < BUTTON_HOLD_THRESHOLD_MS) {
            // Pressão curta: +1°C
            data->temp_target += TEMP_STEP_SINGLE;
            if (data->temp_target > TEMP_MAX) {
                data->temp_target = TEMP_MIN;
            }
            printf("🌡️ Pressão curta: +1°C → %.0f°C\n", data->temp_target);
        } else {
            // Pressão longa: +5°C (incremento final do modo rápido)
            data->temp_target += TEMP_STEP_FAST;
            if (data->temp_target > TEMP_MAX) {
                data->temp_target = TEMP_MIN;
            }
            printf("🌡️ Modo rápido finalizado: +5°C → %.0f°C\n", data->temp_target);
        }
        return;
    }
    
    // Botão ainda pressionado - verificar se ativa modo rápido
    if (button_state && button_was_pressed) {
        uint32_t press_duration = current_time - button_press_start;
        
        // Ativar modo rápido após BUTTON_HOLD_THRESHOLD_MS
        if (press_duration >= BUTTON_HOLD_THRESHOLD_MS) {
            if (!button_in_fast_mode) {
                button_in_fast_mode = true;
                button_last_fast_increment = current_time;
                printf("🚀 Modo rápido ativado! (incremente ao soltar)\n");
                
                // Primeiro incremento rápido imediato ao ativar modo
                data->temp_target += TEMP_STEP_FAST;
                if (data->temp_target > TEMP_MAX) {
                    data->temp_target = TEMP_MIN;
                }
                printf("🌡️ Modo rápido: +5°C → %.0f°C\n", data->temp_target);
            }
            
            // Incrementos rápidos adicionais a cada BUTTON_FAST_REPEAT_MS
            if (current_time - button_last_fast_increment >= BUTTON_FAST_REPEAT_MS) {
                button_last_fast_increment = current_time;
                
                data->temp_target += TEMP_STEP_FAST;
                if (data->temp_target > TEMP_MAX) {
                    data->temp_target = TEMP_MIN;
                }
                
                printf("🌡️ Modo rápido: +5°C → %.0f°C\n", data->temp_target);
            }
        }
    }
}

// Calcular PWM atual baseado no estado do heater
void update_pwm_indication(dryer_data_t *data) {
    if (!is_dht22_sensor_safe()) {
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

// Declarações das funções de segurança do DHT22
bool is_dht22_sensor_safe(void);
void dht22_recovery_attempt(void);

// Inicialização dos componentes
void dryer_init(void) {
    // Inicializar GPIOs
    gpio_init(HEATER_PIN);
    gpio_init(FAN_PIN);
    gpio_set_dir(HEATER_PIN, GPIO_OUT);
    gpio_set_dir(FAN_PIN, GPIO_OUT);
    
    // Inicializar botão
    button_init();
    
    // Inicializar ADC para sensor de energia
    adc_init();
    adc_gpio_init(ENERGY_SENSOR_PIN);
    
    // Estado inicial seguro
    control_heater(false);
    control_fan(false);
    
    printf("Sistema da estufa inicializado!\n");
    printf("Botão de ajuste temperatura no GPIO %d\n", BUTTON_PIN);
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

// Controle automático da temperatura COM SEGURANÇA CRÍTICA
void temperature_control(dryer_data_t *data) {
    // 🚨 VERIFICAÇÃO DE SEGURANÇA CRÍTICA - DHT22 deve estar funcionando
    if (!is_dht22_sensor_safe()) {
        // PARADA DE EMERGÊNCIA - Desligar aquecedor imediatamente
        data->heater_on = false;
        data->fan_on = true;  // Manter ventilação para resfriamento de segurança
        
        // Aplicar controles de hardware imediatamente
        control_heater(false);  // FORÇA desligamento do aquecedor
        control_fan(true);      // FORÇA ligamento da ventoinha
        
        printf("🚨 MODO SEGURANÇA: Aquecedor desabilitado - DHT22 falhou\n");
        return; // Sair sem controle de temperatura
    }
    
    // Controle normal de temperatura (apenas se sensor OK)
    // Histerese de 2°C otimizada para amostragem DHT22 (2-3s)
    // TODO: Substituir por PID quando hardware estiver disponível
    if (data->temperature < (data->temp_target - 2.0)) {
        data->heater_on = true;
        data->fan_on = true;  // Ventilação forçada quando aquecendo
    } else if (data->temperature > (data->temp_target + 1.0)) {
        data->heater_on = false;
        data->fan_on = true;  // Continua ventilando para resfriar
    }
    // Zona morta: entre (target-2) e (target) mantém estado atual
    
    // Controla os hardware (apenas se sensor está OK)
    control_heater(data->heater_on);
    control_fan(data->fan_on);
    
    // Atualizar indicação PWM
    update_pwm_indication(data);
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
        .sensor_safe = true,  // Assume sensor OK no início
        .uptime = 0,
        .pwm_percent = 0.0
    };
    
    printf("Sistema iniciado! Temperatura alvo: %.0f°C\n", dryer_data.temp_target);
    
    printf("Iniciando tela de teste de caracteres...\n");
    // Tela de inicialização com teste de caracteres
    display_test_characters();
    printf("Tela de teste concluída, aguardando 5s...\n");
    sleep_ms(5000);  // Mostra por 5 segundos
    
    printf("Iniciando tela de inicialização...\n");
    // Tela de inicialização normal
    display_init_screen();
    printf("Tela de inicialização concluída, aguardando 3s...\n");
    sleep_ms(3000);
    
    printf("Desenhando interface estática...\n");
    // Desenhar interface estática uma única vez
    draw_static_interface();
    printf("Interface estática desenhada!\n");
    
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
    prev_data.sensor_safe = !dryer_data.sensor_safe; // Força atualização
    prev_data.uptime = -1;         // Força atualização
    prev_data.pwm_percent = -1;    // Força atualização PWM
    
    printf("🎯 Temperatura alvo inicial: %.0f°C\n", dryer_data.temp_target);
    
    printf("Atualizando interface inicial...\n");
    update_interface_smart(&dryer_data, &prev_data);
    printf("Interface inicial atualizada!\n");
    
    printf("Entrando no loop principal...\n");
    
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
            dryer_data.sensor_safe = is_dht22_sensor_safe();  // Atualizar status de segurança
            dryer_data.energy_current = read_energy_sensor();
            
            // Acumular energia (aproximação simples)
            dryer_data.energy_24h += (dryer_data.energy_current * UPDATE_INTERVAL_MS) / 3600000.0; // Wh
            
            // Reset do consumo diário (simplificado - a cada 24h de uptime)
            if (dryer_data.uptime > 0 && (dryer_data.uptime % (24 * 3600)) == 0) {
                dryer_data.energy_24h = 0.0;
                printf("Reset do consumo diário\n");
            }
            
            // Controle automático de temperatura (com verificação de segurança)
            temperature_control(&dryer_data);
            
            // Tentativa de recuperação do DHT22 a cada 30 segundos se falhou
            static uint32_t last_recovery_attempt = 0;
            if (!is_dht22_sensor_safe() && 
                (current_time - last_recovery_attempt >= 30000)) {
                last_recovery_attempt = current_time;
                dht22_recovery_attempt();
            }
            
            // Atualizar display de forma inteligente (apenas o que mudou)
            update_interface_smart(&dryer_data, &prev_data);
            
            // Log no serial com status de segurança e PWM
            const char* safety_status = is_dht22_sensor_safe() ? "SAFE" : "⚠️UNSAFE";
            printf("T:%.1f°C H:%.1f%% E:%.1fW Target:%.0f°C Heater:%s(%.0f%%) Fan:%s [%s]\n",
                   dryer_data.temperature, dryer_data.humidity, dryer_data.energy_current,
                   dryer_data.temp_target,
                   dryer_data.heater_on ? "ON" : "OFF", dryer_data.pwm_percent,
                   dryer_data.fan_on ? "ON" : "OFF",
                   safety_status);
        }
        
        // Verificar botão de ajuste de temperatura (fora do intervalo principal para resposta rápida)
        static float last_target_logged = -1;
        float old_target = dryer_data.temp_target;  // Salvar valor antes
        adjust_target_temperature(&dryer_data);
        
        // Se temperatura alvo mudou, atualizar display imediatamente
        if (dryer_data.temp_target != old_target) {
            printf("🎮 MAIN: Temperatura alvo mudou para %.0f°C\n", dryer_data.temp_target);
            
            // Atualizar display imediatamente (sem esperar os 5s)
            dryer_data_t temp_prev_data = prev_data;
            temp_prev_data.temp_target = old_target;  // Usar valor anterior
            update_temperature_display(dryer_data.temperature, dryer_data.temp_target, 
                                     temp_prev_data.temperature, temp_prev_data.temp_target);
            
            last_target_logged = dryer_data.temp_target;
        }
        
        // LED de status (diferentes padrões para diferentes estados)
        int led_interval;
        if (!is_dht22_sensor_safe()) {
            // DHT22 falhou - pisca muito rápido (ALERTA)
            led_interval = 100;
        } else if (dryer_data.heater_on) {
            // Aquecendo - pisca moderado
            led_interval = 250;
        } else {
            // Normal - pisca lento
            led_interval = 1000;
        }
        
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
