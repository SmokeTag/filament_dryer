#include "acs712.h"
#include "hardware/adc.h"
#include "logger.h"
#include <math.h>

#define TAG "ACS712"

// Configurações do ACS712 5A
// Alimentação típica de 5V
// Zero Point (0A) = Vcc / 2 = 2.5V
// Sensibilidade = 185 mV/A
#define ACS712_ZERO_VOLTAGE 2.5f
#define ACS712_SENSITIVITY  0.185f

// Configurações do ADC do RP2040
#define ADC_VREF 3.3f
#define ADC_RANGE 4096.0f

static uint adc_channel;
static uint gpio_pin_stored;

void _acs712_init_internal(uint gpio_pin) {
    adc_channel = gpio_pin - 26;
    gpio_pin_stored = gpio_pin;
    adc_gpio_init(gpio_pin);
}

float acs712_read_current(acs712_status_t *status) {
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
    LOGD(TAG, "ACS712 GPIO %d: ADC=%.2f V=%.2fV", gpio_pin_stored, avg_adc, voltage);

    // Validar conexão e segurança
    if (voltage < 0.15f) {
        // Tensão muito baixa, sensor desabilitado ou desconectado
        if (status) {
            status->code = ACS712_DISCONNECTED;
            status->gpio_pin = gpio_pin_stored;
            status->voltage = voltage;
        }
        return 0.0f;
    } else if (voltage > 2.6f) {
        // Tensão acima do ponto zero (2.5V)
        // Se subir muito, pode queimar o ADC (max 3.3V)
        if (status) {
            status->code = ACS712_HIGH_VOLTAGE_WARNING;
            status->gpio_pin = gpio_pin_stored;
            status->voltage = voltage;
        }
    } else {
        if (status) {
            status->code = ACS712_OK;
            status->gpio_pin = gpio_pin_stored;
            status->voltage = voltage;
        }
    }

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
