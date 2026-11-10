#ifndef MACARRONICA_TOKEN_H
#define MACARRONICA_TOKEN_H

#include <stddef.h>

typedef enum {
    TOK_ID,
    TOK_NUM_INTEIRO,
    TOK_NUM_DECIMAL,
    TOK_TEXTO,

    TOK_VER,
    TOK_FAL,

    TOK_RECEBA,
    TOK_REPETE,
    TOK_DURANTE,
    TOK_COND,
    TOK_CASOCONTRARIO,
    TOK_FUNC,
    TOK_BOOL,
    TOK_INTEIRA,
    TOK_FLUT,
    TOK_PALAVRA,
    TOK_DUPLOCARPADO,
    TOK_PRINCIPAL,
    TOK_VAZIO,
    TOK_RESPOST,

    TOK_REGISTRO,
    TOK_OUTRAFUNCAO,
    TOK_VET,
    TOK_EDAI,
    TOK_RAIZ,
    TOK_LE,
    TOK_IMPRIME,

    TOK_OU,
    TOK_E,
    TOK_NAO,
    TOK_XOR,

    TOK_MAIS,
    TOK_MENOS,
    TOK_MULT,
    TOK_DIV,
    TOK_DIV_INT,
    TOK_POTENCIA,

    TOK_MENOR,
    TOK_MAIOR,
    TOK_IGUAL_IGUAL,
    TOK_DIFERENTE,

    TOK_ABRE_COLCHETE,
    TOK_FECHA_COLCHETE,
    TOK_ABRE_CHAVE,
    TOK_FECHA_CHAVE,
    TOK_ABRE_PAREN,
    TOK_FECHA_PAREN,
    TOK_PONTO_VIRGULA,
    TOK_VIRGULA,
    TOK_ARROBA,
    TOK_PONTO,

    TOK_EOF,
    TOK_INVALIDO
} TokenType;

typedef struct {
    TokenType type;
    char *lexeme;
    size_t lexeme_length;
    size_t line;
    size_t column;
    const char *error_message;
} Token;

const char *token_type_name(TokenType type);
void token_destroy(Token *token);

#endif
