/* -------------------------------------------------------------------------------------------------------------------------------------
/ Projeto: Sensor AHT10 com OLED SSD1306
/ Descrição: Este código lê a temperatura e umidade do sensor AHT10 e exibe os dados em um display OLED SSD1306.
/ Bibliotecas: aht10, ssd1306
/ Autor: Felipe Teles do Nascimento
/ Data de Criação: 12/09/2025
/----------------------------------------------------------------------------------------------------------------------------------------
*/
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "aht10.h"
#include "ssd1306.h"

// I2C usado: I2C0 com SDA=GPIO4, SCL=GPIO5
#define I2C_PORT0 i2c0
#define I2C_SDA0 0
#define I2C_SCL0 1

#define I2C_PORT1 i2c1
#define I2C_SDA1 14
#define I2C_SCL1 15

// Prototipos das funções I2C
int i2c_write(uint8_t addr, const uint8_t *data, uint16_t len);
int i2c_read(uint8_t addr, uint8_t *data, uint16_t len);
void delay_ms(uint32_t ms);

int main() {
    stdio_init_all();

    // Inicializa I2C sensor
    i2c_init(I2C_PORT0, 100 * 1000); // 100 kHz
    gpio_set_function(I2C_SDA0, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL0, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA0);
    gpio_pull_up(I2C_SCL0);

    // Inicializa I2C OLED
    i2c_init(I2C_PORT1, 400000);
    gpio_set_function(I2C_SDA1, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL1, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA1);
    gpio_pull_up(I2C_SCL1);

    ssd1306_init(I2C_PORT1);
    ssd1306_clear();
    ssd1306_draw_string(32, 0, "Embarcatech");
    ssd1306_draw_string(20, 10, "Inicializando...");
    ssd1306_show();

    // Define estrutura do sensor
    AHT10_Handle aht10 = {
        .iface = {
            .i2c_write = i2c_write,
            .i2c_read = i2c_read,
            .delay_ms = delay_ms
        }
    };

    printf("Inicializando AHT10...\n");
    if (!AHT10_Init(&aht10)) {
        printf("Falha na inicialização do sensor!\n");
        ssd1306_clear();
        ssd1306_draw_string(32, 0, "Embarcatech");
        ssd1306_draw_string(23, 30, "Falha no AHT10");
        ssd1306_show();
        while (1) sleep_ms(1000);
    }

    while (1) {
        float temp, hum;
        if (AHT10_ReadTemperatureHumidity(&aht10, &temp, &hum)) {
            printf("Temperatura: %.2f °C | Umidade: %.2f %%\n", temp, hum);
        } else {
            printf("Falha na leitura dos dados!\n");
        }
            while(hum > 70){
                ssd1306_clear();
                ssd1306_draw_string(32, 0, "Embarcatech");
                ssd1306_draw_string(30, 10, "AHT10 Sensor");
                ssd1306_draw_string(0, 20, "Umidade");
                char hum_str[16];
                snprintf(hum_str, sizeof(hum_str), "%.2f %%", hum);
                ssd1306_draw_string(85, 20, hum_str);
                ssd1306_draw_string(22,40, "Acima de 70 %");
                ssd1306_draw_string(40,50, "ATENCAO");
                ssd1306_show();
                sleep_ms(500);
                ssd1306_draw_string(40,50, "       ");
                ssd1306_show();
                sleep_ms(500);
                AHT10_ReadTemperatureHumidity(&aht10, &temp, &hum);
            }

        while(temp < 20){
                ssd1306_clear();
                ssd1306_draw_string(32, 0, "Embarcatech");
                ssd1306_draw_string(30, 10, "AHT10 Sensor");
                ssd1306_draw_string(0, 20, "Temperatura");
                char hum_str[16];
                snprintf(hum_str, sizeof(hum_str), "%.2f C", temp);
                ssd1306_draw_string(85, 20, hum_str);
                ssd1306_draw_string(20,40, "Abaixo de 20 C");
                ssd1306_draw_string(40,50, "ATENCAO");
                ssd1306_show();
                sleep_ms(500);
                ssd1306_draw_string(40,50, "       ");
                ssd1306_show();
                sleep_ms(500);
                AHT10_ReadTemperatureHumidity(&aht10, &temp, &hum);
            }

        ssd1306_clear();
        ssd1306_draw_string(32, 0, "Embarcatech");
        ssd1306_draw_string(30, 10, "AHT10 Sensor");
        ssd1306_draw_string(0, 30, "Temperatura");
        char temp_str[16];
        snprintf(temp_str, sizeof(temp_str), "%.2f C", temp);
        ssd1306_draw_string(85, 30, temp_str);
        ssd1306_draw_string(0, 50, "Umidade");
        char hum_str[16];
        snprintf(hum_str, sizeof(hum_str), "%.2f %%", hum);
        ssd1306_draw_string(85, 50, hum_str);
        ssd1306_show();

        sleep_ms(1000);
    }
}

// Função para escrita I2C
int i2c_write(uint8_t addr, const uint8_t *data, uint16_t len) {
    int result = i2c_write_blocking(I2C_PORT0, addr, data, len, false);
    return result < 0 ? -1 : 0;
}

// Função para leitura I2C
int i2c_read(uint8_t addr, uint8_t *data, uint16_t len) {
    int result = i2c_read_blocking(I2C_PORT0, addr, data, len, false);
    return result < 0 ? -1 : 0;
}

// Função para delay
void delay_ms(uint32_t ms) {
    sleep_ms(ms);
}

