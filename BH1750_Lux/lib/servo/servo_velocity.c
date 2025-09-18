#include "servo_velocity.h"

#define PULSE_STOP 1500   // µs que faz o servo parar
#define PULSE_MAX 1900    // µs velocidade máxima
#define PULSE_MIN 1100    // µs velocidade máxima na outra direção
#define LOOP_MS 20        // atualização 50Hz (~20ms)

static void servo_set_velocity(servo_velocity_t *servo, float vel) {
    if (vel > 1.0f) vel = 1.0f;
    if (vel < -1.0f) vel = -1.0f;

    uint16_t pulse = PULSE_STOP + (uint16_t)(vel * (PULSE_MAX - PULSE_STOP));
    if (pulse < PULSE_MIN) pulse = PULSE_MIN;
    if (pulse > PULSE_MAX) pulse = PULSE_MAX;

    pwm_set_chan_level(servo->slice, servo->channel, pulse);
}

void servo_init(servo_velocity_t *servo, uint gpio, float Kp) {
    servo->gpio = gpio;
    servo->Kp = Kp;
    servo->target_angle = 0;
    servo->current_angle = 0;

    gpio_set_function(gpio, GPIO_FUNC_PWM);
    servo->slice = pwm_gpio_to_slice_num(gpio);
    servo->channel = pwm_gpio_to_channel(gpio);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 125.0f);   // 1 tick = 1µs
    pwm_config_set_wrap(&config, 20000 - 1);  // 20ms = 50Hz
    pwm_init(servo->slice, &config, true);
}

void servo_set_target_angle(servo_velocity_t *servo, float angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    servo->target_angle = angle;
}

void servo_update(servo_velocity_t *servo) {
    float error = servo->target_angle - servo->current_angle;
    float velocity = servo->Kp * error;

    if (velocity > 1.0f) velocity = 1.0f;
    if (velocity < -1.0f) velocity = -1.0f;

    servo_set_velocity(servo, velocity);

    // Atualiza posição estimada
    servo->current_angle += velocity * 5.0f;  // constante de escala
    if (servo->current_angle > 180) servo->current_angle = 180;
    if (servo->current_angle < 0) servo->current_angle = 0;
}
