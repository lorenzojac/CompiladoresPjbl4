/*
 * tests/contador_alocacoes.c - verificacao de vazamento de memoria.
 *
 * Ligado ao compilador com
 *   -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free,--wrap=main
 * (ld do GNU, em Linux e MinGW), conta os blocos alocados pelo proprio
 * compilador. Ao fim de main imprime em stderr quantos blocos criados
 * DENTRO de main continuam vivos; tem de ser 0. Blocos que a biblioteca do C
 * aloca antes de main (argv, tabela do atexit) nao entram na conta.
 * Usado por tests/run_memoria.sh (make test-memoria). Sem AddressSanitizer,
 * que o MSYS2 nao traz; no Linux, -fsanitize=address da a mesma resposta.
 */
#include <stdio.h>
#include <stdlib.h>

void *__real_malloc(size_t tamanho);
void *__real_calloc(size_t quantidade, size_t tamanho);
void *__real_realloc(void *bloco, size_t tamanho);
void __real_free(void *bloco);
int __real_main(int argc, char **argv);

void *__wrap_malloc(size_t tamanho);
void *__wrap_calloc(size_t quantidade, size_t tamanho);
void *__wrap_realloc(void *bloco, size_t tamanho);
void __wrap_free(void *bloco);
int __wrap_main(int argc, char **argv);

static long vivos;

void *__wrap_malloc(size_t tamanho)
{
    void *bloco = __real_malloc(tamanho);

    if (bloco != NULL) {
        vivos++;
    }
    return bloco;
}

void *__wrap_calloc(size_t quantidade, size_t tamanho)
{
    void *bloco = __real_calloc(quantidade, tamanho);

    if (bloco != NULL) {
        vivos++;
    }
    return bloco;
}

void *__wrap_realloc(void *bloco, size_t tamanho)
{
    void *novo = __real_realloc(bloco, tamanho);

    if (novo != NULL && bloco == NULL) {
        vivos++;
    }
    return novo;
}

void __wrap_free(void *bloco)
{
    if (bloco != NULL) {
        vivos--;
    }
    __real_free(bloco);
}

int __wrap_main(int argc, char **argv)
{
    long antes = vivos;
    int resultado = __real_main(argc, argv);

    fprintf(stderr, "[memoria] blocos vivos criados em main: %ld\n",
            vivos - antes);
    return resultado;
}
