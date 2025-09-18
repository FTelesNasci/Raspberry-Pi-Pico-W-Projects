#include "servo_sim.h"
#include <math.h>

// Configuração do PWM: 50 Hz, 1 tick = 1us
void servo_sim_init(servo_sim_t *servo, uint gpio, float rotation_time_ms) {
    servo->gpio = gpio;
    servo->stop_pulse_us = 1500.0f;
    servo->speed_scale = 500.0f; // ±500us
    servo->current_angle = 90.0f; // posição inicial “meio”
    servo->deg_per_ms = 180.0f / rotation_time_ms;

    gpio_set_function(gpio, GPIO_FUNC_PWM);
    servo->slice = pwm_gpio_to_slice_num(gpio);
    servo->channel = pwm_gpio_to_channel(gpio);

    float clk_div = 125.0f; // 125MHz / 125 = 1MHz → 1us por tick
    servo->top = 20000;     // 20ms = 50Hz

    pwm_set_clkdiv(servo->slice, clk_div);
    pwm_set_wrap(servo->slice, servo->top);
    pwm_set_enabled(servo->slice, true);
}

static void servo_set_speed(servo_sim_t *servo, float speed) {
    if (speed > 1.0f) speed = 1.0f;
    if (speed < -1.0f) speed = -1.0f;

    float pulse_us = servo->stop_pulse_us + speed * servo->speed_scale;
    if (pulse_us < 1000.0f) pulse_us = 1000.0f;
    if (pulse_us > 2000.0f) pulse_us = 2000.0f;

    uint16_t level = (uint16_t)((pulse_us / 20000.0f) * servo->top);
    pwm_set_chan_level(servo->slice, servo->channel, level);
}

void servo_sim_set_angle(servo_sim_t *servo, float target_angle) {
    if (target_angle < 0) target_angle = 0;
    if (target_angle > 180) target_angle = 180;

    float delta = target_angle - servo->current_angle;

    if (fabsf(delta) < 0.5f) {  // deadband
        servo_set_speed(servo, 0);
        servo->current_angle = target_angle;
        return;
    }

    float speed = (delta > 0) ? 1.0f : -1.0f;
    float time_ms = fabsf(delta) / servo->deg_per_ms;

    servo_set_speed(servo, speed);
    sleep_ms((uint32_t)(time_ms + 0.5f));
    servo_set_speed(servo, 0);

    servo->current_angle = target_angle;
}

void servo_sim_calibrate(servo_sim_t *servo) {
    // Vai “para 0°” (giro negativo por 500ms para encostar)
    servo_set_speed(servo, -1.0f);
    sleep_ms(500);
    servo_set_speed(servo, 0);
    servo->current_angle = 0.0f;
    sleep_ms(300);

    // Mede tempo até “180°”
    absolute_time_t start = get_absolute_time();
    servo_set_speed(servo, 1.0f);
    // Limite superior de segurança: 2s (evita travar caso não mova)
    sleep_ms(1200);
    servo_set_speed(servo, 0);
    absolute_time_t end = get_absolute_time();

    int64_t elapsed_ms = absolute_time_diff_us(start, end) / 1000;
    if (elapsed_ms < 500)  elapsed_ms = 500;
    if (elapsed_ms > 2000) elapsed_ms = 2000;

    servo->deg_per_ms = 180.0f / (float)elapsed_ms;
    servo->current_angle = 180.0f;
}
