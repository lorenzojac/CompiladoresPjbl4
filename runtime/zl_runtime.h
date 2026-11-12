#ifndef ZL_RUNTIME_H
#define ZL_RUNTIME_H

/*
 * Biblioteca de execucao da Macaronica: a unica ponte entre o codigo gerado
 * e a plataforma (PjBL 3, Secao 4.5). Todas as funcoes tem assinatura fixa
 * (nada variadico) e tipos de largura fixa; bool atravessa a fronteira como
 * int32_t (0 ou 1) para que a ABI seja a mesma em x86-64, ARM e AVR.
 *
 * Implementacoes:
 *   zl_runtime_comum.c  formatacao e leitura, iguais em todo alvo (saida do
 *                       PC identica a da placa);
 *   zl_runtime_posix.c  PC (Linux, macOS, Windows): stdio, main();
 *   zl_runtime_pico.c   Raspberry Pi Pico 2 W (placa principal, D48): USB,
 *                       LED no pino 25 (chip CYW43439);
 *   zl_runtime_uno.c    Arduino Uno (2a arquitetura): USART a 9600 baud,
 *                       LED no pino 13.
 */

#include <stdint.h>

/* Codigos de erro de execucao (zl_erro_execucao). */
#define ZL_ERRO_PARIDADE 1          /* reservado (Zolang) */
#define ZL_ERRO_INDICE 2            /* indice de vetor fora dos limites */
#define ZL_ERRO_DIVISAO_ZERO 3      /* divisor zero em / ou // */
#define ZL_ERRO_RAIZ_NEGATIVA 4     /* raiz() de valor negativo */
#define ZL_ERRO_POTENCIA_NEGATIVA 5 /* inteira ^ expoente negativo */
#define ZL_ERRO_LEITURA 6           /* entrada invalida ou ausente em le() */
#define ZL_ERRO_PINO 7              /* pino ou modo invalido */

/* Codigo de saida do processo no PC quando ocorre erro de execucao. */
#define ZL_SAIDA_ERRO_EXECUCAO 3

/* Ponto de entrada gerado pelo compilador (funcao principal). */
int32_t zl_principal(void);

/* Saida: cada imprime() gera o valor e depois zl_imprime_nl(). */
void zl_imprime_inteira(int32_t valor);
void zl_imprime_flut(float valor);      /* 4 casas decimais */
void zl_imprime_duplo(double valor);    /* 8 casas decimais */
void zl_imprime_bool(int32_t valor);    /* VER ou FAL */
void zl_imprime_texto(const char *texto);
void zl_imprime_nl(void);

/* Entrada: um valor por chamada, separado por espacos ou quebras de linha. */
int32_t zl_le_inteira(int32_t linha);
float zl_le_flut(int32_t linha);
double zl_le_duplo(int32_t linha);
int32_t zl_le_bool(int32_t linha);      /* VER/FAL ou 1/0 */
const char *zl_le_texto(int32_t linha); /* uma linha inteira */

/* Operacoes que precisam de codigo de apoio. */
int32_t zl_texto_igual(const char *a, const char *b);
int32_t zl_potencia_inteira(int32_t base, int32_t expoente, int32_t linha);

/* Erro de execucao: mensagem com a linha do fonte; nao retorna. */
void zl_erro_execucao(int32_t codigo, int32_t linha)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((noreturn))
#endif
    ;

/*
 * E/S fisica (D44, D48). Pinos na numeracao da propria placa: Pico 2 W
 * GP0-GP22 e GP26-GP28, 25 = LED (o PC simula esses); Uno 2-13 e
 * A0-A5 = 14-19. Cada plataforma valida com zl_pino_valido.
 */
#define ZL_PINO_ENTRADA 0
#define ZL_PINO_SAIDA 1
#define ZL_PINO_ENTRADA_PULLUP 2
void zl_pino_modo(int32_t pino, int32_t modo, int32_t linha);
void zl_pino_escreve(int32_t pino, int32_t valor, int32_t linha);
int32_t zl_pino_le(int32_t pino, int32_t linha);
void zl_espera(int32_t milissegundos);

#endif
