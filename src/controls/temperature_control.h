#ifndef TEMPERATURE_CONTROL_H
#define TEMPERATURE_CONTROL_H

#include "display_interface.h"
#include <stdbool.h>

// Funções públicas do módulo
void temperature_control_update(dryer_data_t *data, bool sensor_safe);

#endif // TEMPERATURE_CONTROL_H