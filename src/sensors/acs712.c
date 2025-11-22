#include "acs712.h"
#include "hardware/adc.h"
#include <math.h>

// Configurações do ACS712 5A
// Alimentação típica de 5V
// Zero Point (0A) = Vcc / 2 = 2.5V
// Sensibilidade = 185 mV/A
#define ACS712_ZERO_VOLTAGE 2.5f
#define ACS712_SENSITIVITY  0.185f

// Configurações do ADC do RP2040
#define ADC_VREF 3.3f
#define ADC_RANGE 4096.0f

static uint acs712_pin;

void acs712_init(uint gpio_pin) {
    acs712_pin = gpio_pin;
    adc_gpio_init(acs712_pin);
}

float acs712_read_current(void) {
    // Determinar canal ADC baseando-se no pino (26=0, 27=1, 28=2)
    uint adc_channel = acs712_pin - 26;
    if (adc_channel > 3) return 0.0f;

    adc_select_input(adc_channel);

    // Realizar múltiplas leituras para média (filtro simples)
    // O ACS712 pode ser ruidoso
    uint32_t sum = 0;
    const int samples = 100;
    
    for(int i = 0; i < samples; i++) {
        sum += adc_read();
        sleep_us(20); // Pequeno delay entre amostras
    }

    float avg_adc = (float)sum / samples;
    
    // Converter valor ADC para Tensão no pino
    float voltage = (avg_adc / ADC_RANGE) * ADC_VREF;

    // Converter Tensão para Corrente
    // Fórmula: Corrente = (TensãoLida - TensãoZero) / Sensibilidade
    // Nota: Se houver divisor de tensão no hardware, ajustar aqui.
    // Assumindo conexão direta (cuidado com 5V no pino 3.3V se corrente for alta positiva)
    
    float current = fabs(voltage - ACS712_ZERO_VOLTAGE) / ACS712_SENSITIVITY;

    // Remover offset pequeno próximo de zero (ruído)
    if (fabs(current) < 0.05f) {
        current = 0.0f;
    }

    return current;
}
