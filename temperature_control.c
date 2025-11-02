#include "temperature_control.h"
#include "hardware_control.h"
#include <stdio.h>

// Controle automático da temperatura COM SEGURANÇA CRÍTICA
void temperature_control_update(dryer_data_t *data, bool sensor_safe) {
    // 🚨 VERIFICAÇÃO DE SEGURANÇA CRÍTICA - DHT22 deve estar funcionando
    if (!sensor_safe) {
        // PARADA DE EMERGÊNCIA - Desligar aquecedor imediatamente
        data->heater_on = false;
        data->fan_on = true;  // Manter ventilação para resfriamento de segurança
        
        // Aplicar controles de hardware imediatamente
        hardware_control_heater(false);  // FORÇA desligamento do aquecedor
        hardware_control_fan(true);      // FORÇA ligamento da ventoinha
        
        printf("Temperature Control: 🚨 MODO SEGURANÇA: Aquecedor desabilitado - DHT22 falhou\n");
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
    // Zona morta: entre (target-2) e (target+1) mantém estado atual
    
    // Controla os hardware (apenas se sensor está OK)
    hardware_control_heater(data->heater_on);
    hardware_control_fan(data->fan_on);
    
    // Atualizar indicação PWM
    hardware_control_update_pwm(data, sensor_safe);
}