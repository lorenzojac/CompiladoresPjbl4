#ifndef ZL_PLATAFORMA_H
#define ZL_PLATAFORMA_H

/*
 * Ganchos que cada plataforma implementa para zl_runtime_comum.c.
 * Nao faz parte da interface usada pelo codigo gerado (essa e zl_runtime.h).
 */

#include <stddef.h>
#include <stdint.h>

/* Escreve bytes na saida do programa (stdout no PC, serial na placa). */
void zl_plataforma_escreve(const char *bytes, size_t tamanho);

/* Proximo caractere da entrada, ou -1 no fim da entrada. */
int zl_plataforma_le_caractere(void);

/* Guarda uma copia de um texto lido por le() e devolve o ponteiro. */
const char *zl_plataforma_guarda_texto(const char *texto, size_t tamanho);

/*
 * Encerra o programa depois de um erro de execucao; a mensagem ja foi
 * escrita por zl_erro_execucao (nao retorna).
 */
void zl_plataforma_termina_com_erro(int32_t codigo)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((noreturn))
#endif
    ;

/*
 * 1 se o pino existe e pode ser usado pelo programa, na numeracao da
 * propria placa: GP0-GP22, GP26-GP28 e 25 = LED na Pico 2 W (tambem
 * simulada no PC); 2-19 no Arduino Uno (D44, D48).
 */
int zl_pino_valido(int32_t pino);

#endif
