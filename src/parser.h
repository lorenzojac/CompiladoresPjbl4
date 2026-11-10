#ifndef MACARRONICA_PARSER_H
#define MACARRONICA_PARSER_H

/*
 * Parser de descida recursiva da Macaronica.
 *
 * Consome os tokens do lexer, constroi a AST e acumula diagnosticos lexicos
 * e sintaticos (o parser continua apos erros, ressincronizando no proximo
 * comando ou item de nivel superior). A gramatica efetiva esta em
 * docs/README_gramatica.md.
 */

#include <stdbool.h>
#include <stddef.h>

#include "ast.h"
#include "lexer.h"

/*
 * Limites de profundidade (D35). Um fonte mais aninhado que isso gera um
 * erro sintatico normal, em vez de estourar a pilha do parser, da analise
 * semantica, dos destrutores da AST ou do gerador de codigo.
 *
 * - PARSER_MAX_NESTING conta niveis de recursao do parser: parenteses,
 *   argumentos, indices, blocos [ ], operadores unarios e potencia.
 * - PARSER_MAX_EXPRESSION_HEIGHT limita a altura de uma arvore de expressao,
 *   que cresce mesmo sem recursao no parser (cadeias a + a + a + ...).
 */
#define PARSER_MAX_NESTING ((size_t)256)
#define PARSER_MAX_EXPRESSION_HEIGHT ((size_t)1024)

typedef enum {
    DIAG_LEXICAL,
    DIAG_SYNTAX,
    DIAG_INTERNAL
} DiagnosticKind;

typedef struct {
    DiagnosticKind kind;
    SourceLocation location;
    TokenType found_type;
    char *found_lexeme;
    size_t found_lexeme_length;
    const char *expected;
    const char *message;
} Diagnostic;

typedef enum {
    PARSE_OK,
    PARSE_INPUT_ERRORS,
    PARSE_INTERNAL_FAILURE
} ParseStatus;

typedef struct {
    ParseStatus status;
    AstProgram *ast;
    Diagnostic *diagnostics;
    size_t diagnostic_count;
    size_t lexical_error_count;
    size_t syntax_error_count;
    /*
     * Erros sintaticos que nao foram reportados por serem consequencia direta
     * de um erro ja reportado no mesmo comando (D36): token invalido descartado
     * pelo lexer ou limite de aninhamento atingido.
     */
    size_t suppressed_syntax_error_count;
} ParseResult;

typedef struct {
    Lexer lexer;
    Token current;
    bool has_current;
    bool initialized;
    bool fatal;
    bool parsed;
    AstProgram *program;
    Diagnostic *diagnostics;
    size_t diagnostic_count;
    size_t diagnostic_capacity;
    size_t lexical_error_count;
    size_t syntax_error_count;
    size_t suppressed_syntax_error_count;

    /* Nivel atual de recursao (comparado com PARSER_MAX_NESTING). */
    size_t depth;
    /*
     * Modo panico: ligado depois de um erro cuja consequencia sintatica e
     * previsivel (token invalido, limite de aninhamento). Enquanto ligado,
     * novos erros sintaticos sao contados como suprimidos. Desliga ao
     * consumir ';', '[' ou ']'.
     */
    bool panic;
    /* Linha e coluna final do ultimo token consumido (para ';' ausente). */
    bool has_previous;
    size_t previous_line;
    size_t previous_end_column;
} Parser;

bool parser_init(Parser *parser, const char *source, size_t length);
bool parser_init_from_string(Parser *parser, const char *source);
ParseResult parser_parse(Parser *parser);
void parser_destroy(Parser *parser);
void parse_result_destroy(ParseResult *result);

#endif
