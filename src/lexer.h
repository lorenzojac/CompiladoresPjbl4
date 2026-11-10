#ifndef MACARRONICA_LEXER_H
#define MACARRONICA_LEXER_H

#include <stdbool.h>
#include <stddef.h>

#include "token.h"

/*
 * lexer_init exige que esta estrutura esteja zero-inicializada ou tenha sido
 * passada previamente a lexer_destroy. O lexer cria e passa a ser dono de
 * uma copia de source; o buffer original continua pertencendo ao chamador.
 */
typedef struct {
    char *source;
    size_t length;
    size_t current;
    size_t token_start;
    size_t line;
    size_t column;
    size_t token_line;
    size_t token_column;
    bool finished;
    bool failed;
} Lexer;

bool lexer_init(Lexer *lexer, const char *source, size_t length);
bool lexer_init_from_string(Lexer *lexer, const char *source);
void lexer_destroy(Lexer *lexer);

/*
 * out_token deve estar zerado ou ter sido passado a token_destroy. Retorna
 * true quando produz um token, inclusive TOK_INVALIDO e TOK_EOF. Retorna
 * false somente em argumento inválido ou falha interna, como falta de
 * memória; nesse caso, o lexer fica marcado como failed.
 */
bool lexer_next_token(Lexer *lexer, Token *out_token);

#endif
