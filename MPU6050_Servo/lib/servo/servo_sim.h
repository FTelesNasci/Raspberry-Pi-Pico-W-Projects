#ifndef SERVO_SIM_H
#define SERVO_SIM_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"

typedef struct {
    uint gpio;
    uint slice;
    uint channel;
    uint16_t top;
    float stop_pulse_us;    // pulso neutro (~1500us)
    float speed_scale;      // faixa de controle (±500us)
    float current_angle;    // posição simulada atual
    float deg_per_ms;       // graus por milissegundo (calibrado)
} servo_sim_t;

/**
 * Inicializa o servo contínuo (simulado), definindo o tempo para 180°
 * rotation_time_ms tempo (ms) medido/calibrado para ir de 0° a 180°
 */
void servo_sim_init(servo_sim_t *servo, uint gpio, float rotation_time_ms);

/**
 * Move até ângulo simulado (0..180) usando rotação por tempo
 */
void servo_sim_set_angle(servo_sim_t *servo, float target_angle);

/**
 * Calibração automática: mede tempo de 0° a 180° e atualiza deg_per_ms
 * (usa velocidade máxima; bloqueante por ~1.7s)
 */
void servo_sim_calibrate(servo_sim_t *servo);

#endif
