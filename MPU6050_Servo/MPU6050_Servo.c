/* -------------------------------------------------------------------------------------------------------------------------------------
/ Projeto: MPU6050_SERVO
/ Descrição: Este projeto utiliza um sensor de movimento MPU6050 para controlar a posição de um servo contínuo simulado, 
/ exibindo informações em um display OLED SSD1306. O servo é calibrado automaticamente no boot se um botão for pressionado, 
/ e o tempo de rotação é salvo na memória flash para uso futuro. A posição do servo é determinada pela inclinação do sensor: 
/ inclinação para frente move o servo para 0 graus, inclinação para trás para 90 graus, e inclinação lateral para 180 graus. 
/ Os valores dos ângulos são aproximados pois o servo não guarda a posição.
/ Bibliotecas: pico-sdk, extras (servo_sim, flash_storage, ssd1306, mpu6050).
/ Autor: José Adriano
/ Data de Criação: 10/09/2025
/----------------------------------------------------------------------------------------------------------------------------------------
*/
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "servo_sim.h"
#include "flash_storage.h"
#include "ssd1306.h"
#include "mpu6050_i2c.h"

// ==== Pinos ====
#define SERVO_PIN    2     // GPIO do servo contínuo (simulado)
#define BTN_CALIB    5     // Botão de calibração (ativo em nível baixo)
#define BUZZER1_PIN  10    // Buzzer 1
#define BUZZER2_PIN  27    // Buzzer 2

// I2C OLED (i2c1)
#define I2C_PORT_OLED i2c1
#define SDA_OLED 14
#define SCL_OLED 15

// ==== Configurações ====
#define ALERT_THRESHOLD 90.0f   // Ângulo limite para alerta

int main() {
    stdio_init_all();

    // === Botão de calibração ===
    gpio_init(BTN_CALIB);
    gpio_set_dir(BTN_CALIB, GPIO_IN);
    gpio_pull_up(BTN_CALIB);

    // === Buzzers ===
    gpio_init(BUZZER1_PIN);
    gpio_set_dir(BUZZER1_PIN, GPIO_OUT);
    gpio_put(BUZZER1_PIN, 0);

    gpio_init(BUZZER2_PIN);
    gpio_set_dir(BUZZER2_PIN, GPIO_OUT);
    gpio_put(BUZZER2_PIN, 0);

    // === Inicializa MPU6050 (I2C0) ===
    mpu6050_setup_i2c();
    mpu6050_reset();
    sleep_ms(200);

    if (!mpu6050_test()) {
        printf("MPU6050 nao encontrado!\n");
        while (1) sleep_ms(1000);
    }

    // === Inicializa OLED (I2C1) ===
    i2c_init(I2C_PORT_OLED, 400000);
    gpio_set_function(SDA_OLED, GPIO_FUNC_I2C);
    gpio_set_function(SCL_OLED, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_OLED);
    gpio_pull_up(SCL_OLED);
    ssd1306_init(I2C_PORT_OLED);

    // Tela inicial
    ssd1306_clear();
    ssd1306_draw_string(20, 0, "Servo MPU6050");
    ssd1306_draw_string(8, 12, "Inicializando...");
    ssd1306_show();
    sleep_ms(1000);

    // === Servo: carrega calibração ===
    uint32_t rotation_time_ms = 1000;
    bool have_calib = flash_storage_read(&rotation_time_ms);

    servo_sim_t servo;
    servo_sim_init(&servo, SERVO_PIN, (float)rotation_time_ms);

    if (!gpio_get(BTN_CALIB)) {
        ssd1306_clear();
        ssd1306_draw_string(20, 24, "Calibrando...");
        ssd1306_show();

        servo_sim_calibrate(&servo);
        rotation_time_ms = (uint32_t)(180.0f / servo.deg_per_ms);
        flash_storage_write(rotation_time_ms);

        ssd1306_clear();
        ssd1306_draw_string(8, 24, "Calibracao salva!");
        ssd1306_show();
        sleep_ms(1200);
    } else if (have_calib) {
        char msg[24];
        snprintf(msg, sizeof(msg), "Calib: %ums", rotation_time_ms);
        ssd1306_clear();
        ssd1306_draw_string(10, 24, msg);
        ssd1306_show();
        sleep_ms(800);
    }
        
    float current_angle = 90.0f;  // posição inicial
    bool alert_active = false;
    int frame = 0;
    bool use_buzzer1 = true;

    char alert_msg[32];
    snprintf(alert_msg, sizeof(alert_msg), "!!! ALERTA >%.0f !!!", ALERT_THRESHOLD);

    while (true) {
        int16_t accel[3], gyro[3], temp_raw;
        mpu6050_read_raw(accel, gyro, &temp_raw);

        // Escolha de ângulo: simples → aceleração no eixo X
        float ax = accel[0] / ACCEL_SENS_2G;
        float target_angle = (ax < -0.5f) ? 0.0f : (ax < 0.5f ? 90.0f : 180.0f);

        // Movimento suave
        if (current_angle < target_angle) current_angle += 2.0f;
        else if (current_angle > target_angle) current_angle -= 2.0f;

        // Atualiza flag do alerta
        alert_active = (current_angle > ALERT_THRESHOLD);

        // Debug serial
        printf(">");
        printf("AX=%.2fg AY=%.2fg AZ=%.2fg | GX=%d GY=%d GZ=%d | Alvo=%.0f deg | Atual=%.0f deg\n",
               accel[0]/ACCEL_SENS_2G, accel[1]/ACCEL_SENS_2G, accel[2]/ACCEL_SENS_2G,
               gyro[0], gyro[1], gyro[2], target_angle, current_angle);

        // Servo simulado
        servo_sim_set_angle(&servo, current_angle);

        // === Display ===
        ssd1306_clear();
        ssd1306_draw_string(20, 0, "Servo MPU6050");

        char line1[24], line2[24];
        snprintf(line1, sizeof(line1), "AX: %.2fg", accel[0]/ACCEL_SENS_2G);
        snprintf(line2, sizeof(line2), "Ang: %.0f/%0.f", current_angle, target_angle);

        ssd1306_draw_string(6, 20, line1);
        ssd1306_draw_string(6, 36, line2);

        // Alerta piscante + beep alternado
        if (alert_active && (frame % 2 == 0)) {
            ssd1306_draw_string(6, 52, alert_msg);

            if (use_buzzer1) {
                gpio_put(BUZZER1_PIN, 1);
                sleep_ms(100);
                gpio_put(BUZZER1_PIN, 0);
            } else {
                gpio_put(BUZZER2_PIN, 1);
                sleep_ms(100);
                gpio_put(BUZZER2_PIN, 0);
            }
            use_buzzer1 = !use_buzzer1; // alterna buzzer
        }

        ssd1306_show();
        frame++;

        sleep_ms(200);
    }
}
