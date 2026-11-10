#ifndef MACARRONICA_STRBUF_H
#define MACARRONICA_STRBUF_H

/*
 * Buffer de texto que cresce sob demanda. Os geradores de codigo montam o
 * LLVM IR e o MLIR em memoria e so escrevem o arquivo de saida no fim, para
 * que um programa invalido nunca deixe arquivo parcial (PjBL 3, Secao 4.4).
 *
 * Falha de alocacao e "pegajosa": depois dela todas as operacoes viram no-op
 * e strbuf_failed devolve true; o chamador verifica uma unica vez no fim.
 */

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
    bool failed;
} StrBuf;

void strbuf_append(StrBuf *buffer, const char *text);
void strbuf_append_bytes(StrBuf *buffer, const char *bytes, size_t length);
/* No MinGW o arquetipo "printf" e o do MSVCRT antigo (sem %zu). */
#if defined(__MINGW32__) && !defined(__clang__)
#define STRBUF_PRINTF_FORMAT __attribute__((format(gnu_printf, 2, 3)))
#elif defined(__GNUC__) || defined(__clang__)
#define STRBUF_PRINTF_FORMAT __attribute__((format(printf, 2, 3)))
#else
#define STRBUF_PRINTF_FORMAT
#endif

void strbuf_appendf(StrBuf *buffer, const char *format, ...)
    STRBUF_PRINTF_FORMAT;
void strbuf_append_buffer(StrBuf *buffer, const StrBuf *other);
const char *strbuf_text(const StrBuf *buffer);
bool strbuf_failed(const StrBuf *buffer);
void strbuf_clear(StrBuf *buffer);
void strbuf_destroy(StrBuf *buffer);

#endif
