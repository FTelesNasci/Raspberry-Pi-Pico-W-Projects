#ifndef SERVO_VELOCITY_H
#define SERVO_VELOCITY_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"

typedef struct {
    uint gpio;        // GPIO conectado ao servo
    uint slice;       // slice PWM
    uint channel;     // canal PWM
    float target_angle; // ângulo alvo estimado
    float current_angle; // posição estimada
    float Kp;         // ganho proporcional da velocidade
} servo_velocity_t;

// Inicializa o servo no GPIO especificado
void servo_init(servo_velocity_t *servo, uint gpio, float Kp);

// Define o ângulo alvo (0 a 180)
void servo_set_target_angle(servo_velocity_t *servo, float angle);

// Atualiza o servo (chamar dentro do loop)
void servo_update(servo_velocity_t *servo);

#endif
