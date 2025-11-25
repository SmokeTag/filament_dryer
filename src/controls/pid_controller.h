#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Estrutura do controlador PID
 * 
 * O controlador PID calcula a saída de controle baseado em três termos:
 * - P (Proporcional): Resposta imediata ao erro atual
 * - I (Integral): Elimina erro residual acumulado ao longo do tempo
 * - D (Derivativo): Antecipa mudanças futuras baseado na taxa de variação
 */
typedef struct {
    // Ganhos do PID (configuráveis)
    float kp;                   // Ganho proporcional
    float ki;                   // Ganho integral
    float kd;                   // Ganho derivativo
    
    // Limites de saída
    float output_min;           // Limite mínimo de saída (ex: 0%)
    float output_max;           // Limite máximo de saída (ex: 100%)
    
    // Limite anti-windup para o termo integral
    float integral_max;         // Máximo valor absoluto do termo integral
    
    // Variáveis internas de estado
    float setpoint;             // Valor desejado (temperatura alvo)
    float integral;             // Acumulador do termo integral
    float last_output;          // Última saída calculada
    float last_pv;             // Último valor do processo

    // Variáveis para debug/tunning    
    float debug_p_term;
    float debug_i_term;
    float debug_d_term;
    
    // Controle de tempo
    uint32_t last_time;         // Timestamp da última execução (ms)
    uint32_t sample_time;       // Intervalo entre cálculos (ms)
    
    // Estado
    bool enabled;               // PID habilitado ou desabilitado
} pid_controller_t;

/**
 * Inicializa o controlador PID com os ganhos especificados
 * 
 * @param pid Ponteiro para a estrutura do PID
 * @param kp Ganho proporcional (valor inicial sugerido: 10.0)
 * @param ki Ganho integral (valor inicial sugerido: 0.5)
 * @param kd Ganho derivativo (valor inicial sugerido: 1.0)
 * @param output_min Limite mínimo de saída (normalmente 0.0)
 * @param output_max Limite máximo de saída (normalmente 100.0)
 * @param sample_time_ms Intervalo entre cálculos em milissegundos (sugerido: 1000)
 */
void pid_init(pid_controller_t *pid, float kp, float ki, float kd, 
              float output_min, float output_max, uint32_t sample_time_ms);

/**
 * Define o setpoint (valor alvo) do PID
 * 
 * @param pid Ponteiro para a estrutura do PID
 * @param setpoint Novo valor alvo (ex: temperatura desejada em °C)
 */
void pid_set_setpoint(pid_controller_t *pid, float setpoint);

/**
 * Atualiza os ganhos do PID em tempo de execução
 * Útil para tunning/ajuste fino dos parâmetros
 * 
 * @param pid Ponteiro para a estrutura do PID
 * @param kp Novo ganho proporcional
 * @param ki Novo ganho integral
 * @param kd Novo ganho derivativo
 */
void pid_set_tunings(pid_controller_t *pid, float kp, float ki, float kd);

/**
 * Calcula a saída do PID baseado no valor atual (process variable)
 * 
 * Esta função deve ser chamada periodicamente (de acordo com sample_time)
 * 
 * @param pid Ponteiro para a estrutura do PID
 * @param current_value Valor atual da variável controlada (ex: temperatura atual)
 * @return Valor de saída do PID (duty cycle PWM em %)
 */
float pid_compute(pid_controller_t *pid, float current_value);

/**
 * Reseta o estado interno do PID
 * Limpa o termo integral e erro anterior
 * Útil ao mudar o setpoint drasticamente ou reiniciar o controle
 * 
 * @param pid Ponteiro para a estrutura do PID
 */
void pid_reset(pid_controller_t *pid);

/**
 * Habilita ou desabilita o controlador PID
 * Quando desabilitado, pid_compute() retorna 0
 * 
 * @param pid Ponteiro para a estrutura do PID
 * @param enable true para habilitar, false para desabilitar
 */
void pid_enable(pid_controller_t *pid, bool enable);

/**
 * Retorna o termo proporcional atual (para debug/tunning)
 */
float pid_get_p_term(pid_controller_t *pid);

/**
 * Retorna o termo integral atual (para debug/tunning)
 */
float pid_get_i_term(pid_controller_t *pid);

/**
 * Retorna o termo derivativo atual (para debug/tunning)
 */
float pid_get_d_term(pid_controller_t *pid);

#endif // PID_CONTROLLER_H
