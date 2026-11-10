/* token.c - nomes legiveis dos tipos de token e liberacao de lexemas. */
#include "token.h"

#include <stdlib.h>

const char *token_type_name(TokenType type)
{
    switch (type) {
    case TOK_ID: return "TOK_ID";
    case TOK_NUM_INTEIRO: return "TOK_NUM_INTEIRO";
    case TOK_NUM_DECIMAL: return "TOK_NUM_DECIMAL";
    case TOK_TEXTO: return "TOK_TEXTO";
    case TOK_VER: return "TOK_VER";
    case TOK_FAL: return "TOK_FAL";
    case TOK_RECEBA: return "TOK_RECEBA";
    case TOK_REPETE: return "TOK_REPETE";
    case TOK_DURANTE: return "TOK_DURANTE";
    case TOK_COND: return "TOK_COND";
    case TOK_CASOCONTRARIO: return "TOK_CASOCONTRARIO";
    case TOK_FUNC: return "TOK_FUNC";
    case TOK_BOOL: return "TOK_BOOL";
    case TOK_INTEIRA: return "TOK_INTEIRA";
    case TOK_FLUT: return "TOK_FLUT";
    case TOK_PALAVRA: return "TOK_PALAVRA";
    case TOK_DUPLOCARPADO: return "TOK_DUPLOCARPADO";
    case TOK_PRINCIPAL: return "TOK_PRINCIPAL";
    case TOK_VAZIO: return "TOK_VAZIO";
    case TOK_RESPOST: return "TOK_RESPOST";
    case TOK_REGISTRO: return "TOK_REGISTRO";
    case TOK_OUTRAFUNCAO: return "TOK_OUTRAFUNCAO";
    case TOK_VET: return "TOK_VET";
    case TOK_EDAI: return "TOK_EDAI";
    case TOK_RAIZ: return "TOK_RAIZ";
    case TOK_LE: return "TOK_LE";
    case TOK_IMPRIME: return "TOK_IMPRIME";
    case TOK_OU: return "TOK_OU";
    case TOK_E: return "TOK_E";
    case TOK_NAO: return "TOK_NAO";
    case TOK_XOR: return "TOK_XOR";
    case TOK_MAIS: return "TOK_MAIS";
    case TOK_MENOS: return "TOK_MENOS";
    case TOK_MULT: return "TOK_MULT";
    case TOK_DIV: return "TOK_DIV";
    case TOK_DIV_INT: return "TOK_DIV_INT";
    case TOK_POTENCIA: return "TOK_POTENCIA";
    case TOK_MENOR: return "TOK_MENOR";
    case TOK_MAIOR: return "TOK_MAIOR";
    case TOK_IGUAL_IGUAL: return "TOK_IGUAL_IGUAL";
    case TOK_DIFERENTE: return "TOK_DIFERENTE";
    case TOK_ABRE_COLCHETE: return "TOK_ABRE_COLCHETE";
    case TOK_FECHA_COLCHETE: return "TOK_FECHA_COLCHETE";
    case TOK_ABRE_CHAVE: return "TOK_ABRE_CHAVE";
    case TOK_FECHA_CHAVE: return "TOK_FECHA_CHAVE";
    case TOK_ABRE_PAREN: return "TOK_ABRE_PAREN";
    case TOK_FECHA_PAREN: return "TOK_FECHA_PAREN";
    case TOK_PONTO_VIRGULA: return "TOK_PONTO_VIRGULA";
    case TOK_VIRGULA: return "TOK_VIRGULA";
    case TOK_ARROBA: return "TOK_ARROBA";
    case TOK_PONTO: return "TOK_PONTO";
    case TOK_EOF: return "TOK_EOF";
    case TOK_INVALIDO: return "TOK_INVALIDO";
    }

    return "TOK_DESCONHECIDO";
}

void token_destroy(Token *token)
{
    if (token == NULL) {
        return;
    }

    free(token->lexeme);
    *token = (Token){0};
}
