/*
 * zl_runtime_comum.c - parte da biblioteca de execucao igual em todo alvo.
 *
 * Toda a formatacao e feita aqui, sem printf, com aritmetica de double
 * IEEE-754 de 64 bits. No PC o double e nativo; no Arduino Uno o projeto
 * compila com -mdouble=64 (avr-gcc 10+ com avr-libc 2.2+), entao as mesmas
 * operacoes produzem os mesmos digitos e a saida da placa e identica a do
 * PC (PjBL 4, Parte D).
 *
 * Textos fixos da biblioteca (mensagens de erro, VER/FAL, nan...) usam
 * FIXO("..."): no AVR ficam na flash (PROGMEM) e sao lidos com
 * pgm_read_byte, economizando ~200 bytes dos 2 KB de RAM; no PC, FIXO e o
 * proprio texto. E uma otimizacao dependente do alvo feita so na biblioteca:
 * o IR gerado pelo compilador nao muda.
 */
#include "zl_plataforma.h"
#include "zl_runtime.h"

#include <stdlib.h>
#include <string.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#define FIXO(texto) PSTR(texto)
#define LE_FIXO(p) ((char)pgm_read_byte(p))
#define COMPARA_FIXO(a, b) strcmp_P((a), (b))
#else
#define FIXO(texto) (texto)
#define LE_FIXO(p) (*(p))
#define COMPARA_FIXO(a, b) strcmp((a), (b))
#endif

#define ZL_CASAS_FLUT 4
#define ZL_CASAS_DUPLO 8
#define ZL_TAMANHO_TOKEN 64

/* Escreve um texto fixo da biblioteca (flash no AVR). */
static void escreve_fixo(const char *texto)
{
    char c;

    while ((c = LE_FIXO(texto)) != '\0') {
        zl_plataforma_escreve(&c, 1);
        texto++;
    }
}

/* Escreve o inteiro sem sinal "valor" em decimal. */
static void escreve_natural(uint64_t valor)
{
    char digitos[24];
    int n = 0;

    do {
        digitos[n++] = (char)('0' + (int)(valor % 10u));
        valor /= 10u;
    } while (valor != 0);
    while (n > 0) {
        char c = digitos[--n];

        zl_plataforma_escreve(&c, 1);
    }
}

void zl_imprime_inteira(int32_t valor)
{
    if (valor < 0) {
        escreve_fixo(FIXO("-"));
        /* -INT32_MIN nao cabe em int32: calcula em 64 bits. */
        escreve_natural((uint64_t)(-(int64_t)valor));
    } else {
        escreve_natural((uint64_t)valor);
    }
}

/*
 * Escreve "valor" com "casas" casas decimais, arredondando a metade para
 * longe do zero (2.00005 -> 2.0001 com 4 casas). Valores grandes demais
 * para a forma fixa usam notacao cientifica (1.2345e+20). NaN e infinito
 * so aparecem por estouro (divisao por zero e raiz de negativo sao erros).
 */
static void escreve_real(double valor, int casas)
{
    uint64_t potencia = 1;
    double escala = 1.0;
    double absoluto;
    int i;

    if (valor != valor) {
        escreve_fixo(FIXO("nan"));
        return;
    }
    if (valor - valor != 0.0) {
        escreve_fixo(valor > 0 ? FIXO("inf") : FIXO("-inf"));
        return;
    }
    for (i = 0; i < casas; i++) {
        potencia *= 10u;
        escala *= 10.0;
    }
    absoluto = valor < 0 ? -valor : valor;

    if (absoluto * escala < 9.0e15) {
        uint64_t escalado = (uint64_t)(absoluto * escala + 0.5);
        uint64_t fracao = escalado % potencia;
        char digitos[24];

        if (valor < 0 && escalado != 0) {
            escreve_fixo(FIXO("-"));
        }
        escreve_natural(escalado / potencia);
        escreve_fixo(FIXO("."));
        for (i = casas - 1; i >= 0; i--) {
            digitos[i] = (char)('0' + (int)(fracao % 10u));
            fracao /= 10u;
        }
        zl_plataforma_escreve(digitos, (size_t)casas);
        return;
    }

    {
        int expoente = 0;

        while (absoluto >= 10.0) {
            absoluto /= 10.0;
            expoente++;
        }
        /* Arredondar pode levar a mantissa a 10.000...: renormaliza. */
        if ((uint64_t)(absoluto * escala + 0.5) >= 10u * potencia) {
            absoluto /= 10.0;
            expoente++;
        }
        escreve_real(valor < 0 ? -absoluto : absoluto, casas);
        escreve_fixo(FIXO("e+"));
        escreve_natural((uint64_t)expoente);
    }
}

void zl_imprime_flut(float valor)
{
    escreve_real((double)valor, ZL_CASAS_FLUT);
}

void zl_imprime_duplo(double valor)
{
    escreve_real(valor, ZL_CASAS_DUPLO);
}

void zl_imprime_bool(int32_t valor)
{
    escreve_fixo(valor ? FIXO("VER") : FIXO("FAL"));
}

/* Texto do programa: esta na RAM (literal do IR ou lido por le()). */
void zl_imprime_texto(const char *texto)
{
    if (texto != NULL) {
        zl_plataforma_escreve(texto, strlen(texto));
    }
}

void zl_imprime_nl(void)
{
    escreve_fixo(FIXO("\n"));
}

/* ------------------------------------------------------------------ */
/* Leitura                                                             */
/* ------------------------------------------------------------------ */

static int espaco(int c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/* Le o proximo token (sequencia sem espacos); 0 se a entrada acabou. */
static int le_token(char *token, size_t capacidade)
{
    size_t n = 0;
    int c;

    do {
        c = zl_plataforma_le_caractere();
    } while (c >= 0 && espaco(c));
    if (c < 0) {
        return 0;
    }
    while (c >= 0 && !espaco(c)) {
        if (n + 1 < capacidade) {
            token[n++] = (char)c;
        }
        c = zl_plataforma_le_caractere();
    }
    token[n] = '\0';
    return 1;
}

int32_t zl_le_inteira(int32_t linha)
{
    char token[ZL_TAMANHO_TOKEN];
    const char *p = token;
    int negativo = 0;
    int64_t valor = 0;

    if (!le_token(token, sizeof(token))) {
        zl_erro_execucao(ZL_ERRO_LEITURA, linha);
    }
    if (*p == '-' || *p == '+') {
        negativo = *p == '-';
        p++;
    }
    if (*p == '\0') {
        zl_erro_execucao(ZL_ERRO_LEITURA, linha);
    }
    for (; *p != '\0'; p++) {
        if (*p < '0' || *p > '9') {
            zl_erro_execucao(ZL_ERRO_LEITURA, linha);
        }
        valor = valor * 10 + (*p - '0');
        if (valor > 2147483648LL) {
            zl_erro_execucao(ZL_ERRO_LEITURA, linha);
        }
    }
    if (negativo) {
        valor = -valor;
    }
    if (valor > 2147483647LL) {
        zl_erro_execucao(ZL_ERRO_LEITURA, linha);
    }
    return (int32_t)valor;
}

static double le_real(int32_t linha)
{
    char token[ZL_TAMANHO_TOKEN];
    char *fim;
    double valor;

    if (!le_token(token, sizeof(token))) {
        zl_erro_execucao(ZL_ERRO_LEITURA, linha);
    }
    valor = strtod(token, &fim);
    if (fim == token || *fim != '\0') {
        zl_erro_execucao(ZL_ERRO_LEITURA, linha);
    }
    return valor;
}

float zl_le_flut(int32_t linha)
{
    return (float)le_real(linha);
}

double zl_le_duplo(int32_t linha)
{
    return le_real(linha);
}

int32_t zl_le_bool(int32_t linha)
{
    char token[ZL_TAMANHO_TOKEN];

    if (!le_token(token, sizeof(token))) {
        zl_erro_execucao(ZL_ERRO_LEITURA, linha);
    }
    if (COMPARA_FIXO(token, FIXO("VER")) == 0 ||
        COMPARA_FIXO(token, FIXO("1")) == 0) {
        return 1;
    }
    if (COMPARA_FIXO(token, FIXO("FAL")) == 0 ||
        COMPARA_FIXO(token, FIXO("0")) == 0) {
        return 0;
    }
    zl_erro_execucao(ZL_ERRO_LEITURA, linha);
}

/* palavra: le ate o fim da linha (linhas vazias iniciais sao puladas). */
const char *zl_le_texto(int32_t linha)
{
    char texto[ZL_TAMANHO_TOKEN];
    size_t n = 0;
    int c;

    do {
        c = zl_plataforma_le_caractere();
    } while (c == '\n' || c == '\r');
    if (c < 0) {
        zl_erro_execucao(ZL_ERRO_LEITURA, linha);
    }
    while (c >= 0 && c != '\n' && c != '\r') {
        if (n + 1 < sizeof(texto)) {
            texto[n++] = (char)c;
        }
        c = zl_plataforma_le_caractere();
    }
    texto[n] = '\0';
    return zl_plataforma_guarda_texto(texto, n);
}

/* ------------------------------------------------------------------ */
/* Operacoes de apoio                                                  */
/* ------------------------------------------------------------------ */

int32_t zl_texto_igual(const char *a, const char *b)
{
    return strcmp(a != NULL ? a : "", b != NULL ? b : "") == 0;
}

/*
 * inteira ^ inteira por quadrados sucessivos, com a mesma aritmetica em
 * complemento de dois de + - * (estouro da a volta, D43). Expoente
 * negativo nao tem resultado inteiro: erro de execucao.
 */
int32_t zl_potencia_inteira(int32_t base, int32_t expoente, int32_t linha)
{
    uint32_t resultado = 1u;
    uint32_t fator = (uint32_t)base;
    uint32_t e;

    if (expoente < 0) {
        zl_erro_execucao(ZL_ERRO_POTENCIA_NEGATIVA, linha);
    }
    e = (uint32_t)expoente;
    while (e != 0) {
        if (e & 1u) {
            resultado *= fator;
        }
        fator *= fator;
        e >>= 1;
    }
    return (int32_t)resultado;
}

static const char *mensagem_de_erro(int32_t codigo)
{
    switch (codigo) {
    case ZL_ERRO_INDICE:
        return FIXO("indice de vetor fora dos limites");
    case ZL_ERRO_DIVISAO_ZERO:
        return FIXO("divisao por zero");
    case ZL_ERRO_RAIZ_NEGATIVA:
        return FIXO("raiz de numero negativo");
    case ZL_ERRO_POTENCIA_NEGATIVA:
        return FIXO("expoente negativo em potencia de inteira");
    case ZL_ERRO_LEITURA:
        return FIXO("entrada invalida ou ausente");
    case ZL_ERRO_PINO:
        return FIXO("pino ou modo invalido");
    default:
        return FIXO("erro desconhecido");
    }
}

/*
 * "Erro de execucao (linha L): <motivo>" na saida do programa, e a
 * plataforma encerra (codigo 3 no PC; LED piscando na placa).
 */
void zl_erro_execucao(int32_t codigo, int32_t linha)
{
    escreve_fixo(FIXO("Erro de execucao (linha "));
    escreve_natural(linha > 0 ? (uint64_t)linha : 0u);
    escreve_fixo(FIXO("): "));
    escreve_fixo(mensagem_de_erro(codigo));
    escreve_fixo(FIXO("\n"));
    zl_plataforma_termina_com_erro(codigo);
}

int zl_pino_valido(int32_t pino)
{
    /* 0 e 1 sao RX/TX da serial usada por imprime(). */
    return pino >= 2 && pino <= 19;
}
