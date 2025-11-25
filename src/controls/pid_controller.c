#include "pid_controller.h"
#include "pico/time.h"
#include <math.h>

void pid_init(pid_controller_t *pid, float kp, float ki, float kd, 
              float output_min, float output_max, uint32_t sample_time_ms) {
    // Configurar ganhos
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    
    // Configurar limites
    pid->output_min = output_min;
    pid->output_max = output_max;
    
    // Anti-windup: limitar integral a 2x o range de saída
    pid->integral_max = (output_max - output_min) * 2.0f;
    
    // Configurar timing
    pid->sample_time = sample_time_ms;
    pid->last_time = to_ms_since_boot(get_absolute_time());
    
    // Resetar estado
    pid->setpoint = 0.0f;
    pid->last_pv = 0.0f;
    pid->integral = 0.0f;
    pid->last_output = 0.0f;
    pid->enabled = true;
    
    // Resetar termos de debug
    pid->debug_p_term = 0.0f;
    pid->debug_i_term = 0.0f;
    pid->debug_d_term = 0.0f;
}

void pid_set_setpoint(pid_controller_t *pid, float setpoint) {
    pid->setpoint = setpoint;
}

void pid_set_tunings(pid_controller_t *pid, float kp, float ki, float kd) {
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
}

float pid_compute(pid_controller_t *pid, float current_value) {
    if (!pid->enabled) {
        return 0.0f;
    }
    
    // Verificar se é hora de calcular (respeitando sample_time)
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    uint32_t time_delta = current_time - pid->last_time;
    
    if (time_delta < pid->sample_time) {
        return pid->last_output; // Retorna última saída calculada
    }
    
    pid->last_time = current_time;
    
    // Calcular erro
    float error = pid->setpoint - current_value;
    
    // Converter time_delta para segundos para os cálculos
    float dt = (float)time_delta / 1000.0f;
    // Evitar divisão por zero no primeiro cálculo
    if (dt < 0.001f) {
        dt = 0.001f;
    }
    
    // === TERMO PROPORCIONAL ===
    // Resposta imediata proporcional ao erro
    float p_term = pid->kp * error;

    // === TERMO DERIVATIVO ===
    float derivative = (current_value - pid->last_pv) / dt;
    float d_term = -pid->kd * derivative;
    
    // === TERMO INTEGRAL ===
    // Acumula o erro ao longo do tempo (elimina erro residual)
    float i_term = pid->ki * pid->integral;
    
    // Cálculo preliminar da saída antes de atualizar o integral
    float tentative_output = p_term + i_term + d_term;
    
    if (!((tentative_output >= pid->output_max && error > 0) ||
          (tentative_output < pid->output_min && error < 0))) {
        // Atualiza o termo integral somente se não estiver saturado
        pid->integral += error * dt;
        
        // Anti-windup: limitar o termo integral
        if (pid->integral > pid->integral_max) {
            pid->integral = pid->integral_max;
        } else if (pid->integral < -pid->integral_max) {
            pid->integral = -pid->integral_max;
        }
        i_term = pid->ki * pid->integral;
    }
    
    // === SAÍDA FINAL ===
    float output = p_term + i_term + d_term;
    
    // Aplicar limites de saída
    if (output > pid->output_max) {
        output = pid->output_max;
    } else if (output < pid->output_min) {
        output = pid->output_min;
    }
    
    // Salva estados
    pid->last_output = output;
    pid->last_pv = current_value;
    pid->debug_p_term = p_term;
    pid->debug_i_term = i_term;
    pid->debug_d_term = d_term;

    return output;
}

void pid_reset(pid_controller_t *pid) {
    pid->integral = 0.0f;
    pid->last_time = to_ms_since_boot(get_absolute_time());
    
    pid->debug_p_term = 0.0f;
    pid->debug_i_term = 0.0f;
    pid->debug_d_term = 0.0f;
}

void pid_enable(pid_controller_t *pid, bool enable) {
    pid->enabled = enable;
    
    // Ao habilitar novamente, reseta o estado para evitar "saltos"
    if (enable) {
        pid_reset(pid);
    }
}

float pid_get_p_term(pid_controller_t *pid) {
    (void)pid; // Unused
    return pid->debug_p_term;
}

float pid_get_i_term(pid_controller_t *pid) {
    (void)pid; // Unused
    return pid->debug_i_term;
}

float pid_get_d_term(pid_controller_t *pid) {
    (void)pid; // Unused
    return pid->debug_d_term;
}
