#ifndef ACS712_H
#define ACS712_H

#include "pico/stdlib.h"

typedef enum {
    ACS712_OK,
    ACS712_DISCONNECTED,        // Tensão muito baixa (< 0.5V), provável desconexão
    ACS712_HIGH_VOLTAGE_WARNING // Tensão acima de 2.6V, risco para o ADC (3.3V max)
} acs712_status_code_t;

typedef struct {
    acs712_status_code_t code;
    uint gpio_pin;  // GPIO onde o sensor está conectado
    float voltage;  // Tensão lida (para diagnóstico)
} acs712_status_t;

/**
 * Inicializa o sensor ACS712 no pino especificado.
 * @param gpio_pin Pino GPIO conectado à saída do sensor (deve ser um pino ADC: 26, 27, 28)
 */
void _acs712_init_internal(uint gpio_pin);

// Macro para validar o pino em tempo de compilação
#define acs712_init(pin) do { \
    _Static_assert((pin) == 26 || (pin) == 27 || (pin) == 28, "ACS712 must use ADC pins 26, 27, or 28"); \
    _acs712_init_internal(pin); \
} while(0)

/**
 * Lê a corrente atual em Amperes.
 * Realiza múltiplas leituras para filtrar ruído.
 * @param status Ponteiro para armazenar o status da leitura (opcional, pode ser NULL)
 * @return Corrente em Amperes
 */
float acs712_read_current(acs712_status_t *status);

#endif // ACS712_H
