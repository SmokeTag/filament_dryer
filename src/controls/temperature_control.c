#include "temperature_control.h"
#include "hardware_control.h"
#include "logger.h"
#include <stdio.h>

#define TAG "TempCtrl"

// Controle automático da temperatura COM SEGURANÇA CRÍTICA
void temperature_control_update(dryer_data_t *data, bool sensor_safe) {
    // CRITICAL SAFETY CHECK - Sensor must be operational
    if (!sensor_safe) {
        // PARADA DE EMERGÊNCIA - Desligar aquecedor imediatamente
        data->heater_on = false;
        
        // Aplicar controles de hardware imediatamente
        hardware_control_heater(false);  // FORÇA desligamento do aquecedor
        
        LOGW(TAG, "SAFETY MODE: Heater disabled - Sensor failed");
        return; // Sair sem controle de temperatura
    }
    
    // Controle normal de temperatura (apenas se sensor OK)
    // Histerese de 2°C otimizada para amostragem DHT22 (2-3s)
    // TODO: Substituir por PID quando hardware estiver disponível
    if (data->temperature < (data->temp_target - 2.0)) {
        data->heater_on = true;
    } else if (data->temperature > (data->temp_target + 1.0)) {
        data->heater_on = false;
    }
    // Zona morta: entre (target-2) e (target+1) mantém estado atual
    
    // Controla o hardware (apenas se sensor está OK)
    hardware_control_heater(data->heater_on);
    
    // Atualizar indicação PWM
    hardware_control_update_pwm(data, sensor_safe);
}