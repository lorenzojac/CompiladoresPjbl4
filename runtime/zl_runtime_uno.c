/*
 * zl_runtime_uno.c - biblioteca de execucao para o Arduino Uno (PjBL 4, D).
 *
 * ATmega328P a 16 MHz, sem sistema operacional. Compilar com avr-gcc:
 *   -mmcu=atmega328p -DF_CPU=16000000UL -mdouble=64
 * (-mdouble=64 da ao duplocarpado o mesmo double IEEE de 64 bits do PC.)
 *
 * - Saida e entrada pela USART0 (pinos 0/1, a mesma porta USB do Arduino),
 *   9600 baud, 8N1. O Monitor Serial da IDE do Arduino, o avrdude/terminal
 *   ou o QEMU (-serial stdio) mostram o que o programa imprime.
 * - Erro de execucao: a mensagem vai pela serial e o LED do pino 13 pisca
 *   rapido para sempre (sinal fisico exigido pelo enunciado).
 * - Fim normal de zl_principal: o LED fica aceso fixo e o processador para.
 * - Pinos: numeracao da propria placa (2-13 digitais, 14-19 = A0-A5);
 *   0 e 1 sao reservados a serial.
 *
 * Com -DZL_SENTINELA (usado pelos testes no QEMU) o fim do programa e
 * marcado com o byte 0x04 (EOT) na serial.
 */
#include "zl_plataforma.h"
#include "zl_runtime.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include <string.h>

/* Parte comum na mesma unidade: basta ligar este arquivo ao programa. */
#include "zl_runtime_comum.c"

#define ZL_BAUD 9600UL
#define ZL_UBRR ((F_CPU / (16UL * ZL_BAUD)) - 1UL)

/* LED embutido: pino 13 = PB5. */
#define ZL_LED_DDR DDRB
#define ZL_LED_PORT PORTB
#define ZL_LED_BIT 5

static void serial_inicia(void)
{
    UBRR0H = (uint8_t)(ZL_UBRR >> 8);
    UBRR0L = (uint8_t)ZL_UBRR;
    UCSR0A = 0;
    UCSR0B = (uint8_t)((1 << RXEN0) | (1 << TXEN0));
    UCSR0C = (uint8_t)((1 << UCSZ01) | (1 << UCSZ00));
}

static void serial_envia(char c)
{
    while (!(UCSR0A & (1 << UDRE0))) {
    }
    UDR0 = (uint8_t)c;
}

static void serial_espera_envio(void)
{
    while (!(UCSR0A & (1 << UDRE0))) {
    }
}

void zl_plataforma_escreve(const char *bytes, size_t tamanho)
{
    size_t i;

    for (i = 0; i < tamanho; i++) {
        serial_envia(bytes[i]);
    }
}

int zl_plataforma_le_caractere(void)
{
    while (!(UCSR0A & (1 << RXC0))) {
    }
    return (int)UDR0;
}

/* Textos lidos ficam em 4 buffers rotativos de 32 bytes (RAM de 2 KB). */
const char *zl_plataforma_guarda_texto(const char *texto, size_t tamanho)
{
    static char buffers[4][32];
    static uint8_t proximo;
    char *destino = buffers[proximo];

    proximo = (uint8_t)((proximo + 1u) % 4u);
    if (tamanho > sizeof(buffers[0]) - 1) {
        tamanho = sizeof(buffers[0]) - 1;
    }
    memcpy(destino, texto, tamanho);
    destino[tamanho] = '\0';
    return destino;
}

static void para_processador(void)
{
    cli();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    for (;;) {
        sleep_cpu();
    }
}

void zl_plataforma_termina_com_erro(int32_t codigo)
{
    (void)codigo;
#ifdef ZL_SENTINELA
    serial_envia('\x04');
#endif
    serial_espera_envio();
    ZL_LED_DDR |= (uint8_t)(1 << ZL_LED_BIT);
    for (;;) {
        ZL_LED_PORT ^= (uint8_t)(1 << ZL_LED_BIT);
        _delay_ms(100);
    }
}

/*
 * Numeracao Arduino -> registradores do ATmega328P:
 *   2-7 -> PORTD2..7, 8-13 -> PORTB0..5, 14-19 (A0-A5) -> PORTC0..5.
 */
typedef struct {
    volatile uint8_t *ddr;
    volatile uint8_t *port;
    volatile uint8_t *pin;
    uint8_t bit;
} Pino;

static Pino pino_da_placa(int32_t numero)
{
    Pino pino;

    if (numero < 8) {
        pino.ddr = &DDRD;
        pino.port = &PORTD;
        pino.pin = &PIND;
        pino.bit = (uint8_t)numero;
    } else if (numero < 14) {
        pino.ddr = &DDRB;
        pino.port = &PORTB;
        pino.pin = &PINB;
        pino.bit = (uint8_t)(numero - 8);
    } else {
        pino.ddr = &DDRC;
        pino.port = &PORTC;
        pino.pin = &PINC;
        pino.bit = (uint8_t)(numero - 14);
    }
    return pino;
}

void zl_pino_modo(int32_t numero, int32_t modo, int32_t linha)
{
    Pino pino;
    uint8_t mascara;

    if (!zl_pino_valido(numero) || modo < ZL_PINO_ENTRADA ||
        modo > ZL_PINO_ENTRADA_PULLUP) {
        zl_erro_execucao(ZL_ERRO_PINO, linha);
    }
    pino = pino_da_placa(numero);
    mascara = (uint8_t)(1u << pino.bit);
    if (modo == ZL_PINO_SAIDA) {
        *pino.ddr |= mascara;
    } else {
        *pino.ddr &= (uint8_t)~mascara;
        if (modo == ZL_PINO_ENTRADA_PULLUP) {
            *pino.port |= mascara;
        } else {
            *pino.port &= (uint8_t)~mascara;
        }
    }
}

void zl_pino_escreve(int32_t numero, int32_t valor, int32_t linha)
{
    Pino pino;
    uint8_t mascara;

    if (!zl_pino_valido(numero)) {
        zl_erro_execucao(ZL_ERRO_PINO, linha);
    }
    pino = pino_da_placa(numero);
    mascara = (uint8_t)(1u << pino.bit);
    if (valor != 0) {
        *pino.port |= mascara;
    } else {
        *pino.port &= (uint8_t)~mascara;
    }
}

int32_t zl_pino_le(int32_t numero, int32_t linha)
{
    Pino pino;

    if (!zl_pino_valido(numero)) {
        zl_erro_execucao(ZL_ERRO_PINO, linha);
    }
    pino = pino_da_placa(numero);
    return (*pino.pin >> pino.bit) & 1u;
}

void zl_espera(int32_t milissegundos)
{
    while (milissegundos-- > 0) {
        _delay_ms(1);
    }
}

int main(void)
{
    serial_inicia();
    (void)zl_principal();
#ifdef ZL_SENTINELA
    serial_envia('\x04');
#endif
    serial_espera_envio();
    ZL_LED_DDR |= (uint8_t)(1 << ZL_LED_BIT);
    ZL_LED_PORT |= (uint8_t)(1 << ZL_LED_BIT);
    para_processador();
    return 0;
}
