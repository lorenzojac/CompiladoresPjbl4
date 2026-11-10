/*
 * zl_runtime_posix.c - biblioteca de execucao para PC (PjBL 3).
 *
 * Funciona em Linux, macOS e Windows (MinGW/UCRT): usa apenas a biblioteca
 * padrao do C, mais Sleep/nanosleep para espera().
 *
 * Toda a saida, inclusive a mensagem de erro de execucao, vai para stdout,
 * caractere a caractere com putchar. Dois motivos:
 *   - a saida do PC fica identica a da placa, onde tudo sai pela serial
 *     (requisito da Parte D: "saida identica a do PC");
 *   - no Windows, dentro do lli, fwrite(stdout) usa a estrutura FILE de
 *     outro CRT e a saida se perde; putchar/getchar funcionam em todo lugar.
 * Erro de execucao termina o processo com ZL_SAIDA_ERRO_EXECUCAO (3).
 *
 * As funcoes de pino nao tem hardware no PC: sao simuladas (validam os
 * argumentos, guardam o nivel e registram a operacao entre colchetes) para
 * que os programas da placa tambem possam ser testados aqui.
 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 199309L
#endif

#include "zl_plataforma.h"
#include "zl_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

/*
 * A parte comum entra na mesma unidade de compilacao: assim o comando do
 * enunciado funciona com um unico arquivo de biblioteca,
 *   clang saida.ll runtime/zl_runtime_posix.c -o prog
 * e o lli recebe a biblioteca inteira em um modulo (ver README).
 */
#include "zl_runtime_comum.c"

void zl_plataforma_escreve(const char *bytes, size_t tamanho)
{
    size_t i;

    for (i = 0; i < tamanho; i++) {
        putchar((unsigned char)bytes[i]);
    }
}

static void escreve_texto(const char *texto)
{
    zl_plataforma_escreve(texto, strlen(texto));
}

int zl_plataforma_le_caractere(void)
{
    int c = getchar();

    return c == EOF ? -1 : c;
}

const char *zl_plataforma_guarda_texto(const char *texto, size_t tamanho)
{
    char *copia = malloc(tamanho + 1);

    if (copia == NULL) {
        escreve_texto("Erro de execucao: memoria insuficiente\n");
        fflush(stdout);
        exit(ZL_SAIDA_ERRO_EXECUCAO);
    }
    memcpy(copia, texto, tamanho);
    copia[tamanho] = '\0';
    return copia;
}

void zl_plataforma_termina_com_erro(int32_t codigo)
{
    (void)codigo;
    fflush(stdout);
    exit(ZL_SAIDA_ERRO_EXECUCAO);
}

/* Nivel simulado dos pinos 0-19. */
static int32_t niveis[20];

static void escreve_numero(int32_t valor)
{
    zl_imprime_inteira(valor);
}

void zl_pino_modo(int32_t pino, int32_t modo, int32_t linha)
{
    if (!zl_pino_valido(pino) || modo < ZL_PINO_ENTRADA ||
        modo > ZL_PINO_ENTRADA_PULLUP) {
        zl_erro_execucao(ZL_ERRO_PINO, linha);
    }
    if (modo == ZL_PINO_ENTRADA_PULLUP) {
        niveis[pino] = 1;
    }
    escreve_texto("[pino ");
    escreve_numero(pino);
    escreve_texto(modo == ZL_PINO_SAIDA ? ": saida]\n"
                  : modo == ZL_PINO_ENTRADA ? ": entrada]\n"
                                            : ": entrada com pull-up]\n");
}

void zl_pino_escreve(int32_t pino, int32_t valor, int32_t linha)
{
    if (!zl_pino_valido(pino)) {
        zl_erro_execucao(ZL_ERRO_PINO, linha);
    }
    niveis[pino] = valor != 0;
    escreve_texto("[pino ");
    escreve_numero(pino);
    escreve_texto(valor != 0 ? " <- 1]\n" : " <- 0]\n");
}

int32_t zl_pino_le(int32_t pino, int32_t linha)
{
    if (!zl_pino_valido(pino)) {
        zl_erro_execucao(ZL_ERRO_PINO, linha);
    }
    return niveis[pino];
}

void zl_espera(int32_t milissegundos)
{
    if (milissegundos <= 0) {
        return;
    }
    fflush(stdout);
#ifdef _WIN32
    Sleep((DWORD)milissegundos);
#else
    {
        struct timespec pausa;

        pausa.tv_sec = milissegundos / 1000;
        pausa.tv_nsec = (long)(milissegundos % 1000) * 1000000L;
        nanosleep(&pausa, NULL);
    }
#endif
}

/*
 * Com -DZL_SEM_MAIN a biblioteca vira uma biblioteca compartilhada sem
 * ponto de entrada, para o executor JIT do MLIR chamar @zl_principal
 * diretamente (scripts/jit.sh).
 */
#ifndef ZL_SEM_MAIN
int main(void)
{
    int32_t resultado = zl_principal();

    fflush(stdout);
    return (int)resultado;
}
#else
/* Chamada pelo invólucro do JIT depois de @zl_principal (scripts/jit.sh). */
void zl_jit_termina(int32_t resultado);

void zl_jit_termina(int32_t resultado)
{
    fflush(stdout);
    exit((int)resultado);
}
#endif
