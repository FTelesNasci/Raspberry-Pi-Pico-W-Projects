#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "bh1750.h"
#include "servo_velocity.h"

// #define ANGLE_ALERT_THRESHOLD 60.0f // Ângulo limite para alerta
#define SERVO_MIN_ANGLE 0
#define SERVO_MAX_ANGLE 180

// ==== Configurações I2C ====
// BH1750 em i2c0
#define I2C_PORT_SENSOR i2c0
#define SDA_SENSOR 0
#define SCL_SENSOR 1

// ==== Configuração Servo ====
#define SERVO_PIN 2

int main() {
    stdio_init_all();

    // ---- Inicializa BH1750 ----
    i2c_init(I2C_PORT_SENSOR, 100 * 1000);
    gpio_set_function(SDA_SENSOR, GPIO_FUNC_I2C);
    gpio_set_function(SCL_SENSOR, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_SENSOR);
    gpio_pull_up(SCL_SENSOR);

    bh1750_init(I2C_PORT_SENSOR);
    sleep_ms(200); // tempo para estabilizar

    servo_velocity_t servo;
    servo_init(&servo, 2, 0.02f);  // GPIO2, ganho Kp=0.02

    uint32_t last_sensor_time = 0;
    uint32_t last_display_time = 0;

    while (true) {
        // Atualiza servo a cada ~20ms
        servo_update(&servo);
        sleep_ms(20);

        uint32_t now = to_ms_since_boot(get_absolute_time());

        // ---- Leitura do sensor (a cada 1s) ----
        if (now - last_sensor_time >= 1000) {
            last_sensor_time = now;
            float lux = bh1750_read_lux(I2C_PORT_SENSOR);
            printf("Luminosidade: %.2f lux\n", lux);

            // ---- Calcular ângulo proporcional ----
            float angle;
            if (lux <= 0) {
                angle = 0;
            } else if (lux >= 100) {
                angle = 180;
            } else {
                angle = (lux / 1000.0f) * 180.0f;
            }

            servo_set_target_angle(&servo, angle);
            printf("Ângulo alvo do servo: %.2f°\n", angle);
        }
    }
}