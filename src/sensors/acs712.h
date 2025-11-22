#ifndef ACS712_H
#define ACS712_H

#include "pico/stdlib.h"

/**
 * Inicializa o sensor ACS712 no pino especificado.
 * @param gpio_pin Pino GPIO conectado à saída do sensor (deve ser um pino ADC: 26, 27, 28)
 */
void acs712_init(uint gpio_pin);

/**
 * Lê a corrente atual em Amperes.
 * Realiza múltiplas leituras para filtrar ruído.
 * @return Corrente em Amperes
 */
float acs712_read_current(void);

#endif // ACS712_H
