#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"

// Configurações do Wi-Fi
#define WIFI_SSID "ITSelf"
#define WIFI_PASSWORD "code2020"

// IP do servidor UDP
#define IP_SERVER "192.168.1.204"
#define PORT_SERVER 34567

// Pinos do joystick
#define ADC_PIN_X 26 // GP26 -> ADC0
#define ADC_PIN_Y 27 // GP27 -> ADC1

// Protótipo das funções
const char* obterDirecao(uint16_t x, uint16_t y);
void enviaUDP(struct udp_pcb *pcb, const char *msg);

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Iniciando...\n");

    // Inicializar ADC
    adc_init();
    adc_gpio_init(ADC_PIN_X);
    adc_gpio_init(ADC_PIN_Y);

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

    // Inicializar UDP - PCB (Protocol Control Block)
    struct udp_pcb *pcb = udp_new();
    if (!pcb) {
        printf("Erro ao criar PCB UDP\n");
        return -1;
    }

    while (true) {
        // Ler eixo X
        adc_select_input(0);
        uint16_t x = adc_read();

        // Ler eixo Y
        adc_select_input(1);
        uint16_t y = adc_read();

        // Obter direção
        const char *direction = obterDirecao(x, y);

        // Criar mensagem
        char msg[100];
        snprintf(msg, sizeof(msg), "X:%d,Y:%d,Direcao:%s", x, y, direction);
        printf("Enviando: %s\n", msg);

        // Enviar via UDP
        enviaUDP(pcb, msg);

        sleep_ms(500);
    }

    cyw43_arch_deinit();
    return 0;
}

// Área das funções

// Função para obter direção na rosa dos ventos
const char* obterDirecao(uint16_t x, uint16_t y) {
    const uint16_t centro = 2048;
    const uint16_t zonaNeutra = 500;

    int dx = x - centro;
    int dy = y - centro;

    if (abs(dx) < zonaNeutra && abs(dy) < zonaNeutra) {
        return "Centro";
    }

    if (dy > zonaNeutra) {
        if (dx > zonaNeutra) {
            return "Nordeste";
        }
        else if (dx < -zonaNeutra) {
            return "Noroeste";
        }
        else {
            return "Norte";
        }
    } else if (dy < -zonaNeutra) {
        if (dx > zonaNeutra) {
            return "Sudeste";
        }
        else if (dx < -zonaNeutra) 
        {
            return "Sudoeste";
        }
        else {
            return "Sul";
        }
    } else {
        if (dx > zonaNeutra) {
            return "Leste";
        }
        else if (dx < -zonaNeutra) {
        return "Oeste";
        }
    }

    return "Centro";
}

// Função para enviar dados via UDP
void enviaUDP(struct udp_pcb *pcb, const char *msg) {
    struct pbuf *pBuffer = pbuf_alloc(PBUF_TRANSPORT, strlen(msg), PBUF_RAM);

    if (!pBuffer) {
        return;
    }

    memcpy(pBuffer->payload, msg, strlen(msg));

    ip_addr_t ipDestino;
    ipaddr_aton(IP_SERVER, &ipDestino);

    udp_sendto(pcb, pBuffer, &ipDestino, PORT_SERVER);
    pbuf_free(pBuffer);
}
