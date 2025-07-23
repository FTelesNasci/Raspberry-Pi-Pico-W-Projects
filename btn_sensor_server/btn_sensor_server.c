/* -------------------------------------------------------------------------------------------------------------------------------------
/ Projeto: Sensor de Botão e Temperatura com Wi-Fi
/ Descrição: Este código lê a temperatura do sensor e o estado de um botão, enviando os dados para um servidor.
/ Hardware: Raspberry Pi Pico W
/ Bibliotecas: pico-sdk, lwIP, CYW43
/ Autor: Felipe Teles do Nascimento
/ Data de Criação: 15/06/2025
/----------------------------------------------------------------------------------------------------------------------------------------
*/
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"

#define WIFI_SSID "ITSelf"       // Nome da rede Wi-Fi
#define WIFI_PASSWORD "code2020"  // Senha da rede Wi-Fi

#define SERVER_IP "192.168.1.204"    // IP do servidor para onde enviar os dados
#define SERVER_PORT 34567            // Porta do servidor

#define BUTTON_PIN 5                 // GPIO do botão
#define ADC_TEMP 4                   // Canal ADC do sensor de temperatura interno

//Protótipos de funções
void mostra_ip();                                       // Função para exibir o IP da placa
const char* le_botao();                                // Função para ler o estado do botão
float le_temperatura();                                 // Função para ler a temperatura
void envia_udp(struct udp_pcb *pcb, const char *msg);   // Função para enviar dados via UDP

// Função principal
int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Iniciando...\n");

    // Configurar GPIO do botão
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // Inicializar ADC
    adc_init();
    adc_set_temp_sensor_enabled(true);

    // Inicializar Wi-Fi
    if (cyw43_arch_init()) {
        printf("Erro ao inicializar Wi-Fi\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Conectando ao Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Falha na conexão Wi-Fi\n");
        return -1;
    }

    printf("Wi-Fi conectado!\n");

    // Exibir IP da placa
    mostra_ip();

    // Inicializar UDP
    struct udp_pcb *pcb = udp_new();
    if (!pcb) {
        printf("Erro ao criar PCB UDP\n");
        return -1;
        
    }

    while (true) {
        const char* button_state = le_botao();
        float temperature = le_temperatura();

        // Criar mensagem com os dados
        char msg[100];
        snprintf(msg, sizeof(msg), "Botao: %s,Temperatura: %.2f Celsius", button_state, temperature);
        printf("Enviando: %s\n", msg);

        // Enviar via UDP
        envia_udp(pcb, msg);

        sleep_ms(1000);
    }

    cyw43_arch_deinit();
    return 0;
}

// Função para obter o IP da placa
void mostra_ip() {
    struct netif *netif = netif_default;
    if (netif) {
        printf("Endereço IP da placa: %s\n", ipaddr_ntoa(&netif->ip_addr));
    } else {
        printf("Erro ao obter o IP\n");
    }
}

// Função para ler o estado do botão
const char* le_botao() {
    return gpio_get(BUTTON_PIN) ? "LIBERADO" : "PRESSIONADO";  // Invertido devido ao pull-up interno
}

// Função para ler a temperatura
float le_temperatura() {
    adc_select_input(ADC_TEMP);
    uint16_t raw = adc_read();
    float voltage = raw * 3.3f / (1 << 12);
    return 27.0 - (voltage - 0.706) / 0.001721;
}

// Função para enviar dados via UDP
void envia_udp(struct udp_pcb *pcb, const char *msg) {
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, strlen(msg), PBUF_RAM);
    if (!p) return;
    memcpy(p->payload, msg, strlen(msg));

    ip_addr_t dest_ip;
    ipaddr_aton(SERVER_IP, &dest_ip);

    udp_sendto(pcb, p, &dest_ip, SERVER_PORT);
    pbuf_free(p);
}