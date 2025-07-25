#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"
#include "lwip/dns.h"

// Wi-Fi
#define WIFI_SSID "ITSelf"
#define WIFI_PASSWORD "code2020"

// MQTT Broker público compatível com MQTTX
#define MQTT_BROKER "broker.emqx.io"
#define MQTT_BROKER_PORT 1883
#define MQTT_TOPIC "embarca/status"
#define BUTTON_GPIO 5

static mqtt_client_t *mqtt_client;
static ip_addr_t broker_ip;
static bool mqtt_connected = false;
static bool dns_resolved = false;

// Funções
static void mqtt_connection_callback(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
void publish_msg(bool button_pressed, float temp_c);
float read_temperature();
void dns_check_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg);

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("== Publicador MQTT - Pico W + MQTTX ==\n");

    if (cyw43_arch_init()) {
        printf("Erro no Wi-Fi\n");
        return -1;
    }
    cyw43_arch_enable_sta_mode();

    printf("[Wi-Fi] Conectando...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("[Wi-Fi] Falha\n");
        return -1;
    }
    printf("[Wi-Fi] Conectado!\n");

    gpio_init(BUTTON_GPIO);
    gpio_set_dir(BUTTON_GPIO, GPIO_IN);
    gpio_pull_up(BUTTON_GPIO);

    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    mqtt_client = mqtt_client_new();
    if (mqtt_client == NULL) {
        printf("[MQTT] Erro ao criar cliente\n");
        return -1;
    }

    err_t err = dns_gethostbyname(MQTT_BROKER, &broker_ip, dns_check_callback, NULL);
    if (err == ERR_OK) {
        dns_check_callback(MQTT_BROKER, &broker_ip, NULL);
    } else if (err == ERR_INPROGRESS) {
        printf("[DNS] Resolvendo %s...\n", MQTT_BROKER);
    } else {
        printf("[DNS] Erro: %d\n", err);
        return -1;
    }

    while (!dns_resolved || !mqtt_connected) {
        cyw43_arch_poll();
        sleep_ms(10);
    }

    while (true) {
        cyw43_arch_poll();

        bool button_state = !gpio_get(BUTTON_GPIO);
        float temp = read_temperature();

        publish_msg(button_state, temp);

        sleep_ms(1000);
    }

    cyw43_arch_deinit();
    return 0;
}

void publish_msg(bool button_pressed, float temp_c) {
    if (!mqtt_connected) return;

    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"botao\":\"%s\",\"temperatura\":%.2f}",
             button_pressed ? "ON" : "OFF", temp_c);

    mqtt_publish(mqtt_client, MQTT_TOPIC, payload, strlen(payload), 0, 0, NULL, NULL);
    printf("[MQTT] Enviado: %s\n", payload);
}

float read_temperature() {
    const int samples = 5;
    uint32_t total = 0;
    for (int i = 0; i < samples; i++) {
        total += adc_read();
        sleep_ms(2);
    }

    float avg = total / (float)samples;
    const float factor = 3.3f / (1 << 12);
    float voltage = avg * factor;
    float temp = 27.0f - (voltage - 0.706f) / 0.001721f;
    return temp;
}

void mqtt_connection_callback(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        printf("[MQTT] Conectado ao broker!\n");
        mqtt_connected = true;
    } else {
        printf("[MQTT] Falha (%d)\n", status);
        mqtt_connected = false;
    }
}

void dns_check_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    if (ipaddr) {
        broker_ip = *ipaddr;
        dns_resolved = true;
        printf("[DNS] %s -> %s\n", name, ipaddr_ntoa(ipaddr));

        struct mqtt_connect_client_info_t ci = {
            .client_id = "pico-w-client",
            .keep_alive = 60,
            .client_user = NULL,
            .client_pass = NULL,
            .will_topic = NULL,
            .will_msg = NULL,
            .will_qos = 0,
            .will_retain = 0
        };

        mqtt_client_connect(mqtt_client, &broker_ip, MQTT_BROKER_PORT, mqtt_connection_callback, NULL, &ci);
    } else {
        printf("[DNS] Falha para %s\n", name);
    }
}
