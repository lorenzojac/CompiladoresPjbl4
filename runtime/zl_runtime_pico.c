/*
 * zl_runtime_pico.c - biblioteca de execucao para a Raspberry Pi Pico 2 W
 * (PjBL 4, Parte D; D48).
 *
 * RP2350 (ARM Cortex-M33 a 150 MHz, 520 KB de SRAM, 4 MB de flash), sem
 * sistema operacional, com o Pico SDK 2.x. O programa .ll do compilador
 * vira um objeto ARM (llc) e e ligado a este arquivo pelo CMake do SDK
 * (runtime/pico/CMakeLists.txt).
 *
 * - Saida e entrada pela USB da placa (CDC, aparece como porta COM no
 *   Windows e /dev/ttyACM0 no Linux). Sem conversor USB-serial: e a propria
 *   RP2350 que fala USB, entao nao precisa de driver.
 * - O programa so comeca quando um terminal abre a porta (sinal DTR), para
 *   nenhuma saida se perder; enquanto espera, o LED pisca devagar. Ligada
 *   num carregador (sem PC), a placa roda o programa direto.
 * - Fechar o terminal reinicia a placa: abrir de novo roda o programa de
 *   novo (como o Arduino Uno, que reinicia ao abrir o Monitor Serial).
 * - Erro de execucao: a mensagem sai pela USB e o LED pisca rapido.
 * - Fim normal de zl_principal: o LED fica aceso fixo.
 * - LED embutido: na Pico 2 W ele e ligado ao chip sem fio CYW43439, nao a
 *   um GPIO; na linguagem ele e o pino 25 (o pino do LED na Pico sem fio).
 * - Pinos: GP0-GP22 e GP26-GP28. GP23, GP24 e GP29 sao do chip sem fio.
 * - Abrir a porta a 1200 baud reinicia a placa no modo BOOTSEL (recurso do
 *   SDK), o que permite gravar um novo programa sem apertar o botao.
 *
 * Com -DZL_SENTINELA (testes automaticos na placa) o fim do programa e
 * marcado com o byte 0x04 (EOT).
 */
#include "zl_plataforma.h"
#include "zl_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "pico/cyw43_arch.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"
#include "tusb.h"

/* Parte comum na mesma unidade: basta ligar este arquivo ao programa. */
#include "zl_runtime_comum.c"

#define ZL_PINO_LED 25

/* Tempo para um PC enumerar a placa antes de assumir "sem PC". */
#define ZL_ESPERA_USB_MS 3000

static bool led_disponivel;
static bool led_aceso;
static bool com_terminal;

static void led_poe(bool aceso)
{
    led_aceso = aceso;
    if (led_disponivel) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, aceso);
    }
}

static void reinicia_placa(void)
{
    watchdog_reboot(0, 0, 0);
    for (;;) {
        tight_loop_contents();
    }
}

/* O terminal foi fechado: a placa reinicia e espera o proximo. */
static void verifica_terminal(void)
{
    if (com_terminal && !stdio_usb_connected()) {
        reinicia_placa();
    }
}

void zl_plataforma_escreve(const char *bytes, size_t tamanho)
{
    size_t i;

    /* putchar do SDK converte '\n' em "\r\n" para os terminais. */
    for (i = 0; i < tamanho; i++) {
        putchar((unsigned char)bytes[i]);
    }
}

int zl_plataforma_le_caractere(void)
{
    if (!com_terminal) {
        return -1; /* sem PC nao ha de onde ler: fim da entrada */
    }
    for (;;) {
        int c = getchar_timeout_us(100000);

        if (c != PICO_ERROR_TIMEOUT) {
            return c;
        }
        verifica_terminal();
    }
}

const char *zl_plataforma_guarda_texto(const char *texto, size_t tamanho)
{
    char *copia = malloc(tamanho + 1);

    if (copia == NULL) {
        escreve_fixo("Erro de execucao: memoria insuficiente\n");
        zl_plataforma_termina_com_erro(ZL_SAIDA_ERRO_EXECUCAO);
    }
    memcpy(copia, texto, tamanho);
    copia[tamanho] = '\0';
    return copia;
}

/* Fim do programa: sentinela, LED fixo (ok) ou piscando (erro). */
static void __attribute__((noreturn)) termina(bool erro)
{
#ifdef ZL_SENTINELA
    putchar_raw('\x04');
#endif
    stdio_flush();
    led_poe(true);
    for (;;) {
        sleep_ms(100);
        if (erro) {
            led_poe(!led_aceso);
        }
        verifica_terminal();
    }
}

void zl_plataforma_termina_com_erro(int32_t codigo)
{
    (void)codigo;
    termina(true);
}

int zl_pino_valido(int32_t pino)
{
    return (pino >= 0 && pino <= 22) || (pino >= 25 && pino <= 28);
}

void zl_pino_modo(int32_t pino, int32_t modo, int32_t linha)
{
    if (!zl_pino_valido(pino) || modo < ZL_PINO_ENTRADA ||
        modo > ZL_PINO_ENTRADA_PULLUP) {
        zl_erro_execucao(ZL_ERRO_PINO, linha);
    }
    if (pino == ZL_PINO_LED) {
        return; /* o LED e sempre saida */
    }
    gpio_init((uint)pino);
    if (modo == ZL_PINO_SAIDA) {
        gpio_set_dir((uint)pino, GPIO_OUT);
    } else {
        gpio_set_dir((uint)pino, GPIO_IN);
        if (modo == ZL_PINO_ENTRADA_PULLUP) {
            gpio_pull_up((uint)pino);
        } else {
            gpio_disable_pulls((uint)pino);
        }
    }
}

void zl_pino_escreve(int32_t pino, int32_t valor, int32_t linha)
{
    if (!zl_pino_valido(pino)) {
        zl_erro_execucao(ZL_ERRO_PINO, linha);
    }
    if (pino == ZL_PINO_LED) {
        led_poe(valor != 0);
    } else {
        gpio_put((uint)pino, valor != 0);
    }
}

int32_t zl_pino_le(int32_t pino, int32_t linha)
{
    if (!zl_pino_valido(pino)) {
        zl_erro_execucao(ZL_ERRO_PINO, linha);
    }
    if (pino == ZL_PINO_LED) {
        return led_aceso ? 1 : 0;
    }
    return gpio_get((uint)pino) ? 1 : 0;
}

void zl_espera(int32_t milissegundos)
{
    stdio_flush();
    while (milissegundos > 0) {
        int32_t passo = milissegundos > 20 ? 20 : milissegundos;

        sleep_ms((uint32_t)passo);
        milissegundos -= passo;
        verifica_terminal();
    }
}

/*
 * Espera um terminal abrir a porta USB, piscando o LED devagar. Se nenhum
 * PC enumerar a placa em ZL_ESPERA_USB_MS (carregador ou bateria), roda
 * sem terminal.
 */
static void espera_terminal(void)
{
    uint32_t passos = 0;

    for (;;) {
        if (stdio_usb_connected()) {
            com_terminal = true;
            break;
        }
        if (!tud_mounted() && passos * 50u >= ZL_ESPERA_USB_MS) {
            break;
        }
        if (passos % 10u == 0) {
            led_poe(!led_aceso);
        }
        sleep_ms(50);
        passos++;
    }
    led_poe(false);
}

int main(void)
{
    stdio_init_all();
    led_disponivel = cyw43_arch_init() == 0;
    espera_terminal();
    (void)zl_principal();
    termina(false);
    return 0;
}
