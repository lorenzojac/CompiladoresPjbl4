/*
 * scripts/bench/harness.c - medidor dos nucleos da DSL vetorial (Parte C).
 *
 * Os nucleos sao gerados pelo compilador a partir de scripts/bench/nucleos.mac
 * (ver bench.py) e rebaixados por scripts/pipeline.sh em cada configuracao.
 * Este programa so prepara os dados, chama cada nucleo REPETICOES vezes e
 * imprime "nucleo,segundos_por_chamada,resultado" (o resultado serve para
 * comparar numericamente as configuracoes).
 *
 * ABI (bare pointer): cada vetor e um ponteiro; escalares por valor.
 * Ordem dos nucleos em nucleos.mac:
 *   1: c receba a + b         (flut, elemento a elemento)
 *   2: somavet(ia)            (inteira, reducao)
 *   3: somavet(a)             (flut, reducao)
 *   4: prodesc(a, b)          (flut, produto escalar)
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef N
#define N 1000000
#endif

void zl_nucleo_1(float *a, float *b, float *c);
int32_t zl_nucleo_2(int32_t *ia);
float zl_nucleo_3(float *a);
float zl_nucleo_4(float *a, float *b);

static double agora(void)
{
    struct timespec t;

    timespec_get(&t, TIME_UTC);
    return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}

int main(int argc, char **argv)
{
    int repeticoes = argc > 1 ? atoi(argv[1]) : 10;
    float *a = malloc(sizeof(float) * N);
    float *b = malloc(sizeof(float) * N);
    float *c = malloc(sizeof(float) * N);
    int32_t *ia = malloc(sizeof(int32_t) * N);
    volatile double sorvedouro = 0;
    double inicio;
    int r;
    long i;

    if (a == NULL || b == NULL || c == NULL || ia == NULL) {
        fputs("sem memoria\n", stderr);
        return 1;
    }
    /* Dados deterministicos, com magnitudes variadas (realca o efeito da
     * ordem das somas em ponto flutuante). */
    for (i = 0; i < N; i++) {
        a[i] = (float)((i * 7919L) % 1000) / 7.0f + (i % 3 == 0 ? 1e4f : 0.0f);
        b[i] = (float)((i * 104729L) % 997) / 13.0f;
        ia[i] = (int32_t)((i * 31L) % 2001) - 1000;
    }

    zl_nucleo_1(a, b, c);
    inicio = agora();
    for (r = 0; r < repeticoes; r++) {
        zl_nucleo_1(a, b, c);
        sorvedouro += c[r % N];
    }
    printf("soma_elemento,%.9f,%.6f\n", (agora() - inicio) / repeticoes,
           (double)c[N / 2]);

    {
        int32_t resultado = zl_nucleo_2(ia);

        inicio = agora();
        for (r = 0; r < repeticoes; r++) {
            resultado = zl_nucleo_2(ia);
            sorvedouro += resultado;
        }
        printf("somavet_inteira,%.9f,%ld\n", (agora() - inicio) / repeticoes,
               (long)resultado);
    }
    {
        float resultado = zl_nucleo_3(a);

        inicio = agora();
        for (r = 0; r < repeticoes; r++) {
            resultado = zl_nucleo_3(a);
            sorvedouro += resultado;
        }
        printf("somavet_flut,%.9f,%.6f\n", (agora() - inicio) / repeticoes,
               (double)resultado);
    }
    {
        float resultado = zl_nucleo_4(a, b);

        inicio = agora();
        for (r = 0; r < repeticoes; r++) {
            resultado = zl_nucleo_4(a, b);
            sorvedouro += resultado;
        }
        printf("prodesc_flut,%.9f,%.6f\n", (agora() - inicio) / repeticoes,
               (double)resultado);
    }
    (void)sorvedouro;
    free(a);
    free(b);
    free(c);
    free(ia);
    return 0;
}
