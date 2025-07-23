/* -------------------------------------------------------------------------------------------------------------------------------------
/ Projeto: Botão e Temperatura MQTT
/ Descrição: Este código lê a temperatura do sensor e o estado de um botão, enviando os dados para um broker MQTT.
/ Bibliotecas: pico-sdk, lwIP, CYW43
/ Autor: Felipe Teles
/ Obs: Necessário efetuar ajustes nos arquivos CMakeLists.txt e lwipopts.h para compilar e funcionar corretamente.
/ Data de Criação: 22/06/2025
/----------------------------------------------------------------------------------------------------------------------------------------
*/
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"
#include "lwip/dns.h"

// Configurações Wi-Fi
#define WIFI_SSID "ITSelf"
#define WIFI_PASSWORD "code2020"

// Configurações MQTT
#define MQTT_BROKER "broker.hivemq.com"
#define MQTT_BROKER_PORT 1883
#define MQTT_TOPIC "embarca/status"

// Configurações do Botão
#define BUTTON_GPIO 5

// Variáveis Globais
static mqtt_client_t *mqtt_client;
static ip_addr_t broker_ip;
static bool mqtt_connected = false;

// Protótipo das Funções
static void mqtt_connection_callback(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
void publish_msg(bool button_pressed, float temp_c);
float read_temperature();
void dns_check_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg);

// Função Principal
int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("\n=== Iniciando MQTT Button + Temperature ===\n");

    // Inicializa Wi-Fis
    if (cyw43_arch_init()) {
        printf("Erro na inicialização do Wi-Fi\n");
        return -1;
    }
    cyw43_arch_enable_sta_mode();

    printf("[Wi-Fi] Conectando...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("[Wi-Fi] Falha na conexão Wi-Fi\n");
        return -1;
    } else {
        printf("[Wi-Fi] Conectado com sucesso!\n");
    }

    // Configura GPIO do botão
    gpio_init(BUTTON_GPIO);
    gpio_set_dir(BUTTON_GPIO, GPIO_IN);
    gpio_pull_up(BUTTON_GPIO); // Pull-up ativado

    // Inicializa ADC para temperatura
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    // Inicializa cliente MQTT
    mqtt_client = mqtt_client_new();

    // Resolve DNS do broker MQTT
    err_t err = dns_gethostbyname(MQTT_BROKER, &broker_ip, dns_check_callback, NULL);
    if (err == ERR_OK) {
        dns_check_callback(MQTT_BROKER, &broker_ip, NULL);
    } else if (err == ERR_INPROGRESS) {
        printf("[DNS] Resolvendo...\n");
    } else {
        printf("[DNS] Erro ao resolver DNS: %d\n", err);
        return -1;
    }

    // Loop principal
    while (true) {
        // Atualiza tarefas de rede
        cyw43_arch_poll();

        // Lê botão
        bool button_state = !gpio_get(BUTTON_GPIO); // Inversão por pull-up

        // Lê temperatura
        float temp_c = read_temperature();
        printf("[TEMP] Temperatura atual: %.2f °C\n", temp_c);

        // Publica ambos no mesmo tópico
        publish_msg(button_state, temp_c);

        // Espera 1 segundo
        sleep_ms(1000);
    }

    cyw43_arch_deinit();
    return 0;
}

// Callback de conexão MQTT
static void mqtt_connection_callback(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        printf("[MQTT] Conectado ao broker!\n");
        mqtt_connected = true;
    } else {
        printf("[MQTT] Falha na conexão MQTT. Código: %d\n", status);
        mqtt_connected = false;
    }
}

// Publicar botão + temperatura
void publish_msg(bool button_pressed, float temp_c) {
    if (!mqtt_connected) {
        printf("[MQTT] Não conectado, não publicando dados\n");
        return;
    }

    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"botao\":\"%s\",\"temperatura\":%.2f}",
             button_pressed ? "ON" : "OFF",
             temp_c);

    printf("[MQTT] Publicando: tópico='%s', mensagem='%s'\n", MQTT_TOPIC, payload);

    err_t err = mqtt_publish(mqtt_client, MQTT_TOPIC, payload, strlen(payload), 0, 0, NULL, NULL);
    if (err == ERR_OK) {
        printf("[MQTT] Publicação OK\n");
    } else {
        printf("[MQTT] Erro ao publicar: %d\n", err);
    }
}

// Leitura da temperatura
float read_temperature() {
    uint16_t raw = adc_read();
    const float conversion_factor = 3.3f / (1 << 12);
    float voltage = raw * conversion_factor;
    float temp_c = 27.0f - (voltage - 0.706f) / 0.001721f;
    return temp_c;
}

// Callback de DNS
void dns_check_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    if (ipaddr != NULL) {
        broker_ip = *ipaddr;
        printf("[DNS] Resolvido: %s -> %s\n", name, ipaddr_ntoa(ipaddr));

        struct mqtt_connect_client_info_t client_info = {
            .client_id = "pico-client",
            .keep_alive = 60,
            .client_user = NULL,
            .client_pass = NULL,
            .will_topic = NULL,
            .will_msg = NULL,
            .will_qos = 0,
            .will_retain = 0
        };

        printf("[MQTT] Conectando ao broker...\n");
        mqtt_client_connect(mqtt_client, &broker_ip, MQTT_BROKER_PORT, mqtt_connection_callback, NULL, &client_info);
    } else {
        printf("[DNS] Falha ao resolver DNS para %s\n", name);
    }
}

