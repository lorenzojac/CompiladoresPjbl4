/*
 * parser.c - analisador sintatico de descida recursiva.
 *
 * Organizacao do arquivo:
 *   1. utilitarios (copias, diagnosticos, avanco de token, modo panico);
 *   2. ressincronizacao apos erro (comando, item global, registro, lista);
 *   3. itens de nivel superior: registros, funcoes, declaracoes globais;
 *   4. comandos (declaracao, atribuicao, chamada, cond, durante, repete,
 *      respost, bloco, le/imprime);
 *   5. expressoes, um nivel de funcao por nivel de precedencia (D08):
 *      OU < XOR < E < NAO < comparacao < + - < * / // < ^ < unario < primario.
 *
 * Recuperacao de erros: cada comando que falha devolve NULL e o laco do bloco
 * chama synchronize_statement, que descarta tokens ate um ';' ou ate o inicio
 * de outro comando. Assim varios erros sao relatados em uma unica execucao.
 */
#include "parser.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const char *const MESSAGE_UNEXPECTED = "token inesperado";
static const char *const MESSAGE_EXPECTED_EXPRESSION =
    "expressao esperada";
static const char *const MESSAGE_EXPECTED_ITEM =
    "item de nivel superior esperado";
static const char *const MESSAGE_EXPECTED_STATEMENT =
    "comando esperado";
static const char *const MESSAGE_MISSING_SEMICOLON =
    "';' ausente ao final da linha";
static const char *const MESSAGE_MAX_NESTING =
    "aninhamento profundo demais (limite de 256 niveis)";
static const char *const MESSAGE_MAX_HEIGHT =
    "expressao longa ou profunda demais (limite de altura 1024); "
    "divida-a usando variaveis auxiliares";
static const char *const MESSAGE_ID_STATEMENT =
    "apos identificador, esperado 'receba' (atribuicao) ou '(' (chamada)";

static bool parser_advance(Parser *parser);
static bool parser_ensure_current(Parser *parser);
static bool parser_check(const Parser *parser, TokenType type);
static bool parser_match(Parser *parser, TokenType type);
static bool parser_consume(
    Parser *parser,
    TokenType type,
    const char *expected
);

static AstProgram *parse_program(Parser *parser);
static AstTopLevelItem *parse_item(Parser *parser);
static AstTopLevelItem *parse_record_item(Parser *parser);
static AstTopLevelItem *parse_non_main_function_item(Parser *parser);
static AstTopLevelItem *parse_typed_top_level_item(Parser *parser);
static AstFunctionDecl *parse_main_after_type(
    Parser *parser,
    AstType return_type,
    SourceLocation location
);
static AstFunctionDecl *parse_non_main_function(Parser *parser);
static bool parse_record_prefix(
    Parser *parser,
    char **out_name,
    SourceLocation *out_location
);
static AstRecordDecl *parse_record_after_name(
    Parser *parser,
    char *name,
    SourceLocation location
);
static AstDeclaration *parse_record_declaration_after_name(
    Parser *parser,
    char *record_name,
    SourceLocation location
);
static AstField *parse_field(Parser *parser);
static AstType parse_type(Parser *parser, bool allow_void);
static AstDeclaration *parse_declaration_after_type(
    Parser *parser,
    AstType type,
    bool require_semicolon
);
static AstDeclarator *parse_declarator(Parser *parser);
static AstParameter *parse_parameter(Parser *parser);
static bool parse_parameter_list(
    Parser *parser,
    AstFunctionDecl *function
);
static AstBlock *parse_block(Parser *parser);
static AstStatement *parse_statement(Parser *parser);
static AstStatement *parse_declaration_statement(Parser *parser);
static AstStatement *parse_identifier_statement(Parser *parser);
static AstStatement *parse_if_statement(Parser *parser);
static AstStatement *parse_while_statement(Parser *parser);
static AstStatement *parse_repeat_statement(Parser *parser);
static AstStatement *parse_return_statement(Parser *parser);
static AstStatement *parse_block_statement(Parser *parser);
static AstStatement *parse_io_statement(Parser *parser);
static AstAssignment *parse_assignment_core(Parser *parser);
static AstLValue *parse_lvalue(Parser *parser);
static bool parser_expect_semicolon(Parser *parser);
static AstIf *parse_if_after_keyword(Parser *parser, SourceLocation location);
static AstExpression *parse_expression(Parser *parser);
static AstExpression *parse_or(Parser *parser);
static AstExpression *parse_xor(Parser *parser);
static AstExpression *parse_and(Parser *parser);
static AstExpression *parse_not(Parser *parser);
static AstExpression *parse_atom(Parser *parser);
static AstExpression *parse_value(Parser *parser);
static AstExpression *parse_arithmetic(Parser *parser);
static AstExpression *parse_term(Parser *parser);
static AstExpression *parse_power(Parser *parser);
static AstExpression *parse_unary(Parser *parser);
static AstExpression *parse_primary(Parser *parser);
static AstExpression *parse_literal(Parser *parser);
static AstExpression *parse_root(Parser *parser);
static AstExpression *parse_reference(Parser *parser);
static bool parse_argument_list(Parser *parser, AstReference *reference);

static bool is_value_type_token(TokenType type)
{
    return type == TOK_BOOL ||
           type == TOK_INTEIRA ||
           type == TOK_FLUT ||
           type == TOK_PALAVRA ||
           type == TOK_DUPLOCARPADO;
}

/*
 * Tokens que so podem aparecer no inicio de um comando. TOK_ID nao esta aqui
 * porque tambem aparece no meio de expressoes.
 */
static bool is_strong_statement_start(TokenType type)
{
    return is_value_type_token(type) ||
           type == TOK_COND ||
           type == TOK_REPETE ||
           type == TOK_DURANTE ||
           type == TOK_RESPOST ||
           type == TOK_ABRE_COLCHETE ||
           type == TOK_LE ||
           type == TOK_IMPRIME ||
           type == TOK_REGISTRO;
}

static bool is_statement_start(TokenType type)
{
    return is_strong_statement_start(type) || type == TOK_ID;
}

static SourceLocation location_of_token(const Token *token)
{
    SourceLocation location = {0};

    if (token != NULL) {
        location.line = token->line;
        location.column = token->column;
    }
    return location;
}

static bool copy_bytes(const char *source, size_t length, char **out)
{
    char *copy;

    if (out == NULL || length == SIZE_MAX ||
        (length > 0 && source == NULL)) {
        return false;
    }

    copy = malloc(length + 1);
    if (copy == NULL) {
        return false;
    }

    if (length > 0 && source != NULL) {
        memcpy(copy, source, length);
    }
    copy[length] = '\0';
    *out = copy;
    return true;
}

static void *parser_calloc(Parser *parser, size_t count, size_t size)
{
    void *memory;

    if (parser == NULL || count == 0 || size == 0 ||
        count > SIZE_MAX / size) {
        if (parser != NULL) {
            parser->fatal = true;
        }
        return NULL;
    }

    memory = calloc(count, size);
    if (memory == NULL) {
        parser->fatal = true;
    }
    return memory;
}

static char *copy_token_lexeme(Parser *parser, const Token *token)
{
    char *copy = NULL;

    if (parser == NULL || token == NULL || token->lexeme == NULL) {
        if (parser != NULL) {
            parser->fatal = true;
        }
        return NULL;
    }

    if (!copy_bytes(token->lexeme, token->lexeme_length, &copy)) {
        parser->fatal = true;
        return NULL;
    }
    return copy;
}

static bool grow_diagnostics(Parser *parser)
{
    size_t new_capacity;
    Diagnostic *new_diagnostics;

    if (parser->diagnostic_capacity == 0) {
        new_capacity = 4;
    } else {
        if (parser->diagnostic_capacity > SIZE_MAX / 2) {
            return false;
        }
        new_capacity = parser->diagnostic_capacity * 2;
    }

    if (new_capacity > SIZE_MAX / sizeof(*parser->diagnostics)) {
        return false;
    }

    new_diagnostics = realloc(
        parser->diagnostics,
        new_capacity * sizeof(*parser->diagnostics)
    );
    if (new_diagnostics == NULL) {
        return false;
    }

    parser->diagnostics = new_diagnostics;
    parser->diagnostic_capacity = new_capacity;
    return true;
}

static bool append_diagnostic(Parser *parser, Diagnostic *diagnostic)
{
    if (parser == NULL || diagnostic == NULL ||
        parser->diagnostic_count > parser->diagnostic_capacity) {
        return false;
    }

    if (parser->diagnostic_count == parser->diagnostic_capacity &&
        !grow_diagnostics(parser)) {
        return false;
    }

    parser->diagnostics[parser->diagnostic_count++] = *diagnostic;
    if (diagnostic->kind == DIAG_LEXICAL) {
        parser->lexical_error_count++;
    } else if (diagnostic->kind == DIAG_SYNTAX) {
        parser->syntax_error_count++;
    }
    return true;
}

static bool add_diagnostic_from_token(
    Parser *parser,
    DiagnosticKind kind,
    const Token *token,
    const char *expected,
    const char *message
)
{
    Diagnostic diagnostic = {0};

    if (parser == NULL || token == NULL) {
        return false;
    }

    diagnostic.kind = kind;
    diagnostic.location = location_of_token(token);
    diagnostic.found_type = token->type;
    diagnostic.expected = expected;
    diagnostic.message = message;

    if (token->lexeme != NULL &&
        !copy_bytes(
            token->lexeme,
            token->lexeme_length,
            &diagnostic.found_lexeme
        )) {
        parser->fatal = true;
        return false;
    }
    diagnostic.found_lexeme_length = token->lexeme_length;

    if (!append_diagnostic(parser, &diagnostic)) {
        free(diagnostic.found_lexeme);
        parser->fatal = true;
        return false;
    }
    return true;
}

/*
 * Registra um erro sintatico no token atual. Em modo panico (D36) o erro e
 * apenas contado como suprimido: ele e consequencia de um erro ja relatado.
 * O retorno indica somente se houve falha interna.
 */
static bool add_syntax_error(
    Parser *parser,
    const char *expected,
    const char *message
)
{
    if (parser == NULL || !parser->has_current) {
        if (parser != NULL) {
            parser->fatal = true;
        }
        return false;
    }

    if (parser->panic) {
        parser->suppressed_syntax_error_count++;
        return true;
    }

    return add_diagnostic_from_token(
        parser,
        DIAG_SYNTAX,
        &parser->current,
        expected,
        message
    );
}

/* Erro sintatico que, alem de relatado, liga o modo panico. */
static bool add_syntax_error_and_panic(
    Parser *parser,
    const char *expected,
    const char *message
)
{
    bool ok = add_syntax_error(parser, expected, message);

    if (parser != NULL) {
        parser->panic = true;
    }
    return ok;
}

static bool record_lexical_error(Parser *parser, const Token *token)
{
    return add_diagnostic_from_token(
        parser,
        DIAG_LEXICAL,
        token,
        NULL,
        token->error_message
    );
}

/*
 * Avanca para o proximo token valido.
 *
 * - TOK_INVALIDO: o erro lexico e registrado, o token e descartado e o modo
 *   panico e ligado, porque o comando atual quase certamente ficara
 *   sintaticamente incompleto (evita a cascata "erro lexico + sintatico").
 * - Token valido com error_message (identificador com acento, D37): o erro
 *   lexico e registrado, mas o token segue para o parser como TOK_ID.
 * - Consumir ';', '[' ou ']' encerra o modo panico: e uma fronteira de
 *   comando.
 */
static bool parser_advance(Parser *parser)
{
    Token token = {0};

    if (parser == NULL || parser->fatal || !parser->initialized) {
        return false;
    }

    if (parser->has_current) {
        TokenType consumed = parser->current.type;

        if (consumed != TOK_EOF) {
            parser->has_previous = true;
            parser->previous_line = parser->current.line;
            parser->previous_end_column =
                parser->current.column + parser->current.lexeme_length;
        }
        if (consumed == TOK_PONTO_VIRGULA ||
            consumed == TOK_ABRE_COLCHETE ||
            consumed == TOK_FECHA_COLCHETE) {
            parser->panic = false;
        }
        token_destroy(&parser->current);
        parser->has_current = false;
    }

    for (;;) {
        if (!lexer_next_token(&parser->lexer, &token)) {
            parser->fatal = true;
            token_destroy(&token);
            return false;
        }

        if (token.type == TOK_INVALIDO) {
            if (!record_lexical_error(parser, &token)) {
                token_destroy(&token);
                return false;
            }
            parser->panic = true;
            token_destroy(&token);
            continue;
        }

        if (token.error_message != NULL) {
            if (!record_lexical_error(parser, &token)) {
                token_destroy(&token);
                return false;
            }
        }

        parser->current = token;
        parser->has_current = true;
        return true;
    }
}

static bool parser_ensure_current(Parser *parser)
{
    if (parser == NULL || parser->fatal || !parser->initialized) {
        return false;
    }

    if (parser->has_current) {
        return true;
    }
    return parser_advance(parser);
}

static bool parser_check(const Parser *parser, TokenType type)
{
    return parser != NULL && parser->has_current &&
           parser->current.type == type;
}

static bool parser_match(Parser *parser, TokenType type)
{
    if (!parser_ensure_current(parser)) {
        return false;
    }

    if (!parser_check(parser, type)) {
        return false;
    }
    return parser_advance(parser);
}

static bool parser_consume(
    Parser *parser,
    TokenType type,
    const char *expected
)
{
    if (!parser_ensure_current(parser)) {
        return false;
    }

    if (!parser_check(parser, type)) {
        add_syntax_error(parser, expected, MESSAGE_UNEXPECTED);
        return false;
    }
    return parser_advance(parser);
}

static bool parser_at_end(const Parser *parser)
{
    return parser_check(parser, TOK_EOF);
}

/*
 * Consome o ';' que termina um comando ou declaracao.
 *
 * Recuperacao de ';' ausente (D36): se o token atual esta em uma linha
 * posterior a do ultimo token consumido, o erro e relatado na posicao em que
 * o ';' deveria estar e o parser age como se ele tivesse sido inserido,
 * retornando true. Isso evita a cascata classica em que a linha seguinte
 * ("inteira b;") e mal interpretada.
 */
static bool parser_expect_semicolon(Parser *parser)
{
    Diagnostic diagnostic = {0};

    if (!parser_ensure_current(parser)) {
        return false;
    }
    if (parser_check(parser, TOK_PONTO_VIRGULA)) {
        return parser_advance(parser);
    }
    if (!parser->has_previous ||
        parser->current.line <= parser->previous_line) {
        add_syntax_error(parser, "TOK_PONTO_VIRGULA", MESSAGE_UNEXPECTED);
        return false;
    }

    if (parser->panic) {
        parser->suppressed_syntax_error_count++;
        parser->panic = false;
        return true;
    }

    diagnostic.kind = DIAG_SYNTAX;
    diagnostic.location.line = parser->previous_line;
    diagnostic.location.column = parser->previous_end_column;
    diagnostic.found_type = parser->current.type;
    diagnostic.expected = "TOK_PONTO_VIRGULA";
    diagnostic.message = MESSAGE_MISSING_SEMICOLON;
    if (parser->current.lexeme != NULL) {
        if (!copy_bytes(
                parser->current.lexeme,
                parser->current.lexeme_length,
                &diagnostic.found_lexeme
            )) {
            parser->fatal = true;
            return false;
        }
        diagnostic.found_lexeme_length = parser->current.lexeme_length;
    }
    if (!append_diagnostic(parser, &diagnostic)) {
        free(diagnostic.found_lexeme);
        parser->fatal = true;
        return false;
    }
    return true;
}

/*
 * Controle de profundidade (D35). Todo parser_enter que retorna true exige
 * um parser_leave correspondente. Ao atingir o limite, o erro e relatado uma
 * unica vez e o modo panico suprime a cascata de ')' e ']' esperados nos
 * niveis externos.
 */
static TokenType closing_token_for(TokenType open)
{
    switch (open) {
    case TOK_ABRE_PAREN:
        return TOK_FECHA_PAREN;
    case TOK_ABRE_COLCHETE:
        return TOK_FECHA_COLCHETE;
    case TOK_ABRE_CHAVE:
        return TOK_FECHA_CHAVE;
    default:
        return TOK_EOF;
    }
}

/*
 * Pula um grupo balanceado "( ... )", "[ ... ]" ou "{ ... }" a partir do
 * token de abertura atual, sem recursao. Usado quando o limite de
 * aninhamento e atingido: o conteudo profundo inteiro vira um unico erro.
 */
static void skip_balanced_group(Parser *parser)
{
    TokenType open = parser->current.type;
    TokenType close = closing_token_for(open);
    size_t level = 0;

    if (close == TOK_EOF) {
        return;
    }
    while (parser->has_current && !parser_at_end(parser) && !parser->fatal) {
        if (parser_check(parser, open)) {
            level++;
        } else if (parser_check(parser, close)) {
            level--;
        }
        if (!parser_advance(parser)) {
            return;
        }
        if (level == 0) {
            break;
        }
    }
    /* parser_advance desliga o panico em '[' e ']'; o grupo ainda e erro. */
    parser->panic = true;
}

static bool parser_enter(Parser *parser)
{
    if (parser->depth >= PARSER_MAX_NESTING) {
        if (!parser->panic) {
            add_syntax_error_and_panic(
                parser,
                "aninhamento menor",
                MESSAGE_MAX_NESTING
            );
        } else {
            parser->suppressed_syntax_error_count++;
        }
        skip_balanced_group(parser);
        return false;
    }
    parser->depth++;
    return true;
}

static void parser_leave(Parser *parser)
{
    if (parser->depth > 0) {
        parser->depth--;
    }
}

/*
 * Descarta tokens ate o fim do comando com erro: consome o proximo ';' ou
 * para antes de ']' / EOF / inicio de outro comando. Em modo panico so os
 * inicios "fortes" (palavras reservadas, '[') contam, para nao parar em cada
 * identificador de uma expressao enorme.
 */
static void synchronize_statement(Parser *parser)
{
    bool discarded = false;

    while (parser != NULL && parser->has_current &&
           !parser_at_end(parser) &&
           !parser_check(parser, TOK_FECHA_COLCHETE)) {
        if (parser_check(parser, TOK_PONTO_VIRGULA)) {
            parser_advance(parser);
            return;
        }
        if (discarded &&
            (parser->panic
                 ? is_strong_statement_start(parser->current.type)
                 : is_statement_start(parser->current.type))) {
            return;
        }
        if (!parser_advance(parser)) {
            return;
        }
        discarded = true;
    }
}

static void synchronize_top_level(Parser *parser)
{
    bool discarded = false;

    while (parser != NULL && parser->has_current &&
           !parser_at_end(parser)) {
        if (discarded &&
            (parser_check(parser, TOK_REGISTRO) ||
             parser_check(parser, TOK_OUTRAFUNCAO) ||
             parser_check(parser, TOK_VAZIO) ||
             is_value_type_token(parser->current.type))) {
            return;
        }
        if (!parser_advance(parser)) {
            return;
        }
        discarded = true;
    }
}

static void synchronize_record(Parser *parser)
{
    while (parser != NULL && parser->has_current &&
           !parser_at_end(parser) &&
           !parser_check(parser, TOK_FECHA_COLCHETE)) {
        if (parser_check(parser, TOK_PONTO_VIRGULA)) {
            parser_advance(parser);
            return;
        }
        if (!parser_advance(parser)) {
            return;
        }
    }
}

static bool synchronize_parenthesized(Parser *parser)
{
    while (parser != NULL && parser->has_current &&
           !parser_at_end(parser) &&
           !parser_check(parser, TOK_FECHA_PAREN)) {
        if (parser_check(parser, TOK_VIRGULA)) {
            parser_advance(parser);
            return true;
        }
        if (!parser_advance(parser)) {
            return false;
        }
    }
    return false;
}

static bool parse_type_into(
    Parser *parser,
    bool allow_void,
    AstType *out_type
)
{
    AstType type = {0};

    if (parser == NULL || out_type == NULL ||
        !parser_ensure_current(parser)) {
        return false;
    }

    type.location = location_of_token(&parser->current);
    switch (parser->current.type) {
    case TOK_BOOL:
        type.kind = AST_TYPE_BOOL;
        break;
    case TOK_INTEIRA:
        type.kind = AST_TYPE_INTEIRA;
        break;
    case TOK_FLUT:
        type.kind = AST_TYPE_FLUT;
        break;
    case TOK_PALAVRA:
        type.kind = AST_TYPE_PALAVRA;
        break;
    case TOK_DUPLOCARPADO:
        type.kind = AST_TYPE_DUPLOCARPADO;
        break;
    case TOK_VAZIO:
        if (!allow_void) {
            add_syntax_error(parser, "tipo de valor", MESSAGE_UNEXPECTED);
            return false;
        }
        type.kind = AST_TYPE_VAZIO;
        break;
    default:
        add_syntax_error(
            parser,
            allow_void ? "tipo de retorno" : "tipo de valor",
            MESSAGE_UNEXPECTED
        );
        return false;
    }

    if (!parser_advance(parser)) {
        return false;
    }
    *out_type = type;
    return true;
}

static AstType parse_type(Parser *parser, bool allow_void)
{
    AstType type = {0};
    parse_type_into(parser, allow_void, &type);
    return type;
}

static AstTopLevelItem *make_top_level_item(
    Parser *parser,
    AstTopLevelItemKind kind,
    SourceLocation location
)
{
    AstTopLevelItem *item = parser_calloc(parser, 1, sizeof(*item));

    if (item != NULL) {
        item->kind = kind;
        item->location = location;
    }
    return item;
}

static AstStatement *make_statement(
    Parser *parser,
    AstStatementKind kind,
    SourceLocation location
)
{
    AstStatement *statement = parser_calloc(parser, 1, sizeof(*statement));

    if (statement != NULL) {
        statement->kind = kind;
        statement->location = location;
    }
    return statement;
}

static AstExpression *make_expression(
    Parser *parser,
    AstExpressionKind kind,
    SourceLocation location
)
{
    AstExpression *expression = parser_calloc(parser, 1, sizeof(*expression));

    if (expression != NULL) {
        expression->kind = kind;
        expression->location = location;
        expression->height = 1;
    }
    return expression;
}

static size_t expression_height(const AstExpression *expression)
{
    return expression == NULL ? 0 : expression->height;
}

/*
 * Define a altura de um no a partir da maior altura entre seus filhos e
 * verifica PARSER_MAX_EXPRESSION_HEIGHT (D35). Em caso de excesso, relata o
 * erro (uma vez por comando, via modo panico) e devolve false; o chamador
 * destroi o no.
 */
static bool set_expression_height(
    Parser *parser,
    AstExpression *expression,
    size_t tallest_child
)
{
    expression->height = tallest_child + 1;
    if (expression->height <= PARSER_MAX_EXPRESSION_HEIGHT) {
        return true;
    }
    if (!parser->panic) {
        add_syntax_error_and_panic(
            parser,
            "expressao menor",
            MESSAGE_MAX_HEIGHT
        );
    } else {
        parser->suppressed_syntax_error_count++;
    }
    return false;
}

/*
 * Item iniciado por 'registro': definicao ("registro R @ [...]") ou
 * declaracao global de variavel registro ("registro R p;"). O token apos o
 * nome decide: '@' e definicao; qualquer outro, declaracao.
 */
static AstTopLevelItem *parse_record_item(Parser *parser)
{
    AstTopLevelItem *item;
    SourceLocation location;
    char *name;

    if (!parser_ensure_current(parser)) {
        return NULL;
    }
    if (!parse_record_prefix(parser, &name, &location)) {
        if (!parser->fatal) {
            synchronize_top_level(parser);
        }
        return NULL;
    }

    if (parser_check(parser, TOK_ARROBA)) {
        AstRecordDecl *record = parse_record_after_name(parser, name, location);

        if (record == NULL) {
            return NULL;
        }
        item = make_top_level_item(parser, AST_TOP_RECORD, location);
        if (item == NULL) {
            ast_record_decl_destroy(record);
            return NULL;
        }
        item->as.record = record;
        return item;
    }

    {
        AstDeclaration *declaration = parse_record_declaration_after_name(
            parser,
            name,
            location
        );

        if (declaration == NULL) {
            return NULL;
        }
        item = make_top_level_item(parser, AST_TOP_DECLARATION, location);
        if (item == NULL) {
            ast_declaration_destroy(declaration);
            return NULL;
        }
        item->as.declaration = declaration;
        return item;
    }
}

static AstTopLevelItem *parse_non_main_function_item(Parser *parser)
{
    AstTopLevelItem *item;
    AstFunctionDecl *function;
    SourceLocation location;

    if (!parser_ensure_current(parser)) {
        return NULL;
    }
    location = location_of_token(&parser->current);
    function = parse_non_main_function(parser);
    if (function == NULL) {
        return NULL;
    }

    item = make_top_level_item(parser, AST_TOP_FUNCTION, location);
    if (item == NULL) {
        ast_function_decl_destroy(function);
        return NULL;
    }
    item->as.function = function;
    return item;
}

static AstTopLevelItem *parse_typed_top_level_item(Parser *parser)
{
    AstType type;
    SourceLocation location;
    AstTopLevelItem *item;
    AstFunctionDecl *function;
    AstDeclaration *declaration;

    if (!parser_ensure_current(parser)) {
        return NULL;
    }
    location = location_of_token(&parser->current);
    if (!parse_type_into(parser, true, &type)) {
        synchronize_top_level(parser);
        return NULL;
    }

    if (parser_check(parser, TOK_PRINCIPAL)) {
        function = parse_main_after_type(parser, type, location);
        if (function == NULL) {
            return NULL;
        }
        item = make_top_level_item(parser, AST_TOP_FUNCTION, location);
        if (item == NULL) {
            ast_function_decl_destroy(function);
            return NULL;
        }
        item->as.function = function;
        return item;
    }

    if (type.kind == AST_TYPE_VAZIO) {
        add_syntax_error(
            parser,
            "TOK_PRINCIPAL",
            "vazio so pode iniciar retorno de funcao"
        );
        synchronize_top_level(parser);
        return NULL;
    }

    declaration = parse_declaration_after_type(parser, type, true);
    if (declaration == NULL) {
        return NULL;
    }
    item = make_top_level_item(parser, AST_TOP_DECLARATION, location);
    if (item == NULL) {
        ast_declaration_destroy(declaration);
        return NULL;
    }
    item->as.declaration = declaration;
    return item;
}

static AstTopLevelItem *parse_item(Parser *parser)
{
    if (!parser_ensure_current(parser)) {
        return NULL;
    }

    switch (parser->current.type) {
    case TOK_REGISTRO:
        return parse_record_item(parser);
    case TOK_OUTRAFUNCAO:
        return parse_non_main_function_item(parser);
    case TOK_VAZIO:
    case TOK_BOOL:
    case TOK_INTEIRA:
    case TOK_FLUT:
    case TOK_PALAVRA:
    case TOK_DUPLOCARPADO:
        return parse_typed_top_level_item(parser);
    default:
        add_syntax_error(parser, "item de nivel superior", MESSAGE_EXPECTED_ITEM);
        synchronize_top_level(parser);
        return NULL;
    }
}

/*
 * PROGRAMA -> ITEM* EOF. Itens com erro sao descartados e o parser
 * ressincroniza no proximo inicio de item (registro, outrafuncao, tipo).
 */
static AstProgram *parse_program(Parser *parser)
{
    AstProgram *program;

    program = parser_calloc(parser, 1, sizeof(*program));
    if (program == NULL) {
        return NULL;
    }
    parser->program = program;

    while (!parser->fatal && parser->has_current &&
           !parser_at_end(parser)) {
        size_t before = parser->lexer.current;
        AstTopLevelItem *item = parse_item(parser);

        if (item != NULL) {
            if (!ast_program_append(program, item)) {
                ast_top_level_item_destroy(item);
                parser->fatal = true;
                break;
            }
        }

        if (parser->fatal || parser_at_end(parser)) {
            break;
        }

        if (parser->lexer.current == before) {
            synchronize_top_level(parser);
            if (parser->lexer.current == before &&
                !parser_at_end(parser)) {
                parser_advance(parser);
            }
        }
    }

    return program;
}

/*
 * Le "'registro' ID", comum a definicao de registro e a declaracao de
 * variavel de tipo registro (D39). Em caso de erro relata e devolve false.
 */
static bool parse_record_prefix(
    Parser *parser,
    char **out_name,
    SourceLocation *out_location
)
{
    *out_name = NULL;
    *out_location = location_of_token(&parser->current);
    if (!parser_advance(parser)) {
        return false;
    }
    if (!parser_check(parser, TOK_ID)) {
        add_syntax_error(parser, "TOK_ID", MESSAGE_UNEXPECTED);
        return false;
    }
    *out_name = copy_token_lexeme(parser, &parser->current);
    if (*out_name == NULL) {
        return false;
    }
    if (!parser_advance(parser)) {
        free(*out_name);
        *out_name = NULL;
        return false;
    }
    return true;
}

/*
 * DECLARACAO_REGISTRO -> 'registro' ID DECLARADOR (',' DECLARADOR)* ';'
 * (o prefixo ja foi lido). Ex.: "registro Ponto p, vet{3} pts;".
 */
static AstDeclaration *parse_record_declaration_after_name(
    Parser *parser,
    char *record_name,
    SourceLocation location
)
{
    AstType type;
    AstDeclaration *declaration;

    type.kind = AST_TYPE_REGISTRO;
    type.location = location;
    declaration = parse_declaration_after_type(parser, type, true);
    if (declaration == NULL) {
        free(record_name);
        return NULL;
    }
    declaration->record_name = record_name;
    return declaration;
}

/* REGISTRO -> 'registro' ID '@' '[' (TIPO_VALOR ID ';')* ']' (prefixo lido) */
static AstRecordDecl *parse_record_after_name(
    Parser *parser,
    char *name,
    SourceLocation location
)
{
    AstRecordDecl *record;
    AstField *field;

    record = parser_calloc(parser, 1, sizeof(*record));
    if (record == NULL) {
        free(name);
        return NULL;
    }
    record->location = location;
    record->name = name;

    parser_consume(parser, TOK_ARROBA, "TOK_ARROBA");
    parser_consume(parser, TOK_ABRE_COLCHETE, "TOK_ABRE_COLCHETE");

    while (!parser->fatal && parser->has_current &&
           !parser_at_end(parser) &&
           !parser_check(parser, TOK_FECHA_COLCHETE)) {
        size_t before = parser->lexer.current;

        field = parse_field(parser);
        if (field != NULL) {
            if (!ast_record_append_field(record, field)) {
                ast_field_destroy(field);
                parser->fatal = true;
                break;
            }
        } else if (!parser->fatal) {
            synchronize_record(parser);
        }

        if (!parser->fatal && parser->has_current &&
            !parser_at_end(parser) &&
            !parser_check(parser, TOK_FECHA_COLCHETE) &&
            parser->lexer.current == before) {
            parser_advance(parser);
        }
    }

    if (parser_check(parser, TOK_FECHA_COLCHETE)) {
        parser_consume(parser, TOK_FECHA_COLCHETE, "TOK_FECHA_COLCHETE");
    } else if (parser_at_end(parser)) {
        add_syntax_error(parser, "TOK_FECHA_COLCHETE", MESSAGE_UNEXPECTED);
    }

    return record;
}

static AstField *parse_field(Parser *parser)
{
    AstField *field;
    AstType type;
    SourceLocation location;

    if (!parse_type_into(parser, false, &type)) {
        return NULL;
    }
    location = type.location;
    field = parser_calloc(parser, 1, sizeof(*field));
    if (field == NULL) {
        return NULL;
    }
    field->location = location;
    field->type = type;

    if (!parser_check(parser, TOK_ID)) {
        add_syntax_error(parser, "TOK_ID", MESSAGE_UNEXPECTED);
        ast_field_destroy(field);
        return NULL;
    }
    field->name = copy_token_lexeme(parser, &parser->current);
    if (field->name == NULL) {
        ast_field_destroy(field);
        return NULL;
    }
    if (!parser_advance(parser)) {
        ast_field_destroy(field);
        return NULL;
    }
    if (!parser_expect_semicolon(parser)) {
        ast_field_destroy(field);
        return NULL;
    }
    return field;
}

/* 'outrafuncao' TIPO_RETORNO ID '(' PARAMETROS? ')' '@' BLOCO */
static AstFunctionDecl *parse_non_main_function(Parser *parser)
{
    AstFunctionDecl *function;
    AstType return_type;
    SourceLocation location;

    location = location_of_token(&parser->current);
    parser_advance(parser);
    return_type = parse_type(parser, true);
    if (parser->fatal) {
        return NULL;
    }

    function = parser_calloc(parser, 1, sizeof(*function));
    if (function == NULL) {
        return NULL;
    }
    function->location = location;
    function->return_type = return_type;
    function->is_main = false;

    if (!parser_check(parser, TOK_ID)) {
        add_syntax_error(parser, "TOK_ID", MESSAGE_UNEXPECTED);
        synchronize_top_level(parser);
        ast_function_decl_destroy(function);
        return NULL;
    }
    function->name = copy_token_lexeme(parser, &parser->current);
    if (function->name == NULL) {
        ast_function_decl_destroy(function);
        return NULL;
    }
    if (!parser_advance(parser)) {
        ast_function_decl_destroy(function);
        return NULL;
    }

    parser_consume(parser, TOK_ABRE_PAREN, "TOK_ABRE_PAREN");
    if (!parser->fatal) {
        parse_parameter_list(parser, function);
    }
    parser_consume(parser, TOK_FECHA_PAREN, "TOK_FECHA_PAREN");
    parser_consume(parser, TOK_ARROBA, "TOK_ARROBA");
    function->body = parse_block(parser);

    if (function->body == NULL && parser->fatal) {
        ast_function_decl_destroy(function);
        return NULL;
    }
    return function;
}

/* TIPO_RETORNO 'principal' '(' PARAMETROS? ')' '@' BLOCO (tipo ja lido) */
static AstFunctionDecl *parse_main_after_type(
    Parser *parser,
    AstType return_type,
    SourceLocation location
)
{
    AstFunctionDecl *function;

    function = parser_calloc(parser, 1, sizeof(*function));
    if (function == NULL) {
        return NULL;
    }
    function->location = location;
    function->return_type = return_type;
    function->is_main = true;
    function->name = NULL;

    parser_consume(parser, TOK_PRINCIPAL, "TOK_PRINCIPAL");
    parser_consume(parser, TOK_ABRE_PAREN, "TOK_ABRE_PAREN");
    if (!parser->fatal) {
        parse_parameter_list(parser, function);
    }
    parser_consume(parser, TOK_FECHA_PAREN, "TOK_FECHA_PAREN");
    parser_consume(parser, TOK_ARROBA, "TOK_ARROBA");
    function->body = parse_block(parser);

    if (function->body == NULL && parser->fatal) {
        ast_function_decl_destroy(function);
        return NULL;
    }
    return function;
}

static AstParameter *parse_parameter(Parser *parser)
{
    AstParameter *parameter;
    AstType type;

    if (!parse_type_into(parser, false, &type)) {
        return NULL;
    }
    parameter = parser_calloc(parser, 1, sizeof(*parameter));
    if (parameter == NULL) {
        return NULL;
    }
    parameter->location = type.location;
    parameter->type = type;

    if (!parser_check(parser, TOK_ID)) {
        add_syntax_error(parser, "TOK_ID", MESSAGE_UNEXPECTED);
        ast_parameter_destroy(parameter);
        return NULL;
    }
    parameter->name = copy_token_lexeme(parser, &parser->current);
    if (parameter->name == NULL) {
        ast_parameter_destroy(parameter);
        return NULL;
    }
    if (!parser_advance(parser)) {
        ast_parameter_destroy(parameter);
        return NULL;
    }
    return parameter;
}

static bool parse_parameter_list(
    Parser *parser,
    AstFunctionDecl *function
)
{
    if (parser_check(parser, TOK_FECHA_PAREN)) {
        return true;
    }

    while (!parser->fatal && parser->has_current &&
           !parser_at_end(parser)) {
        AstParameter *parameter = parse_parameter(parser);

        if (parameter == NULL) {
            if (!parser->fatal) {
                if (synchronize_parenthesized(parser) &&
                    !parser_check(parser, TOK_FECHA_PAREN) &&
                    !parser_at_end(parser)) {
                    continue;
                }
            }
            return false;
        }
        if (!ast_function_append_parameter(function, parameter)) {
            ast_parameter_destroy(parameter);
            parser->fatal = true;
            return false;
        }

        if (!parser_match(parser, TOK_VIRGULA)) {
            break;
        }
        if (parser_check(parser, TOK_FECHA_PAREN)) {
            add_syntax_error(
                parser,
                "parametro",
                "virgula final nao e permitida"
            );
            return false;
        }
    }
    return true;
}

/* DECLARADOR -> ID | 'vet' '{' NUM_INTEIRO '}' ID */
static AstDeclarator *parse_declarator(Parser *parser)
{
    AstDeclarator *declarator;
    SourceLocation location;

    if (!parser_ensure_current(parser)) {
        return NULL;
    }

    declarator = parser_calloc(parser, 1, sizeof(*declarator));
    if (declarator == NULL) {
        return NULL;
    }

    location = location_of_token(&parser->current);
    if (parser_check(parser, TOK_ID)) {
        declarator->kind = AST_DECLARATOR_SIMPLE;
        declarator->location = location;
        declarator->name = copy_token_lexeme(parser, &parser->current);
        if (declarator->name == NULL) {
            ast_declarator_destroy(declarator);
            return NULL;
        }
        if (!parser_advance(parser)) {
            ast_declarator_destroy(declarator);
            return NULL;
        }
        return declarator;
    }

    if (parser_check(parser, TOK_VET)) {
        declarator->kind = AST_DECLARATOR_VECTOR;
        declarator->location = location;
        parser_advance(parser);
        parser_consume(parser, TOK_ABRE_CHAVE, "TOK_ABRE_CHAVE");
        if (!parser_check(parser, TOK_NUM_INTEIRO)) {
            add_syntax_error(parser, "TOK_NUM_INTEIRO", MESSAGE_UNEXPECTED);
            ast_declarator_destroy(declarator);
            return NULL;
        }
        declarator->vector_size_length = parser->current.lexeme_length;
        declarator->vector_size_lexeme =
            copy_token_lexeme(parser, &parser->current);
        if (declarator->vector_size_lexeme == NULL) {
            ast_declarator_destroy(declarator);
            return NULL;
        }
        if (!parser_advance(parser)) {
            ast_declarator_destroy(declarator);
            return NULL;
        }
        if (!parser_consume(parser, TOK_FECHA_CHAVE, "TOK_FECHA_CHAVE") ||
            !parser_check(parser, TOK_ID)) {
            if (parser->has_current &&
                !parser_check(parser, TOK_ID)) {
                add_syntax_error(parser, "TOK_ID", MESSAGE_UNEXPECTED);
            }
            ast_declarator_destroy(declarator);
            return NULL;
        }
        declarator->name = copy_token_lexeme(parser, &parser->current);
        if (declarator->name == NULL) {
            ast_declarator_destroy(declarator);
            return NULL;
        }
        if (!parser_advance(parser)) {
            ast_declarator_destroy(declarator);
            return NULL;
        }
        return declarator;
    }

    add_syntax_error(parser, "declarador", MESSAGE_UNEXPECTED);
    ast_declarator_destroy(declarator);
    return NULL;
}

static AstDeclaration *parse_declaration_after_type(
    Parser *parser,
    AstType type,
    bool require_semicolon
)
{
    AstDeclaration *declaration;
    AstDeclarator *declarator;

    declaration = parser_calloc(parser, 1, sizeof(*declaration));
    if (declaration == NULL) {
        return NULL;
    }
    declaration->location = type.location;
    declaration->type = type;

    declarator = parse_declarator(parser);
    if (declarator == NULL) {
        ast_declaration_destroy(declaration);
        return NULL;
    }
    if (!ast_declaration_append_declarator(declaration, declarator)) {
        ast_declarator_destroy(declarator);
        ast_declaration_destroy(declaration);
        parser->fatal = true;
        return NULL;
    }

    while (parser_match(parser, TOK_VIRGULA)) {
        const AstDeclarator *previous =
            declaration->declarators[declaration->declarator_count - 1];

        declarator = parse_declarator(parser);
        if (declarator == NULL) {
            ast_declaration_destroy(declaration);
            return NULL;
        }
        /*
         * D46: forma da especificacao oficial, "tipo vet{tamanho} nome, ...;".
         * Um nome simples depois de um declarador vetor herda o mesmo
         * vet{tamanho}: "inteira vet{8} a, b;" declara dois vetores.
         */
        if (declarator->kind == AST_DECLARATOR_SIMPLE &&
            previous->kind == AST_DECLARATOR_VECTOR) {
            declarator->kind = AST_DECLARATOR_VECTOR;
            declarator->vector_size_length = previous->vector_size_length;
            if (!copy_bytes(previous->vector_size_lexeme,
                            previous->vector_size_length,
                            &declarator->vector_size_lexeme)) {
                ast_declarator_destroy(declarator);
                ast_declaration_destroy(declaration);
                parser->fatal = true;
                return NULL;
            }
        }
        if (!ast_declaration_append_declarator(declaration, declarator)) {
            ast_declarator_destroy(declarator);
            ast_declaration_destroy(declaration);
            parser->fatal = true;
            return NULL;
        }
    }

    if (require_semicolon &&
        !parser_expect_semicolon(parser)) {
        ast_declaration_destroy(declaration);
        return NULL;
    }
    return declaration;
}

/*
 * BLOCO -> '[' COMANDO* ']'
 *
 * Cada comando que falha e seguido de synchronize_statement; a guarda
 * "before" garante progresso mesmo quando a ressincronizacao nao consome
 * nada. Se o limite de aninhamento for atingido, o grupo "[ ... ]" inteiro
 * e pulado e um bloco vazio e devolvido (o programa ja esta marcado como
 * invalido), para que os niveis externos continuem balanceados.
 */
static AstBlock *parse_block(Parser *parser)
{
    AstBlock *block;
    AstStatement *statement;

    if (!parser_check(parser, TOK_ABRE_COLCHETE)) {
        add_syntax_error(parser, "TOK_ABRE_COLCHETE", MESSAGE_UNEXPECTED);
        return NULL;
    }

    block = parser_calloc(parser, 1, sizeof(*block));
    if (block == NULL) {
        return NULL;
    }
    block->location = location_of_token(&parser->current);
    if (!parser_enter(parser)) {
        return block;
    }
    if (!parser_advance(parser)) {
        parser_leave(parser);
        ast_block_destroy(block);
        return NULL;
    }

    while (!parser->fatal && parser->has_current &&
           !parser_at_end(parser) &&
           !parser_check(parser, TOK_FECHA_COLCHETE)) {
        size_t before = parser->lexer.current;

        statement = parse_statement(parser);
        if (statement != NULL) {
            if (!ast_block_append_statement(block, statement)) {
                ast_statement_destroy(statement);
                parser->fatal = true;
                break;
            }
        } else if (!parser->fatal) {
            synchronize_statement(parser);
        }

        if (!parser->fatal && parser->has_current &&
            !parser_at_end(parser) &&
            !parser_check(parser, TOK_FECHA_COLCHETE) &&
            parser->lexer.current == before) {
            synchronize_statement(parser);
            if (parser->has_current &&
                !parser_at_end(parser) &&
                !parser_check(parser, TOK_FECHA_COLCHETE) &&
                parser->lexer.current == before) {
                parser_advance(parser);
            }
        }
    }

    if (parser_check(parser, TOK_FECHA_COLCHETE)) {
        parser_consume(parser, TOK_FECHA_COLCHETE, "TOK_FECHA_COLCHETE");
    } else if (parser_at_end(parser)) {
        add_syntax_error(parser, "TOK_FECHA_COLCHETE", MESSAGE_UNEXPECTED);
    }

    parser_leave(parser);
    return block;
}

static AstStatement *parse_declaration_statement(Parser *parser)
{
    AstType type;
    AstDeclaration *declaration;
    AstStatement *statement;

    if (!parse_type_into(parser, false, &type)) {
        return NULL;
    }
    declaration = parse_declaration_after_type(parser, type, true);
    if (declaration == NULL) {
        return NULL;
    }
    statement = make_statement(
        parser,
        AST_STMT_DECLARATION,
        type.location
    );
    if (statement == NULL) {
        ast_declaration_destroy(declaration);
        return NULL;
    }
    statement->as.declaration = declaration;
    return statement;
}

static AstLValue *finish_lvalue(
    Parser *parser,
    char *name,
    SourceLocation location
);

/*
 * SUFIXO_CAMPO -> ('.' ID)?   (D39). Devolve false so em erro; *out_field
 * fica NULL quando nao ha sufixo.
 */
static bool parse_field_suffix(Parser *parser, char **out_field)
{
    *out_field = NULL;
    if (!parser_check(parser, TOK_PONTO)) {
        return true;
    }
    if (!parser_advance(parser)) {
        return false;
    }
    if (!parser_check(parser, TOK_ID)) {
        add_syntax_error(parser, "TOK_ID", "esperado nome de campo apos '.'");
        return false;
    }
    *out_field = copy_token_lexeme(parser, &parser->current);
    if (*out_field == NULL) {
        return false;
    }
    if (!parser_advance(parser)) {
        free(*out_field);
        *out_field = NULL;
        return false;
    }
    return true;
}

static AstAssignment *finish_assignment(Parser *parser, AstLValue *lvalue);
static AstExpression *finish_reference(
    Parser *parser,
    char *name,
    SourceLocation location
);

/*
 * Comando iniciado por identificador:
 *   ATRIBUICAO    -> LVALUE 'receba' EXPRESSAO ';'
 *   CHAMADA_CMD   -> ID '(' ARGUMENTOS? ')' ';'          (D33)
 *
 * O identificador e lido uma unica vez; o token seguinte decide a forma.
 */
static AstStatement *parse_identifier_statement(Parser *parser)
{
    SourceLocation location = location_of_token(&parser->current);
    AstStatement *statement;
    char *name;

    name = copy_token_lexeme(parser, &parser->current);
    if (name == NULL) {
        return NULL;
    }
    if (!parser_advance(parser)) {
        free(name);
        return NULL;
    }

    if (parser_check(parser, TOK_ABRE_PAREN)) {
        AstExpression *call = finish_reference(parser, name, location);

        if (call == NULL) {
            return NULL;
        }
        if (!parser_expect_semicolon(parser)) {
            ast_expression_destroy(call);
            return NULL;
        }
        statement = make_statement(parser, AST_STMT_CALL, location);
        if (statement == NULL) {
            ast_expression_destroy(call);
            return NULL;
        }
        statement->as.call = call;
        return statement;
    }

    {
        AstLValue *lvalue = finish_lvalue(parser, name, location);
        AstAssignment *assignment;

        if (lvalue == NULL) {
            return NULL;
        }
        assignment = finish_assignment(parser, lvalue);
        if (assignment == NULL) {
            return NULL;
        }
        if (!parser_expect_semicolon(parser)) {
            ast_assignment_destroy(assignment);
            return NULL;
        }
        statement = make_statement(parser, AST_STMT_ASSIGNMENT, location);
        if (statement == NULL) {
            ast_assignment_destroy(assignment);
            return NULL;
        }
        statement->as.assignment = assignment;
        return statement;
    }
}

static AstStatement *parse_if_statement(Parser *parser)
{
    AstIf *if_statement;
    AstStatement *statement;
    SourceLocation location = location_of_token(&parser->current);

    if_statement = parse_if_after_keyword(parser, location);
    if (if_statement == NULL) {
        return NULL;
    }
    statement = make_statement(parser, AST_STMT_IF, location);
    if (statement == NULL) {
        ast_if_destroy(if_statement);
        return NULL;
    }
    statement->as.if_statement = if_statement;
    return statement;
}

/* 'durante' '(' CONDICAO ')' 'edai' BLOCO */
static AstStatement *parse_while_statement(Parser *parser)
{
    AstWhile *while_statement;
    AstStatement *statement;
    SourceLocation location = location_of_token(&parser->current);

    parser_advance(parser);
    while_statement = parser_calloc(parser, 1, sizeof(*while_statement));
    if (while_statement == NULL) {
        return NULL;
    }
    while_statement->location = location;

    parser_consume(parser, TOK_ABRE_PAREN, "TOK_ABRE_PAREN");
    while_statement->condition = parse_expression(parser);
    parser_consume(parser, TOK_FECHA_PAREN, "TOK_FECHA_PAREN");
    parser_consume(parser, TOK_EDAI, "TOK_EDAI");
    while_statement->body = parse_block(parser);

    statement = make_statement(parser, AST_STMT_WHILE, location);
    if (statement == NULL) {
        ast_while_destroy(while_statement);
        return NULL;
    }
    statement->as.while_statement = while_statement;
    return statement;
}

/* 'repete' '(' ATRIBUICAO ',' CONDICAO ',' ATRIBUICAO ')' 'edai' BLOCO (D10) */
static AstStatement *parse_repeat_statement(Parser *parser)
{
    AstRepeat *repeat;
    AstStatement *statement;
    SourceLocation location = location_of_token(&parser->current);

    parser_advance(parser);
    repeat = parser_calloc(parser, 1, sizeof(*repeat));
    if (repeat == NULL) {
        return NULL;
    }
    repeat->location = location;

    parser_consume(parser, TOK_ABRE_PAREN, "TOK_ABRE_PAREN");
    repeat->initialization = parse_assignment_core(parser);
    parser_consume(parser, TOK_VIRGULA, "TOK_VIRGULA");
    repeat->condition = parse_expression(parser);
    parser_consume(parser, TOK_VIRGULA, "TOK_VIRGULA");
    repeat->update = parse_assignment_core(parser);
    parser_consume(parser, TOK_FECHA_PAREN, "TOK_FECHA_PAREN");
    parser_consume(parser, TOK_EDAI, "TOK_EDAI");
    repeat->body = parse_block(parser);

    statement = make_statement(parser, AST_STMT_REPEAT, location);
    if (statement == NULL) {
        ast_repeat_destroy(repeat);
        return NULL;
    }
    statement->as.repeat_statement = repeat;
    return statement;
}

/* 'respost' EXPRESSAO ';' (respost sem valor nao existe, D07) */
static AstStatement *parse_return_statement(Parser *parser)
{
    AstReturn *return_statement;
    AstStatement *statement;
    SourceLocation location = location_of_token(&parser->current);

    parser_advance(parser);
    return_statement = parser_calloc(parser, 1, sizeof(*return_statement));
    if (return_statement == NULL) {
        return NULL;
    }
    return_statement->location = location;
    return_statement->value = parse_expression(parser);
    parser_expect_semicolon(parser);

    statement = make_statement(parser, AST_STMT_RETURN, location);
    if (statement == NULL) {
        ast_return_destroy(return_statement);
        return NULL;
    }
    statement->as.return_statement = return_statement;
    return statement;
}

static AstStatement *parse_block_statement(Parser *parser)
{
    AstBlock *block = parse_block(parser);
    AstStatement *statement;

    if (block == NULL) {
        return NULL;
    }
    statement = make_statement(
        parser,
        AST_STMT_BLOCK,
        block->location
    );
    if (statement == NULL) {
        ast_block_destroy(block);
        return NULL;
    }
    statement->as.block = block;
    return statement;
}

/* 'le' '(' LVALUE ')' ';'  |  'imprime' '(' EXPRESSAO ')' ';'  (D31) */
static AstStatement *parse_io_statement(Parser *parser)
{
    AstIoKind kind;
    SourceLocation location;
    AstLValue *target = NULL;
    AstExpression *value = NULL;
    AstIo *io;
    AstStatement *statement;

    if (!parser_ensure_current(parser)) {
        return NULL;
    }

    if (parser->current.type == TOK_LE) {
        kind = AST_IO_READ;
    } else if (parser->current.type == TOK_IMPRIME) {
        kind = AST_IO_WRITE;
    } else {
        add_syntax_error(parser, "TOK_LE ou TOK_IMPRIME", MESSAGE_UNEXPECTED);
        return NULL;
    }

    location = location_of_token(&parser->current);
    if (!parser_advance(parser) ||
        !parser_consume(parser, TOK_ABRE_PAREN, "TOK_ABRE_PAREN")) {
        return NULL;
    }

    if (kind == AST_IO_READ) {
        target = parse_lvalue(parser);
        if (target == NULL) {
            return NULL;
        }
    } else {
        value = parse_expression(parser);
        if (value == NULL) {
            return NULL;
        }
    }

    if (!parser_consume(parser, TOK_FECHA_PAREN, "TOK_FECHA_PAREN") ||
        !parser_expect_semicolon(parser)) {
        ast_lvalue_destroy(target);
        ast_expression_destroy(value);
        return NULL;
    }

    io = parser_calloc(parser, 1, sizeof(*io));
    if (io == NULL) {
        ast_lvalue_destroy(target);
        ast_expression_destroy(value);
        return NULL;
    }
    io->kind = kind;
    io->location = location;
    if (kind == AST_IO_READ) {
        io->as.target = target;
        target = NULL;
    } else {
        io->as.value = value;
        value = NULL;
    }

    statement = make_statement(parser, AST_STMT_IO, location);
    if (statement == NULL) {
        ast_io_destroy(io);
        return NULL;
    }
    statement->as.io = io;
    return statement;
}

static AstStatement *parse_statement(Parser *parser)
{
    if (!parser_ensure_current(parser)) {
        return NULL;
    }

    switch (parser->current.type) {
    case TOK_BOOL:
    case TOK_INTEIRA:
    case TOK_FLUT:
    case TOK_PALAVRA:
    case TOK_DUPLOCARPADO:
        return parse_declaration_statement(parser);
    case TOK_ID:
        return parse_identifier_statement(parser);
    case TOK_COND:
        return parse_if_statement(parser);
    case TOK_REPETE:
        return parse_repeat_statement(parser);
    case TOK_DURANTE:
        return parse_while_statement(parser);
    case TOK_RESPOST:
        return parse_return_statement(parser);
    case TOK_ABRE_COLCHETE:
        return parse_block_statement(parser);
    case TOK_LE:
    case TOK_IMPRIME:
        return parse_io_statement(parser);
    case TOK_REGISTRO: {
        /* Declaracao local de variavel registro: "registro Ponto p;" (D39). */
        SourceLocation location;
        AstDeclaration *declaration;
        AstStatement *statement;
        char *name;

        if (!parse_record_prefix(parser, &name, &location)) {
            return NULL;
        }
        if (parser_check(parser, TOK_ARROBA)) {
            add_syntax_error(
                parser,
                "declarador",
                "registros so podem ser definidos no nivel superior"
            );
            free(name);
            return NULL;
        }
        declaration = parse_record_declaration_after_name(
            parser,
            name,
            location
        );
        if (declaration == NULL) {
            return NULL;
        }
        statement = make_statement(parser, AST_STMT_DECLARATION, location);
        if (statement == NULL) {
            ast_declaration_destroy(declaration);
            return NULL;
        }
        statement->as.declaration = declaration;
        return statement;
    }
    case TOK_VAZIO: {
        /*
         * "vazio x;" (D07: vazio so e tipo de retorno). Um unico erro e a
         * declaracao inteira e consumida como se fosse inteira, para nao
         * gerar cascata em "x ;". A AST e descartada porque ha erro.
         */
        AstType type;
        AstDeclaration *declaration;
        AstStatement *statement;

        type.kind = AST_TYPE_INTEIRA;
        type.location = location_of_token(&parser->current);
        add_syntax_error(
            parser,
            "tipo de valor",
            "vazio so pode ser tipo de retorno de funcao"
        );
        if (!parser_advance(parser)) {
            return NULL;
        }
        declaration = parse_declaration_after_type(parser, type, true);
        if (declaration == NULL) {
            return NULL;
        }
        statement = make_statement(parser, AST_STMT_DECLARATION, type.location);
        if (statement == NULL) {
            ast_declaration_destroy(declaration);
            return NULL;
        }
        statement->as.declaration = declaration;
        return statement;
    }
    default:
        add_syntax_error(parser, "comando", MESSAGE_EXPECTED_STATEMENT);
        return NULL;
    }
}

/*
 * LVALUE -> ID INDICE?   com INDICE -> '{' EXPRESSAO '}'
 *
 * finish_lvalue recebe o nome ja consumido (e assume a posse de name).
 */
static AstLValue *finish_lvalue(
    Parser *parser,
    char *name,
    SourceLocation location
)
{
    AstLValue *lvalue;

    lvalue = parser_calloc(parser, 1, sizeof(*lvalue));
    if (lvalue == NULL) {
        free(name);
        return NULL;
    }
    lvalue->kind = AST_LVALUE_VARIABLE;
    lvalue->location = location;
    lvalue->name = name;

    if (parser_match(parser, TOK_ABRE_CHAVE)) {
        lvalue->kind = AST_LVALUE_VECTOR;
        lvalue->index = parse_expression(parser);
        parser_consume(parser, TOK_FECHA_CHAVE, "TOK_FECHA_CHAVE");
    }
    if (!parse_field_suffix(parser, &lvalue->field_name)) {
        ast_lvalue_destroy(lvalue);
        return NULL;
    }
    return lvalue;
}

static AstLValue *parse_lvalue(Parser *parser)
{
    SourceLocation location;
    char *name;

    if (!parser_check(parser, TOK_ID)) {
        add_syntax_error(parser, "TOK_ID", MESSAGE_UNEXPECTED);
        return NULL;
    }

    location = location_of_token(&parser->current);
    name = copy_token_lexeme(parser, &parser->current);
    if (name == NULL) {
        return NULL;
    }
    if (!parser_advance(parser)) {
        free(name);
        return NULL;
    }
    return finish_lvalue(parser, name, location);
}

/* Completa "LVALUE 'receba' EXPRESSAO" a partir de um lvalue ja lido. */
static AstAssignment *finish_assignment(Parser *parser, AstLValue *lvalue)
{
    AstAssignment *assignment;

    assignment = parser_calloc(parser, 1, sizeof(*assignment));
    if (assignment == NULL) {
        ast_lvalue_destroy(lvalue);
        return NULL;
    }
    assignment->location = lvalue->location;
    assignment->target = lvalue;

    if (!parser_check(parser, TOK_RECEBA)) {
        add_syntax_error(
            parser,
            "TOK_RECEBA",
            lvalue->kind == AST_LVALUE_VARIABLE
                ? MESSAGE_ID_STATEMENT
                : MESSAGE_UNEXPECTED
        );
        ast_assignment_destroy(assignment);
        return NULL;
    }
    if (!parser_advance(parser)) {
        ast_assignment_destroy(assignment);
        return NULL;
    }
    assignment->value = parse_expression(parser);
    if (assignment->value == NULL) {
        ast_assignment_destroy(assignment);
        return NULL;
    }
    return assignment;
}

/* Atribuicao sem ';' (usada tambem no cabecalho de repete). */
static AstAssignment *parse_assignment_core(Parser *parser)
{
    AstLValue *lvalue = parse_lvalue(parser);

    if (lvalue == NULL) {
        return NULL;
    }
    return finish_assignment(parser, lvalue);
}

/*
 * 'cond' '(' CONDICAO ')' 'edai' BLOCO seguido de zero ou mais
 * 'casocontrario' 'cond' ... e, opcionalmente, 'casocontrario' 'edai'
 * BLOCO, que encerra a cadeia.
 */
static AstIf *parse_if_after_keyword(Parser *parser, SourceLocation location)
{
    AstIf *if_statement;

    parser_advance(parser);
    if_statement = parser_calloc(parser, 1, sizeof(*if_statement));
    if (if_statement == NULL) {
        return NULL;
    }
    if_statement->location = location;

    parser_consume(parser, TOK_ABRE_PAREN, "TOK_ABRE_PAREN");
    if_statement->condition = parse_expression(parser);
    parser_consume(parser, TOK_FECHA_PAREN, "TOK_FECHA_PAREN");
    parser_consume(parser, TOK_EDAI, "TOK_EDAI");
    if_statement->then_block = parse_block(parser);

    while (!parser->fatal && parser_check(parser, TOK_CASOCONTRARIO)) {
        SourceLocation branch_location =
            location_of_token(&parser->current);

        parser_advance(parser);
        if (parser_match(parser, TOK_COND)) {
            AstIfBranch *branch = parser_calloc(
                parser,
                1,
                sizeof(*branch)
            );
            if (branch == NULL) {
                break;
            }
            branch->location = branch_location;
            parser_consume(parser, TOK_ABRE_PAREN, "TOK_ABRE_PAREN");
            branch->condition = parse_expression(parser);
            parser_consume(parser, TOK_FECHA_PAREN, "TOK_FECHA_PAREN");
            parser_consume(parser, TOK_EDAI, "TOK_EDAI");
            branch->block = parse_block(parser);

            if (!ast_if_append_else_if(if_statement, branch)) {
                ast_if_branch_destroy(branch);
                parser->fatal = true;
                break;
            }
        } else if (parser_match(parser, TOK_EDAI)) {
            if_statement->else_block = parse_block(parser);
            break;
        } else {
            add_syntax_error(
                parser,
                "TOK_COND ou TOK_EDAI",
                MESSAGE_UNEXPECTED
            );
            synchronize_statement(parser);
            break;
        }
    }
    return if_statement;
}

static AstExpression *make_unary_expression(
    Parser *parser,
    AstUnaryOp op,
    SourceLocation location,
    AstExpression *operand
)
{
    AstExpression *expression = make_expression(
        parser,
        AST_EXPR_UNARY,
        location
    );

    if (expression == NULL) {
        ast_expression_destroy(operand);
        return NULL;
    }
    expression->as.unary.op = op;
    expression->as.unary.operand = operand;
    if (!set_expression_height(parser, expression, expression_height(operand))) {
        ast_expression_destroy(expression);
        return NULL;
    }
    return expression;
}

static AstExpression *make_binary_expression(
    Parser *parser,
    AstBinaryOp op,
    SourceLocation location,
    AstExpression *left,
    AstExpression *right
)
{
    AstExpression *expression = make_expression(
        parser,
        AST_EXPR_BINARY,
        location
    );

    if (expression == NULL) {
        ast_expression_destroy(left);
        ast_expression_destroy(right);
        return NULL;
    }
    expression->as.binary.op = op;
    expression->as.binary.left = left;
    expression->as.binary.right = right;
    {
        size_t left_height = expression_height(left);
        size_t right_height = expression_height(right);

        if (!set_expression_height(
                parser,
                expression,
                left_height > right_height ? left_height : right_height
            )) {
            ast_expression_destroy(expression);
            return NULL;
        }
    }
    return expression;
}

static bool comparison_operator(
    TokenType type,
    AstBinaryOp *operator
)
{
    if (operator == NULL) {
        return false;
    }

    switch (type) {
    case TOK_MENOR:
        *operator = AST_BINARY_LESS;
        return true;
    case TOK_MAIOR:
        *operator = AST_BINARY_GREATER;
        return true;
    case TOK_IGUAL_IGUAL:
        *operator = AST_BINARY_EQUAL;
        return true;
    case TOK_DIFERENTE:
        *operator = AST_BINARY_DIFFERENT;
        return true;
    default:
        return false;
    }
}

static bool additive_operator(TokenType type, AstBinaryOp *operator)
{
    if (operator == NULL) {
        return false;
    }
    if (type == TOK_MAIS) {
        *operator = AST_BINARY_ADD;
        return true;
    }
    if (type == TOK_MENOS) {
        *operator = AST_BINARY_SUBTRACT;
        return true;
    }
    return false;
}

static bool multiplicative_operator(
    TokenType type,
    AstBinaryOp *operator
)
{
    if (operator == NULL) {
        return false;
    }
    switch (type) {
    case TOK_MULT:
        *operator = AST_BINARY_MULTIPLY;
        return true;
    case TOK_DIV:
        *operator = AST_BINARY_DIVIDE;
        return true;
    case TOK_DIV_INT:
        *operator = AST_BINARY_DIV_INT;
        return true;
    default:
        return false;
    }
}

/*
 * EXPRESSAO -> COND_OU. Ponto de entrada de toda expressao (parenteses,
 * argumentos, indices, condicoes); por isso conta um nivel de aninhamento.
 */
static AstExpression *parse_expression(Parser *parser)
{
    AstExpression *expression;

    if (!parser_enter(parser)) {
        return NULL;
    }
    expression = parse_or(parser);
    parser_leave(parser);
    return expression;
}

static AstExpression *parse_or(Parser *parser)
{
    AstExpression *left = parse_xor(parser);

    while (left != NULL && parser_match(parser, TOK_OU)) {
        SourceLocation location = left->location;
        AstExpression *right = parse_xor(parser);

        if (right == NULL) {
            ast_expression_destroy(left);
            return NULL;
        }
        left = make_binary_expression(
            parser,
            AST_BINARY_OR,
            location,
            left,
            right
        );
    }
    return left;
}

static AstExpression *parse_xor(Parser *parser)
{
    AstExpression *left = parse_and(parser);

    while (left != NULL && parser_match(parser, TOK_XOR)) {
        SourceLocation location = left->location;
        AstExpression *right = parse_and(parser);

        if (right == NULL) {
            ast_expression_destroy(left);
            return NULL;
        }
        left = make_binary_expression(
            parser,
            AST_BINARY_XOR,
            location,
            left,
            right
        );
    }
    return left;
}

static AstExpression *parse_and(Parser *parser)
{
    AstExpression *left = parse_not(parser);

    while (left != NULL && parser_match(parser, TOK_E)) {
        SourceLocation location = left->location;
        AstExpression *right = parse_not(parser);

        if (right == NULL) {
            ast_expression_destroy(left);
            return NULL;
        }
        left = make_binary_expression(
            parser,
            AST_BINARY_AND,
            location,
            left,
            right
        );
    }
    return left;
}

static AstExpression *parse_not(Parser *parser)
{
    if (parser_check(parser, TOK_NAO)) {
        SourceLocation location = location_of_token(&parser->current);
        AstExpression *operand;

        parser_advance(parser);
        if (!parser_enter(parser)) {
            return NULL;
        }
        operand = parse_not(parser);
        parser_leave(parser);
        if (operand == NULL) {
            return NULL;
        }
        return make_unary_expression(
            parser,
            AST_UNARY_NOT,
            location,
            operand
        );
    }
    return parse_atom(parser);
}

/*
 * COND_ATOMO -> VALOR (COMPARADOR VALOR)?  A comparacao nao e
 * associativa: 'a < b < c' e erro sintatico.
 */
static AstExpression *parse_atom(Parser *parser)
{
    AstExpression *left = parse_value(parser);
    AstBinaryOp operator;

    if (left == NULL || !parser->has_current ||
        !comparison_operator(parser->current.type, &operator)) {
        return left;
    }

    {
        SourceLocation location = left->location;
        AstExpression *right;

        parser_advance(parser);
        right = parse_value(parser);
        if (right == NULL) {
            ast_expression_destroy(left);
            return NULL;
        }
        return make_binary_expression(
            parser,
            operator,
            location,
            left,
            right
        );
    }
}

static AstExpression *parse_value(Parser *parser)
{
    if (parser_check(parser, TOK_VER) ||
        parser_check(parser, TOK_FAL) ||
        parser_check(parser, TOK_TEXTO)) {
        return parse_literal(parser);
    }
    return parse_arithmetic(parser);
}

/* ARITMETICA -> TERMO (('+' | '-') TERMO)*, associativa a esquerda. */
static AstExpression *parse_arithmetic(Parser *parser)
{
    AstExpression *left = parse_term(parser);

    while (left != NULL && parser->has_current) {
        AstBinaryOp operator;
        SourceLocation location;
        AstExpression *right;

        if (!additive_operator(parser->current.type, &operator)) {
            break;
        }
        location = left->location;
        parser_advance(parser);
        right = parse_term(parser);
        if (right == NULL) {
            ast_expression_destroy(left);
            return NULL;
        }
        left = make_binary_expression(
            parser,
            operator,
            location,
            left,
            right
        );
    }
    return left;
}

/* TERMO -> POTENCIA (('*' | '/' | '//') POTENCIA)*, a esquerda. */
static AstExpression *parse_term(Parser *parser)
{
    AstExpression *left = parse_power(parser);

    while (left != NULL && parser->has_current) {
        AstBinaryOp operator;
        SourceLocation location;
        AstExpression *right;

        if (!multiplicative_operator(parser->current.type, &operator)) {
            break;
        }
        location = left->location;
        parser_advance(parser);
        right = parse_power(parser);
        if (right == NULL) {
            ast_expression_destroy(left);
            return NULL;
        }
        left = make_binary_expression(
            parser,
            operator,
            location,
            left,
            right
        );
    }
    return left;
}

/* POTENCIA -> UNARIO ('^' POTENCIA)?, associativa a direita. */
static AstExpression *parse_power(Parser *parser)
{
    AstExpression *left = parse_unary(parser);

    if (left != NULL && parser_match(parser, TOK_POTENCIA)) {
        SourceLocation location = left->location;
        AstExpression *right = NULL;

        /* Potencia e associativa a direita: a recursao conta aninhamento. */
        if (parser_enter(parser)) {
            right = parse_power(parser);
            parser_leave(parser);
        }
        if (right == NULL) {
            ast_expression_destroy(left);
            return NULL;
        }
        return make_binary_expression(
            parser,
            AST_BINARY_POWER,
            location,
            left,
            right
        );
    }
    return left;
}

/* UNARIO -> ('+' | '-') UNARIO | PRIMARIO */
static AstExpression *parse_unary(Parser *parser)
{
    if (parser_check(parser, TOK_MAIS) ||
        parser_check(parser, TOK_MENOS)) {
        TokenType type = parser->current.type;
        SourceLocation location = location_of_token(&parser->current);
        AstExpression *operand;

        parser_advance(parser);
        if (!parser_enter(parser)) {
            return NULL;
        }
        operand = parse_unary(parser);
        parser_leave(parser);
        if (operand == NULL) {
            return NULL;
        }
        return make_unary_expression(
            parser,
            type == TOK_MAIS ? AST_UNARY_PLUS : AST_UNARY_MINUS,
            location,
            operand
        );
    }
    return parse_primary(parser);
}

static AstExpression *parse_literal(Parser *parser)
{
    AstExpression *expression;
    SourceLocation location;
    AstLiteralKind kind;

    if (!parser_ensure_current(parser)) {
        return NULL;
    }
    location = location_of_token(&parser->current);
    switch (parser->current.type) {
    case TOK_NUM_INTEIRO:
        kind = AST_LITERAL_INTEGER;
        break;
    case TOK_NUM_DECIMAL:
        kind = AST_LITERAL_DECIMAL;
        break;
    case TOK_TEXTO:
        kind = AST_LITERAL_TEXT;
        break;
    case TOK_VER:
    case TOK_FAL:
        kind = AST_LITERAL_BOOLEAN;
        break;
    default:
        add_syntax_error(parser, "literal", MESSAGE_EXPECTED_EXPRESSION);
        return NULL;
    }

    expression = make_expression(parser, AST_EXPR_LITERAL, location);
    if (expression == NULL) {
        return NULL;
    }
    expression->as.literal.kind = kind;
    expression->as.literal.lexeme =
        copy_token_lexeme(parser, &parser->current);
    if (expression->as.literal.lexeme == NULL) {
        ast_expression_destroy(expression);
        return NULL;
    }
    expression->as.literal.lexeme_length =
        parser->current.lexeme_length;

    if (!parser_advance(parser)) {
        ast_expression_destroy(expression);
        return NULL;
    }
    return expression;
}

static AstExpression *parse_root(Parser *parser)
{
    AstExpression *expression;
    SourceLocation location = location_of_token(&parser->current);

    expression = make_expression(parser, AST_EXPR_ROOT, location);
    if (expression == NULL) {
        return NULL;
    }
    parser_advance(parser);
    parser_consume(parser, TOK_ABRE_PAREN, "TOK_ABRE_PAREN");
    expression->as.root.argument = parse_expression(parser);
    parser_consume(parser, TOK_FECHA_PAREN, "TOK_FECHA_PAREN");

    if (expression->as.root.argument == NULL && parser->fatal) {
        ast_expression_destroy(expression);
        return NULL;
    }
    if (!set_expression_height(
            parser,
            expression,
            expression_height(expression->as.root.argument)
        )) {
        ast_expression_destroy(expression);
        return NULL;
    }
    return expression;
}

static bool parse_argument_list(Parser *parser, AstReference *reference)
{
    if (parser_check(parser, TOK_FECHA_PAREN)) {
        return true;
    }

    while (!parser->fatal && parser->has_current &&
           !parser_at_end(parser)) {
        AstExpression *argument = parse_expression(parser);

        if (argument == NULL) {
            if (!parser->fatal) {
                if (synchronize_parenthesized(parser) &&
                    !parser_check(parser, TOK_FECHA_PAREN) &&
                    !parser_at_end(parser)) {
                    continue;
                }
            }
            return false;
        }
        if (!ast_reference_append_argument(reference, argument)) {
            ast_expression_destroy(argument);
            parser->fatal = true;
            return false;
        }
        if (!parser_match(parser, TOK_VIRGULA)) {
            break;
        }
        if (parser_check(parser, TOK_FECHA_PAREN)) {
            add_syntax_error(
                parser,
                "expressao",
                "virgula final nao e permitida"
            );
            return false;
        }
    }
    return true;
}

/*
 * REFERENCIA -> ID ( '(' ARGUMENTOS? ')' | '{' EXPRESSAO '}' )?
 *
 * finish_reference recebe o nome ja consumido e assume sua posse; e usada
 * tanto em expressoes quanto no comando de chamada (D33).
 */
static AstExpression *finish_reference(
    Parser *parser,
    char *name,
    SourceLocation location
)
{
    AstExpression *expression;
    size_t tallest = 0;

    expression = make_expression(parser, AST_EXPR_REFERENCE, location);
    if (expression == NULL) {
        free(name);
        return NULL;
    }
    expression->as.reference.kind = AST_REFERENCE_VARIABLE;
    expression->as.reference.location = location;
    expression->as.reference.name = name;

    if (parser_match(parser, TOK_ABRE_PAREN)) {
        size_t i;

        expression->as.reference.kind = AST_REFERENCE_CALL;
        parse_argument_list(parser, &expression->as.reference);
        parser_consume(parser, TOK_FECHA_PAREN, "TOK_FECHA_PAREN");
        for (i = 0; i < expression->as.reference.as.call.argument_count; i++) {
            size_t height = expression_height(
                expression->as.reference.as.call.arguments[i]
            );
            if (height > tallest) {
                tallest = height;
            }
        }
    } else if (parser_match(parser, TOK_ABRE_CHAVE)) {
        expression->as.reference.kind = AST_REFERENCE_VECTOR;
        expression->as.reference.as.vector.index = parse_expression(parser);
        parser_consume(parser, TOK_FECHA_CHAVE, "TOK_FECHA_CHAVE");
        tallest = expression_height(expression->as.reference.as.vector.index);
    }
    if (expression->as.reference.kind != AST_REFERENCE_CALL &&
        !parse_field_suffix(parser, &expression->as.reference.field_name)) {
        ast_expression_destroy(expression);
        return NULL;
    }
    if (!set_expression_height(parser, expression, tallest)) {
        ast_expression_destroy(expression);
        return NULL;
    }
    return expression;
}

static AstExpression *parse_reference(Parser *parser)
{
    SourceLocation location = location_of_token(&parser->current);
    char *name;

    name = copy_token_lexeme(parser, &parser->current);
    if (name == NULL) {
        return NULL;
    }
    if (!parser_advance(parser)) {
        free(name);
        return NULL;
    }
    return finish_reference(parser, name, location);
}

/* PRIMARIO -> NUM | 'raiz' '(' EXPRESSAO ')' | '(' EXPRESSAO ')' | REFERENCIA */
static AstExpression *parse_primary(Parser *parser)
{
    if (!parser_ensure_current(parser)) {
        return NULL;
    }

    if (parser_check(parser, TOK_NUM_INTEIRO) ||
        parser_check(parser, TOK_NUM_DECIMAL)) {
        return parse_literal(parser);
    }
    if (parser_check(parser, TOK_RAIZ)) {
        return parse_root(parser);
    }
    if (parser_match(parser, TOK_ABRE_PAREN)) {
        AstExpression *expression = parse_expression(parser);
        parser_consume(parser, TOK_FECHA_PAREN, "TOK_FECHA_PAREN");
        return expression;
    }
    if (parser_check(parser, TOK_ID)) {
        return parse_reference(parser);
    }

    add_syntax_error(parser, "expressao", MESSAGE_EXPECTED_EXPRESSION);
    return NULL;
}

static void destroy_diagnostics(Diagnostic *diagnostics, size_t count)
{
    size_t i;

    if (diagnostics == NULL) {
        return;
    }
    for (i = 0; i < count; i++) {
        free(diagnostics[i].found_lexeme);
    }
    free(diagnostics);
}

static void transfer_diagnostics(Parser *parser, ParseResult *result)
{
    result->diagnostics = parser->diagnostics;
    result->diagnostic_count = parser->diagnostic_count;
    result->lexical_error_count = parser->lexical_error_count;
    result->syntax_error_count = parser->syntax_error_count;
    result->suppressed_syntax_error_count =
        parser->suppressed_syntax_error_count;

    parser->diagnostics = NULL;
    parser->diagnostic_count = 0;
    parser->diagnostic_capacity = 0;
    parser->lexical_error_count = 0;
    parser->syntax_error_count = 0;
    parser->suppressed_syntax_error_count = 0;
}

bool parser_init(Parser *parser, const char *source, size_t length)
{
    if (parser == NULL || source == NULL ||
        parser->initialized ||
        parser->lexer.source != NULL ||
        parser->has_current ||
        parser->program != NULL ||
        parser->diagnostics != NULL) {
        return false;
    }

    if (!lexer_init(&parser->lexer, source, length)) {
        return false;
    }
    parser->initialized = true;
    return true;
}

bool parser_init_from_string(Parser *parser, const char *source)
{
    if (source == NULL) {
        return false;
    }
    return parser_init(parser, source, strlen(source));
}

ParseResult parser_parse(Parser *parser)
{
    ParseResult result = {0};

    if (parser == NULL || !parser->initialized) {
        result.status = PARSE_INTERNAL_FAILURE;
        return result;
    }

    if (parser->parsed) {
        result.status = PARSE_INTERNAL_FAILURE;
        return result;
    }
    parser->parsed = true;

    if (!parser_ensure_current(parser)) {
        result.status = PARSE_INTERNAL_FAILURE;
        transfer_diagnostics(parser, &result);
        return result;
    }

    parse_program(parser);
    if (parser->fatal) {
        ast_program_destroy(parser->program);
        parser->program = NULL;
        result.status = PARSE_INTERNAL_FAILURE;
        transfer_diagnostics(parser, &result);
        return result;
    }

    if (parser->lexical_error_count != 0 ||
        parser->syntax_error_count != 0) {
        ast_program_destroy(parser->program);
        parser->program = NULL;
        result.status = PARSE_INPUT_ERRORS;
        transfer_diagnostics(parser, &result);
        return result;
    }

    result.status = PARSE_OK;
    result.ast = parser->program;
    parser->program = NULL;
    transfer_diagnostics(parser, &result);
    return result;
}

void parser_destroy(Parser *parser)
{
    if (parser == NULL) {
        return;
    }

    if (parser->has_current) {
        token_destroy(&parser->current);
        parser->has_current = false;
    }
    lexer_destroy(&parser->lexer);
    ast_program_destroy(parser->program);
    destroy_diagnostics(parser->diagnostics, parser->diagnostic_count);
    *parser = (Parser){0};
}

void parse_result_destroy(ParseResult *result)
{
    if (result == NULL) {
        return;
    }

    ast_program_destroy(result->ast);
    destroy_diagnostics(result->diagnostics, result->diagnostic_count);
    *result = (ParseResult){0};
}
