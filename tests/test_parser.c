#include "parser.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t tests_run;
static size_t tests_failed;

#define CHECK(condition) do { \
    tests_run++; \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        tests_failed++; \
        return false; \
    } \
} while (0)

static bool parse_ok(const char *source, ParseResult *result)
{
    Parser parser = {0};

    if (!parser_init_from_string(&parser, source)) {
        return false;
    }
    *result = parser_parse(&parser);
    parser_destroy(&parser);
    return result->status == PARSE_OK &&
           result->ast != NULL &&
           result->lexical_error_count == 0 &&
           result->syntax_error_count == 0;
}

static bool parse_input_error(const char *source, ParseResult *result)
{
    Parser parser = {0};

    if (!parser_init_from_string(&parser, source)) {
        return false;
    }
    *result = parser_parse(&parser);
    parser_destroy(&parser);
    return result->status == PARSE_INPUT_ERRORS &&
           result->ast == NULL &&
           result->diagnostic_count > 0;
}

static bool has_expected_diagnostic(
    const ParseResult *result,
    const char *expected
)
{
    size_t i;

    if (result == NULL || expected == NULL) {
        return false;
    }
    for (i = 0; i < result->diagnostic_count; i++) {
        if (result->diagnostics[i].kind == DIAG_SYNTAX &&
            result->diagnostics[i].expected != NULL &&
            strcmp(result->diagnostics[i].expected, expected) == 0) {
            return true;
        }
    }
    return false;
}

static bool test_empty_program_and_lifecycle(void)
{
    Parser parser = {0};
    ParseResult result;

    CHECK(parser_init_from_string(&parser, ""));
    result = parser_parse(&parser);
    CHECK(result.status == PARSE_OK);
    CHECK(result.ast != NULL);
    CHECK(result.ast->count == 0);
    CHECK(result.diagnostic_count == 0);
    parse_result_destroy(&result);
    parse_result_destroy(&result);
    CHECK(!parser_init_from_string(&parser, "inteira y;"));
    parser_destroy(&parser);

    CHECK(parser_init_from_string(&parser, "inteira x;"));
    result = parser_parse(&parser);
    CHECK(result.status == PARSE_OK);
    parse_result_destroy(&result);
    parser_destroy(&parser);
    parser_destroy(&parser);

    {
        Parser before_parse = {0};
        CHECK(parser_init_from_string(&before_parse, "inteira z;"));
        parser_destroy(&before_parse);
        parser_destroy(&before_parse);
    }
    return true;
}

static bool test_global_declarations_and_vectors(void)
{
    ParseResult result = {0};
    AstDeclaration *declaration;
    AstDeclarator *vector;

    CHECK(parse_ok(
        "inteira x, y, vet{5} valores;",
        &result
    ));
    CHECK(result.ast->count == 1);
    CHECK(result.ast->items[0]->kind == AST_TOP_DECLARATION);
    declaration = result.ast->items[0]->as.declaration;
    CHECK(declaration->type.kind == AST_TYPE_INTEIRA);
    CHECK(declaration->declarator_count == 3);
    CHECK(declaration->declarators[0]->kind == AST_DECLARATOR_SIMPLE);
    CHECK(strcmp(declaration->declarators[0]->name, "x") == 0);
    CHECK(declaration->declarators[1]->kind == AST_DECLARATOR_SIMPLE);
    CHECK(strcmp(declaration->declarators[1]->name, "y") == 0);
    vector = declaration->declarators[2];
    CHECK(vector->kind == AST_DECLARATOR_VECTOR);
    CHECK(strcmp(vector->name, "valores") == 0);
    CHECK(vector->vector_size_length == 1);
    CHECK(strcmp(vector->vector_size_lexeme, "5") == 0);
    parse_result_destroy(&result);
    return true;
}

static bool test_records_and_functions(void)
{
    const char *source =
        "registro Pessoa @ ["
        "inteira idade;"
        "palavra nome;"
        "]"
        "outrafuncao inteira f(inteira n, flut y) @ ["
        "respost n + y;"
        "]"
        "vazio principal() @ []";
    ParseResult result = {0};
    AstRecordDecl *record;
    AstFunctionDecl *function;
    AstFunctionDecl *main_function;
    AstStatement *statement;

    CHECK(parse_ok(source, &result));
    CHECK(result.ast->count == 3);

    CHECK(result.ast->items[0]->kind == AST_TOP_RECORD);
    record = result.ast->items[0]->as.record;
    CHECK(strcmp(record->name, "Pessoa") == 0);
    CHECK(record->field_count == 2);
    CHECK(record->fields[0]->type.kind == AST_TYPE_INTEIRA);
    CHECK(strcmp(record->fields[1]->name, "nome") == 0);

    CHECK(result.ast->items[1]->kind == AST_TOP_FUNCTION);
    function = result.ast->items[1]->as.function;
    CHECK(!function->is_main);
    CHECK(strcmp(function->name, "f") == 0);
    CHECK(function->return_type.kind == AST_TYPE_INTEIRA);
    CHECK(function->parameter_count == 2);
    CHECK(strcmp(function->parameters[0]->name, "n") == 0);
    CHECK(function->body->statement_count == 1);
    statement = function->body->statements[0];
    CHECK(statement->kind == AST_STMT_RETURN);
    CHECK(statement->as.return_statement->value->kind == AST_EXPR_BINARY);

    CHECK(result.ast->items[2]->kind == AST_TOP_FUNCTION);
    main_function = result.ast->items[2]->as.function;
    CHECK(main_function->is_main);
    CHECK(main_function->name == NULL);
    CHECK(main_function->return_type.kind == AST_TYPE_VAZIO);
    CHECK(main_function->parameter_count == 0);
    CHECK(main_function->body->statement_count == 0);
    parse_result_destroy(&result);
    return true;
}

static bool test_statements_and_control_flow(void)
{
    const char *source =
        "inteira principal() @ ["
        "inteira x;"
        "x receba 1;"
        "cond (x > 0) edai [x receba x + 1;]"
        "casocontrario cond (x == 0) edai []"
        "casocontrario edai []"
        "durante (x < 10) edai []"
        "repete (x receba 0, x < 10, x receba x + 1) edai []"
        "[respost x;]"
        "]";
    ParseResult result = {0};
    AstBlock *body;
    AstIf *if_statement;

    CHECK(parse_ok(source, &result));
    CHECK(result.ast->items[0]->kind == AST_TOP_FUNCTION);
    body = result.ast->items[0]->as.function->body;
    CHECK(body->statement_count == 6);
    CHECK(body->statements[0]->kind == AST_STMT_DECLARATION);
    CHECK(body->statements[1]->kind == AST_STMT_ASSIGNMENT);
    CHECK(body->statements[2]->kind == AST_STMT_IF);
    CHECK(body->statements[3]->kind == AST_STMT_WHILE);
    CHECK(body->statements[4]->kind == AST_STMT_REPEAT);
    CHECK(body->statements[5]->kind == AST_STMT_BLOCK);

    if_statement = body->statements[2]->as.if_statement;
    CHECK(if_statement->else_if_count == 1);
    CHECK(if_statement->else_block != NULL);
    CHECK(if_statement->then_block->statement_count == 1);
    CHECK(body->statements[4]->as.repeat_statement->initialization != NULL);
    CHECK(body->statements[4]->as.repeat_statement->update != NULL);
    CHECK(body->statements[5]->as.block->statement_count == 1);
    parse_result_destroy(&result);
    return true;
}

static bool test_expression_precedence(void)
{
    const char *source =
        "inteira principal() @ ["
        "x receba 1 + 2 * 3;"
        "y receba 2 ^ 3 ^ 4;"
        "z receba -a ^ b;"
        "]";
    ParseResult result = {0};
    AstBlock *body;
    AstExpression *expression;
    AstExpression *right;

    CHECK(parse_ok(source, &result));
    CHECK(result.ast->items[0]->kind == AST_TOP_FUNCTION);
    body = result.ast->items[0]->as.function->body;

    CHECK(body->statements[0]->kind == AST_STMT_ASSIGNMENT);
    expression = body->statements[0]->as.assignment->value;
    CHECK(expression->kind == AST_EXPR_BINARY);
    CHECK(expression->as.binary.op == AST_BINARY_ADD);
    CHECK(expression->as.binary.right->kind == AST_EXPR_BINARY);
    CHECK(expression->as.binary.right->as.binary.op == AST_BINARY_MULTIPLY);

    CHECK(body->statements[1]->kind == AST_STMT_ASSIGNMENT);
    expression = body->statements[1]->as.assignment->value;
    CHECK(expression->as.binary.op == AST_BINARY_POWER);
    right = expression->as.binary.right;
    CHECK(right->kind == AST_EXPR_BINARY);
    CHECK(right->as.binary.op == AST_BINARY_POWER);

    CHECK(body->statements[2]->kind == AST_STMT_ASSIGNMENT);
    expression = body->statements[2]->as.assignment->value;
    CHECK(expression->as.binary.op == AST_BINARY_POWER);
    CHECK(expression->as.binary.left->kind == AST_EXPR_UNARY);
    CHECK(expression->as.binary.left->as.unary.op == AST_UNARY_MINUS);
    parse_result_destroy(&result);
    return true;
}

static bool test_logical_calls_vectors_and_root(void)
{
    const char *source =
        "inteira principal() @ ["
        "x receba NÃO a < b E c XOR d OU e;"
        "y receba f(1 + 2, raiz(x));"
        "z receba vetor{i + 1};"
        "w receba NÃO (a == b);"
        "]";
    ParseResult result = {0};
    AstBlock *body;
    AstExpression *expression;
    AstExpression *call_expression;
    AstExpression *root_expression;

    CHECK(parse_ok(source, &result));
    CHECK(result.ast->items[0]->kind == AST_TOP_FUNCTION);
    body = result.ast->items[0]->as.function->body;

    CHECK(body->statements[0]->kind == AST_STMT_ASSIGNMENT);
    expression = body->statements[0]->as.assignment->value;
    CHECK(expression->kind == AST_EXPR_BINARY);
    CHECK(expression->as.binary.op == AST_BINARY_OR);
    CHECK(expression->as.binary.left->as.binary.op == AST_BINARY_XOR);
    CHECK(
        expression->as.binary.left->as.binary.left->as.binary.op ==
        AST_BINARY_AND
    );
    CHECK(
        expression->as.binary.left->as.binary.left->as.binary.left->kind ==
        AST_EXPR_UNARY
    );
    CHECK(
        expression->as.binary.left->as.binary.left->as.binary.left->
            as.unary.op == AST_UNARY_NOT
    );

    CHECK(body->statements[1]->kind == AST_STMT_ASSIGNMENT);
    call_expression = body->statements[1]->as.assignment->value;
    CHECK(call_expression->kind == AST_EXPR_REFERENCE);
    CHECK(call_expression->as.reference.kind == AST_REFERENCE_CALL);
    CHECK(call_expression->as.reference.as.call.argument_count == 2);
    root_expression =
        call_expression->as.reference.as.call.arguments[1];
    CHECK(root_expression->kind == AST_EXPR_ROOT);
    CHECK(root_expression->as.root.argument != NULL);

    CHECK(body->statements[2]->kind == AST_STMT_ASSIGNMENT);
    expression = body->statements[2]->as.assignment->value;
    CHECK(expression->kind == AST_EXPR_REFERENCE);
    CHECK(expression->as.reference.kind == AST_REFERENCE_VECTOR);
    CHECK(expression->as.reference.as.vector.index->kind ==
          AST_EXPR_BINARY);

    CHECK(body->statements[3]->kind == AST_STMT_ASSIGNMENT);
    expression = body->statements[3]->as.assignment->value;
    CHECK(expression->kind == AST_EXPR_UNARY);
    CHECK(expression->as.unary.op == AST_UNARY_NOT);
    CHECK(expression->as.unary.operand->as.binary.op == AST_BINARY_EQUAL);
    parse_result_destroy(&result);
    return true;
}

static bool test_io_statements_and_ast(void)
{
    const char *source =
        "inteira principal() @[\n"
        "inteira x;\n"
        "inteira vet{5} v;\n"
        "inteira i;\n"
        "le(x);\n"
        "le(v{i});\n"
        "imprime(x);\n"
        "imprime(x + 1);\n"
        "imprime(soma(1, 2));\n"
        "imprime(\"texto\");\n"
        "imprime(VER);\n"
        "cond (VER) edai [imprime(1);]\n"
        "durante (FAL) edai [imprime(2);]\n"
        "repete (i receba 0, i < 1, i receba i + 1) edai ["
            "imprime(i);]\n"
        "[imprime(3);]\n"
        "respost 0;\n"
        "]"
        "outrafuncao inteira f(inteira x) @[le(x);imprime(x);respost x;]";
    ParseResult result = {0};
    AstBlock *body;
    AstStatement *statement;
    AstIo *io;
    AstExpression *expression;

    CHECK(parse_ok(source, &result));
    CHECK(result.ast->count == 2);
    body = result.ast->items[0]->as.function->body;
    CHECK(body->statement_count == 15);

    statement = body->statements[3];
    CHECK(statement->kind == AST_STMT_IO);
    CHECK(statement->location.line == 5);
    CHECK(statement->location.column == 1);
    CHECK(statement->as.io != NULL);
    io = statement->as.io;
    CHECK(io->kind == AST_IO_READ);
    CHECK(io->location.line == 5);
    CHECK(io->location.column == 1);
    CHECK(io->as.target != NULL);
    CHECK(io->as.target->kind == AST_LVALUE_VARIABLE);
    CHECK(strcmp(io->as.target->name, "x") == 0);
    CHECK(io->as.target->location.line == 5);
    CHECK(io->as.target->location.column == 4);
    CHECK(io->as.target->index == NULL);

    statement = body->statements[4];
    CHECK(statement->kind == AST_STMT_IO);
    io = statement->as.io;
    CHECK(io->kind == AST_IO_READ);
    CHECK(io->location.line == 6);
    CHECK(io->as.target != NULL);
    CHECK(io->as.target->kind == AST_LVALUE_VECTOR);
    CHECK(strcmp(io->as.target->name, "v") == 0);
    CHECK(io->as.target->index != NULL);
    CHECK(io->as.target->index->kind == AST_EXPR_REFERENCE);
    CHECK(io->as.target->index->as.reference.kind == AST_REFERENCE_VARIABLE);
    CHECK(strcmp(io->as.target->index->as.reference.name, "i") == 0);

    statement = body->statements[5];
    CHECK(statement->kind == AST_STMT_IO);
    io = statement->as.io;
    CHECK(io->kind == AST_IO_WRITE);
    CHECK(statement->location.line == 7);
    CHECK(statement->location.column == 1);
    CHECK(io->location.line == 7);
    CHECK(io->location.column == 1);
    CHECK(io->as.value != NULL);
    CHECK(io->as.value->kind == AST_EXPR_REFERENCE);
    CHECK(strcmp(io->as.value->as.reference.name, "x") == 0);

    statement = body->statements[6];
    CHECK(statement->kind == AST_STMT_IO);
    expression = statement->as.io->as.value;
    CHECK(statement->as.io->kind == AST_IO_WRITE);
    CHECK(expression->kind == AST_EXPR_BINARY);
    CHECK(expression->as.binary.op == AST_BINARY_ADD);

    statement = body->statements[7];
    expression = statement->as.io->as.value;
    CHECK(statement->kind == AST_STMT_IO);
    CHECK(statement->as.io->kind == AST_IO_WRITE);
    CHECK(expression->kind == AST_EXPR_REFERENCE);
    CHECK(expression->as.reference.kind == AST_REFERENCE_CALL);
    CHECK(expression->as.reference.as.call.argument_count == 2);

    statement = body->statements[8];
    CHECK(statement->kind == AST_STMT_IO);
    CHECK(statement->as.io->as.value->kind == AST_EXPR_LITERAL);
    CHECK(statement->as.io->as.value->as.literal.kind == AST_LITERAL_TEXT);

    statement = body->statements[9];
    CHECK(statement->kind == AST_STMT_IO);
    CHECK(statement->as.io->as.value->kind == AST_EXPR_LITERAL);
    CHECK(statement->as.io->as.value->as.literal.kind == AST_LITERAL_BOOLEAN);

    CHECK(body->statements[10]->kind == AST_STMT_IF);
    CHECK(body->statements[10]->as.if_statement->then_block->statement_count == 1);
    CHECK(
        body->statements[10]->as.if_statement->then_block->statements[0]->kind ==
        AST_STMT_IO
    );
    CHECK(body->statements[11]->kind == AST_STMT_WHILE);
    CHECK(body->statements[11]->as.while_statement->body->statement_count == 1);
    CHECK(body->statements[12]->kind == AST_STMT_REPEAT);
    CHECK(body->statements[12]->as.repeat_statement->body->statement_count == 1);
    CHECK(body->statements[13]->kind == AST_STMT_BLOCK);
    CHECK(body->statements[13]->as.block->statements[0]->kind == AST_STMT_IO);
    CHECK(result.ast->items[1]->kind == AST_TOP_FUNCTION);
    CHECK(result.ast->items[1]->as.function->body->statement_count == 3);
    CHECK(result.ast->items[1]->as.function->body->statements[0]->kind ==
          AST_STMT_IO);
    CHECK(result.ast->items[1]->as.function->body->statements[1]->kind ==
          AST_STMT_IO);
    parse_result_destroy(&result);
    return true;
}

static bool test_io_syntax_errors(void)
{
    static const char *sources[] = {
        "inteira principal() @[le(1);respost 0;]",
        "inteira principal() @[le(x + 1);respost 0;]",
        "inteira principal() @[le();respost 0;]",
        "inteira principal() @[le(x, y);respost 0;]",
        "inteira principal() @[imprime();respost 0;]",
        "inteira principal() @[imprime(x, y);respost 0;]",
        "inteira principal() @[le(x)respost 0;]",
        "inteira principal() @[imprime(x)respost 0;]",
        "inteira principal() @[le x;respost 0;]",
        "inteira principal() @[imprime x;respost 0;]",
        "inteira principal() @[soma(1, 2) respost 0;]",
        "le(x);"
    };
    size_t i;

    for (i = 0; i < sizeof(sources) / sizeof(sources[0]); i++) {
        ParseResult result = {0};

        CHECK(parse_input_error(sources[i], &result));
        CHECK(result.syntax_error_count >= 1);
        parse_result_destroy(&result);
    }
    return true;
}

static bool test_io_required_delimiters(void)
{
    ParseResult result = {0};

    CHECK(parse_input_error(
        "inteira principal() @[le(x)respost 0;]",
        &result
    ));
    CHECK(has_expected_diagnostic(&result, "TOK_PONTO_VIRGULA"));
    parse_result_destroy(&result);

    CHECK(parse_input_error(
        "inteira principal() @[imprime(x)respost 0;]",
        &result
    ));
    CHECK(has_expected_diagnostic(&result, "TOK_PONTO_VIRGULA"));
    parse_result_destroy(&result);

    CHECK(parse_input_error(
        "inteira principal() @[le x;respost 0;]",
        &result
    ));
    CHECK(has_expected_diagnostic(&result, "TOK_ABRE_PAREN"));
    parse_result_destroy(&result);

    CHECK(parse_input_error(
        "inteira principal() @[imprime x;respost 0;]",
        &result
    ));
    CHECK(has_expected_diagnostic(&result, "TOK_ABRE_PAREN"));
    parse_result_destroy(&result);
    return true;
}

static bool test_io_recovery_and_partial_destroy(void)
{
    ParseResult result = {0};
    AstIo *io;

    CHECK(parse_input_error(
        "inteira principal() @[le(1)le(x);imprime(x);respost 0;]",
        &result
    ));
    CHECK(result.syntax_error_count >= 1);
    parse_result_destroy(&result);

    CHECK(parse_input_error(
        "inteira principal() @[le();imprime();respost 0;]",
        &result
    ));
    CHECK(result.syntax_error_count >= 2);
    parse_result_destroy(&result);

    io = calloc(1, sizeof(*io));
    CHECK(io != NULL);
    io->kind = AST_IO_READ;
    ast_io_destroy(io);

    io = calloc(1, sizeof(*io));
    CHECK(io != NULL);
    io->kind = AST_IO_WRITE;
    ast_io_destroy(io);
    return true;
}

static bool test_lexical_error_recovery(void)
{
    const char source[] = "inteira x;#inteira y;";
    Parser parser = {0};
    ParseResult result;

    CHECK(parser_init(&parser, source, sizeof(source) - 1));
    result = parser_parse(&parser);
    CHECK(result.status == PARSE_INPUT_ERRORS);
    CHECK(result.lexical_error_count == 1);
    CHECK(result.syntax_error_count == 0);
    CHECK(result.diagnostic_count == 1);
    CHECK(result.diagnostics[0].kind == DIAG_LEXICAL);
    CHECK(result.diagnostics[0].found_type == TOK_INVALIDO);
    CHECK(result.diagnostics[0].found_lexeme_length == 1);
    CHECK(result.diagnostics[0].found_lexeme[0] == '#');
    parse_result_destroy(&result);
    parser_destroy(&parser);
    return true;
}

static bool test_eof_diagnostic(void)
{
    const char *source = "inteira principal() @ [inteira x;";
    ParseResult result = {0};
    Diagnostic *diagnostic;

    CHECK(parse_input_error(source, &result));
    CHECK(result.syntax_error_count >= 1);
    diagnostic = &result.diagnostics[result.diagnostic_count - 1];
    CHECK(diagnostic->kind == DIAG_SYNTAX);
    CHECK(diagnostic->found_type == TOK_EOF);
    CHECK(diagnostic->found_lexeme == NULL);
    CHECK(diagnostic->found_lexeme_length == 0);
    CHECK(diagnostic->location.line == 1);
    parse_result_destroy(&result);
    return true;
}

static bool test_syntax_error_categories(void)
{
    ParseResult result = {0};
    size_t i;
    bool found_syntax = false;

    CHECK(parse_input_error("func inteira f() @ []", &result));
    CHECK(result.syntax_error_count >= 1);
    for (i = 0; i < result.diagnostic_count; i++) {
        if (result.diagnostics[i].kind == DIAG_SYNTAX &&
            result.diagnostics[i].found_type == TOK_FUNC) {
            found_syntax = true;
        }
    }
    CHECK(found_syntax);
    parse_result_destroy(&result);

    CHECK(parse_input_error("vazio x;", &result));
    CHECK(result.syntax_error_count >= 1);
    parse_result_destroy(&result);

    CHECK(parse_input_error("f(1);", &result));
    CHECK(result.syntax_error_count >= 1);
    parse_result_destroy(&result);

    CHECK(parse_input_error("inteira principal() @ [a < b < c;]", &result));
    CHECK(result.syntax_error_count >= 1);
    parse_result_destroy(&result);
    return true;
}

static bool test_invalid_constructs_recover(void)
{
    static const char *sources[] = {
        "inteira principal( ) @ [respost ;]",
        "inteira principal() @ [cond (x > 0) []]",
        "inteira principal() @ [casocontrario edai []]",
        "inteira principal() @ [repete (x receba 0; x < 2, x receba 1) edai []]",
        "inteira principal() @ [raiz();]",
        "inteira principal() @ [raiz(a, b);]",
        "inteira principal() @ [f()(x);]",
        "inteira principal() @ [v{1}{2};]"
    };
    size_t i;

    for (i = 0; i < sizeof(sources) / sizeof(sources[0]); i++) {
        ParseResult result = {0};
        CHECK(parse_input_error(sources[i], &result));
        CHECK(result.syntax_error_count >= 1);
        parse_result_destroy(&result);
    }
    return true;
}

static bool test_multiple_errors_and_progress(void)
{
    const char *source =
        ";;;;;"
        "inteira principal() @ ["
        "cond cond cond"
        "respost respost respost"
        "[ [ ["
        "]";
    ParseResult result = {0};

    CHECK(parse_input_error(source, &result));
    CHECK(result.diagnostic_count >= 3);
    parse_result_destroy(&result);
    return true;
}

static bool test_parser_parse_once(void)
{
    Parser parser = {0};
    ParseResult first;
    ParseResult second;

    CHECK(parser_init_from_string(&parser, "inteira x;"));
    first = parser_parse(&parser);
    second = parser_parse(&parser);
    CHECK(first.status == PARSE_OK);
    CHECK(second.status == PARSE_INTERNAL_FAILURE);
    CHECK(second.ast == NULL);
    CHECK(second.diagnostics == NULL);
    CHECK(second.diagnostic_count == 0);
    parse_result_destroy(&first);
    parse_result_destroy(&second);
    parser_destroy(&parser);
    return true;
}

static bool test_parser_source_copy(void)
{
    char source[] = "inteira x;";
    Parser parser = {0};
    ParseResult result;

    CHECK(parser_init(&parser, source, sizeof(source) - 1));
    source[8] = 'y';
    result = parser_parse(&parser);
    CHECK(result.status == PARSE_OK);
    CHECK(result.ast->items[0]->kind == AST_TOP_DECLARATION);
    CHECK(strcmp(
        result.ast->items[0]->as.declaration->declarators[0]->name,
        "x"
    ) == 0);
    parse_result_destroy(&result);
    parser_destroy(&parser);
    return true;
}

static bool test_void_and_invalid_value_contexts(void)
{
    ParseResult result = {0};

    CHECK(parse_ok(
        "outrafuncao vazio f() @[respost 1;]"
        "vazio principal() @[]",
        &result
    ));
    CHECK(result.ast->count == 2);
    CHECK(result.ast->items[0]->kind == AST_TOP_FUNCTION);
    CHECK(result.ast->items[0]->as.function->return_type.kind ==
          AST_TYPE_VAZIO);
    parse_result_destroy(&result);

    CHECK(parse_input_error("registro R @[vazio nome;]", &result));
    CHECK(result.syntax_error_count >= 1);
    parse_result_destroy(&result);

    CHECK(parse_input_error(
        "outrafuncao inteira f(vazio x) @[]",
        &result
    ));
    CHECK(result.syntax_error_count >= 1);
    parse_result_destroy(&result);

    CHECK(parse_input_error(
        "inteira principal() @[vazio x;]",
        &result
    ));
    CHECK(result.syntax_error_count >= 1);
    parse_result_destroy(&result);
    return true;
}

static bool test_explicit_nul_and_partial_cleanup(void)
{
    const char source[] = {
        'i', 'n', 't', 'e', 'i', 'r', 'a', ' ', 'x', ';',
        '\0',
        'i', 'n', 't', 'e', 'i', 'r', 'a', ' ', 'y', ';'
    };
    Parser parser = {0};
    ParseResult result;

    CHECK(parser_init(&parser, source, sizeof(source)));
    result = parser_parse(&parser);
    CHECK(result.status == PARSE_INPUT_ERRORS);
    CHECK(result.lexical_error_count == 1);
    CHECK(result.syntax_error_count == 0);
    CHECK(result.diagnostic_count == 1);
    CHECK(result.diagnostics[0].found_lexeme_length == 1);
    CHECK(memcmp(result.diagnostics[0].found_lexeme, "\0", 1) == 0);
    parse_result_destroy(&result);
    parser_destroy(&parser);

    CHECK(parse_input_error("outrafuncao inteira f() @", &result));
    parse_result_destroy(&result);
    return true;
}

static bool test_official_program_and_growth(void)
{
    const char *source =
        "registro Pessoa @["
        "inteira a; flut b; palavra c; bool d; duplocarpado e;"
        "]"
        "outrafuncao inteira f("
        "inteira a, flut b, palavra c, bool d, duplocarpado e"
        ") @[respost a;]"
        "inteira principal() @["
        "inteira a;"
        "a receba f(1, 2, 3, 4, 5);"
        "cond (a > 0) edai []"
        "casocontrario cond (a == 0) edai []"
        "casocontrario cond (a < 0) edai []"
        "casocontrario cond (a ! 0) edai []"
        "casocontrario cond (a > 1) edai []"
        "casocontrario edai []"
        "durante (a < 10) edai []"
        "repete (a receba 0, a < 10, a receba a + 1) edai []"
        "respost raiz(a);"
        "]";
    ParseResult result = {0};
    AstFunctionDecl *function;
    AstBlock *body;

    CHECK(parse_ok(source, &result));
    CHECK(result.ast->count == 3);
    CHECK(result.ast->items[0]->kind == AST_TOP_RECORD);
    CHECK(result.ast->items[0]->as.record->field_count == 5);
    CHECK(result.ast->items[1]->kind == AST_TOP_FUNCTION);
    function = result.ast->items[1]->as.function;
    CHECK(function->parameter_count == 5);
    CHECK(result.ast->items[2]->kind == AST_TOP_FUNCTION);
    body = result.ast->items[2]->as.function->body;
    CHECK(body->statement_count == 6);
    CHECK(body->statements[1]->kind == AST_STMT_ASSIGNMENT);
    CHECK(body->statements[1]->as.assignment->value->kind ==
          AST_EXPR_REFERENCE);
    CHECK(body->statements[1]->as.assignment->value->as.reference.kind ==
          AST_REFERENCE_CALL);
    CHECK(body->statements[1]->as.assignment->value->as.reference.
          as.call.argument_count == 5);
    CHECK(body->statements[2]->kind == AST_STMT_IF);
    CHECK(body->statements[2]->as.if_statement->else_if_count == 4);
    CHECK(body->statements[2]->as.if_statement->else_block != NULL);
    parse_result_destroy(&result);
    return true;
}

static bool test_parser_smoke_fuzz(void)
{
    unsigned int state = 0x13579BDFu;
    size_t iteration;

    for (iteration = 0; iteration < 4096; iteration++) {
        unsigned char source[64];
        size_t length;
        size_t i;
        Parser parser = {0};
        ParseResult result;

        state = state * 1664525u + 1013904223u;
        length = (size_t)(state % sizeof(source));
        for (i = 0; i < length; i++) {
            state = state * 1664525u + 1013904223u;
            source[i] = (unsigned char)(state >> 24);
        }

        CHECK(parser_init(&parser, (const char *)source, length));
        result = parser_parse(&parser);
        CHECK(
            result.status == PARSE_OK ||
            result.status == PARSE_INPUT_ERRORS ||
            result.status == PARSE_INTERNAL_FAILURE
        );
        parse_result_destroy(&result);
        parser_destroy(&parser);
    }
    return true;
}

static bool test_recovery_stress(void)
{
    static const char *sources[] = {
        ";;;;;",
        "]]]]]",
        ",,,,,",
        "cond cond cond",
        "func func func",
        "respost respost respost",
        "[[[",
        "inteira inteira",
        "x x x"
    };
    size_t i;

    for (i = 0; i < sizeof(sources) / sizeof(sources[0]); i++) {
        ParseResult result = {0};

        CHECK(parse_input_error(sources[i], &result));
        CHECK(result.diagnostic_count > 0);
        parse_result_destroy(&result);
    }
    return true;
}

static bool test_invalid_argument_lifecycle(void)
{
    Parser parser = {0};
    ParseResult result = {0};

    CHECK(!parser_init(NULL, "", 0));
    CHECK(!parser_init(&parser, NULL, 0));
    CHECK(parser_init(&parser, "", 0));
    CHECK(!parser_init(&parser, "inteira x;", 10));
    parser_destroy(&parser);

    CHECK(parser_init_from_string(&parser, "inteira x;"));
    result = parser_parse(&parser);
    CHECK(result.status == PARSE_OK);
    parse_result_destroy(&result);
    parser_destroy(&parser);
    return true;
}

/* ------------------------------------------------------------------ */
/* Testes das correcoes pos-PjBL1 (D33, D35, D36, D37)                 */
/* ------------------------------------------------------------------ */

/* Parseia e devolve o resultado completo (sem exigir status). */
static ParseResult parse_any(const char *source, size_t length)
{
    Parser parser = {0};
    ParseResult result = {0};

    if (!parser_init(&parser, source, length)) {
        result.status = PARSE_INTERNAL_FAILURE;
        return result;
    }
    result = parser_parse(&parser);
    parser_destroy(&parser);
    return result;
}

/* Monta prefix + repeat*count + middle + suffix_repeat*count + suffix. */
static char *build_repeated(
    const char *prefix,
    const char *repeat,
    const char *middle,
    const char *suffix_repeat,
    const char *suffix,
    size_t count,
    size_t *out_length
)
{
    size_t length = strlen(prefix) + strlen(middle) + strlen(suffix) +
                    count * (strlen(repeat) + strlen(suffix_repeat));
    char *buffer = malloc(length + 1);
    char *cursor = buffer;
    size_t i;

    if (buffer == NULL) {
        return NULL;
    }
    cursor += sprintf(cursor, "%s", prefix);
    for (i = 0; i < count; i++) {
        cursor += sprintf(cursor, "%s", repeat);
    }
    cursor += sprintf(cursor, "%s", middle);
    for (i = 0; i < count; i++) {
        cursor += sprintf(cursor, "%s", suffix_repeat);
    }
    sprintf(cursor, "%s", suffix);
    *out_length = length;
    return buffer;
}

static bool test_call_statement(void)
{
    ParseResult result = {0};
    AstFunctionDecl *main_function;
    AstStatement *statement;

    CHECK(parse_ok(
        "outrafuncao vazio mostra(inteira x) @ [imprime(x);]"
        "inteira principal() @ [mostra(1); mostra(2 + 3); respost 0;]",
        &result
    ));
    main_function = result.ast->items[1]->as.function;
    CHECK(main_function->body->statement_count == 3);
    statement = main_function->body->statements[0];
    CHECK(statement->kind == AST_STMT_CALL);
    CHECK(statement->as.call->kind == AST_EXPR_REFERENCE);
    CHECK(statement->as.call->as.reference.kind == AST_REFERENCE_CALL);
    CHECK(strcmp(statement->as.call->as.reference.name, "mostra") == 0);
    CHECK(statement->as.call->as.reference.as.call.argument_count == 1);
    CHECK(main_function->body->statements[1]->kind == AST_STMT_CALL);
    parse_result_destroy(&result);

    /* Chamada sem ';' na mesma linha continua sendo erro. */
    CHECK(parse_input_error(
        "inteira principal() @ [f(1) f(2); respost 0;]",
        &result
    ));
    CHECK(result.syntax_error_count >= 1);
    parse_result_destroy(&result);

    /* Identificador isolado nao e comando. */
    CHECK(parse_input_error("inteira principal() @ [f; respost 0;]", &result));
    CHECK(has_expected_diagnostic(&result, "TOK_RECEBA"));
    parse_result_destroy(&result);
    return true;
}

static bool test_missing_semicolon_recovery(void)
{
    const char *source =
        "inteira principal() @ [\n"
        "    inteira a\n"
        "    inteira b;\n"
        "    b receba 1\n"
        "    a receba b;\n"
        "    respost a\n"
        "]\n";
    ParseResult result = {0};

    CHECK(parse_input_error(source, &result));
    /* Exatamente um erro por ';' ausente, sem cascata na linha seguinte. */
    CHECK(result.syntax_error_count == 3);
    CHECK(result.diagnostics[0].location.line == 2);
    CHECK(result.diagnostics[1].location.line == 4);
    CHECK(result.diagnostics[2].location.line == 6);
    CHECK(result.diagnostics[0].expected != NULL &&
          strcmp(result.diagnostics[0].expected, "TOK_PONTO_VIRGULA") == 0);
    parse_result_destroy(&result);
    return true;
}

static bool test_lexical_error_without_cascade(void)
{
    const char *source =
        "inteira principal() @ [\n"
        "    inteira a;\n"
        "    a receba 1 $ 2;\n"
        "    a receba ;\n"
        "    respost a;\n"
        "]\n";
    ParseResult result = parse_any(source, strlen(source));

    CHECK(result.status == PARSE_INPUT_ERRORS);
    CHECK(result.lexical_error_count == 1);
    /* O erro da linha 3 e consequencia do '$'; o da linha 4 e genuino. */
    CHECK(result.syntax_error_count == 1);
    CHECK(result.suppressed_syntax_error_count >= 1);
    CHECK(result.diagnostics[1].kind == DIAG_SYNTAX);
    CHECK(result.diagnostics[1].location.line == 4);
    parse_result_destroy(&result);
    return true;
}

static bool test_accented_identifier_single_error(void)
{
    const char source[] =
        "inteira principal() @ [inteira fun\xC3\xA7\xC3\xA3o; respost 0;]";
    ParseResult result = parse_any(source, sizeof(source) - 1);

    CHECK(result.status == PARSE_INPUT_ERRORS);
    CHECK(result.lexical_error_count == 1);
    CHECK(result.syntax_error_count == 0);
    CHECK(result.diagnostic_count == 1);
    CHECK(result.diagnostics[0].found_type == TOK_ID);
    CHECK(result.diagnostics[0].found_lexeme_length == 8);
    parse_result_destroy(&result);
    return true;
}

/*
 * Correcao 2.2 do PjBL1: entradas patologicas (100.000 operadores
 * encadeados, 50.000 parenteses, 100.000 blocos...) nao podem estourar a
 * pilha. Cada uma deve gerar exatamente um erro sintatico controlado.
 */
static bool test_extreme_nesting_is_one_error(void)
{
    static const struct {
        const char *prefix;
        const char *repeat;
        const char *middle;
        const char *suffix_repeat;
        const char *suffix;
        size_t count;
    } cases[] = {
        {"inteira principal() @ [inteira a; a receba 1", " + a", "", "",
         "; respost a;]", 100000},
        {"inteira principal() @ [inteira a; a receba ", "(", "a", ")",
         "; respost a;]", 50000},
        {"inteira principal() @ [", "[ ", "", " ]", " respost 0;]", 100000},
        {"inteira principal() @ [inteira a; a receba ", "- ", "a", "",
         "; respost a;]", 100000},
        {"bool principal() @ [bool a; a receba ", "N\xC3\x83O ", "a", "",
         "; respost a;]", 100000},
        {"inteira principal() @ [inteira a; a receba a", " ^ a", "", "",
         "; respost a;]", 100000},
        {"inteira principal() @ [inteira a; a receba ", "f(", "a", ")",
         "; respost a;]", 50000},
        {"inteira principal() @ [", "cond (VER) edai [", "", "]",
         " respost 0;]", 50000},
        {"bool principal() @ [bool a; a receba VER", " E a", "", "",
         "; respost a;]", 100000}
    };
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        size_t length = 0;
        char *source = build_repeated(
            cases[i].prefix,
            cases[i].repeat,
            cases[i].middle,
            cases[i].suffix_repeat,
            cases[i].suffix,
            cases[i].count,
            &length
        );
        ParseResult result;

        CHECK(source != NULL);
        result = parse_any(source, length);
        free(source);
        CHECK(result.status == PARSE_INPUT_ERRORS);
        CHECK(result.lexical_error_count == 0);
        CHECK(result.syntax_error_count == 1);
        parse_result_destroy(&result);
    }
    return true;
}

static bool test_void_variable_single_error(void)
{
    ParseResult result = {0};

    CHECK(parse_input_error(
        "inteira principal() @ [vazio x; x receba 1; respost 0;]",
        &result
    ));
    CHECK(result.syntax_error_count == 1);
    parse_result_destroy(&result);
    return true;
}

/* D39: variaveis de tipo registro e acesso a campo. */
static bool test_record_variables_and_fields(void)
{
    ParseResult result = {0};
    AstDeclaration *global;
    AstFunctionDecl *main_function;
    AstAssignment *assignment;

    CHECK(parse_ok(
        "registro Ponto @ [inteira x;]"
        "registro Ponto origem, vet{4} pts;"
        "inteira principal() @ ["
        "  registro Ponto p;"
        "  p.x receba pts{1}.x + origem.x;"
        "  respost p.x;"
        "]",
        &result
    ));
    CHECK(result.ast->count == 3);
    CHECK(result.ast->items[0]->kind == AST_TOP_RECORD);
    CHECK(result.ast->items[1]->kind == AST_TOP_DECLARATION);
    global = result.ast->items[1]->as.declaration;
    CHECK(global->type.kind == AST_TYPE_REGISTRO);
    CHECK(strcmp(global->record_name, "Ponto") == 0);
    CHECK(global->declarator_count == 2);
    CHECK(global->declarators[1]->kind == AST_DECLARATOR_VECTOR);
    main_function = result.ast->items[2]->as.function;
    CHECK(main_function->body->statements[0]->kind == AST_STMT_DECLARATION);
    assignment = main_function->body->statements[1]->as.assignment;
    CHECK(strcmp(assignment->target->field_name, "x") == 0);
    CHECK(assignment->value->as.binary.left->as.reference.kind ==
          AST_REFERENCE_VECTOR);
    CHECK(strcmp(assignment->value->as.binary.left->as.reference.field_name,
                 "x") == 0);
    parse_result_destroy(&result);

    /* Registro so pode ser definido no nivel superior. */
    CHECK(parse_input_error(
        "inteira principal() @ [registro R @ [inteira x;] respost 0;]",
        &result
    ));
    parse_result_destroy(&result);
    /* '.' sem nome de campo. */
    CHECK(parse_input_error(
        "inteira principal() @ [p. receba 1; respost 0;]",
        &result
    ));
    parse_result_destroy(&result);
    return true;
}

/* D46: "tipo vet{N} a, b;" declara dois vetores de N. */
static bool test_vector_size_shared_by_declarators(void)
{
    ParseResult result = {0};
    AstDeclaration *declaration;

    CHECK(parse_ok("inteira vet{8} a, b, vet{3} c, d; inteira x, vet{2} y;",
                   &result));
    declaration = result.ast->items[0]->as.declaration;
    CHECK(declaration->declarator_count == 4);
    CHECK(declaration->declarators[1]->kind == AST_DECLARATOR_VECTOR);
    CHECK(strcmp(declaration->declarators[1]->vector_size_lexeme, "8") == 0);
    CHECK(strcmp(declaration->declarators[3]->vector_size_lexeme, "3") == 0);
    declaration = result.ast->items[1]->as.declaration;
    CHECK(declaration->declarators[0]->kind == AST_DECLARATOR_SIMPLE);
    parse_result_destroy(&result);
    return true;
}

/* Logo abaixo dos limites, o programa continua valido. */
static bool test_nesting_just_below_limits(void)
{
    size_t length = 0;
    char *source;
    ParseResult result;

    /* 200 parenteses (cada um conta 1 nivel, mais o bloco e a atribuicao). */
    source = build_repeated(
        "inteira principal() @ [inteira a; a receba ", "(", "a", ")",
        "; respost a;]", 200, &length
    );
    CHECK(source != NULL);
    result = parse_any(source, length);
    free(source);
    CHECK(result.status == PARSE_OK);
    parse_result_destroy(&result);

    /* Cadeia de 1000 somas: altura 1001 <= 1024. */
    source = build_repeated(
        "inteira principal() @ [inteira a; a receba 1", " + a", "", "",
        "; respost a;]", 1000, &length
    );
    CHECK(source != NULL);
    result = parse_any(source, length);
    free(source);
    CHECK(result.status == PARSE_OK);
    CHECK(result.ast->items[0]->as.function->body->statements[1]
              ->as.assignment->value->height == 1001);
    parse_result_destroy(&result);
    return true;
}

static bool run_test(const char *name, bool (*test)(void))
{
    size_t failures_before = tests_failed;
    bool result = test();

    if (result && tests_failed == failures_before) {
        printf("PASS %s\n", name);
        return true;
    }
    printf("FAIL %s\n", name);
    return false;
}

int main(void)
{
    run_test("empty_program_and_lifecycle", test_empty_program_and_lifecycle);
    run_test(
        "global_declarations_and_vectors",
        test_global_declarations_and_vectors
    );
    run_test("records_and_functions", test_records_and_functions);
    run_test(
        "statements_and_control_flow",
        test_statements_and_control_flow
    );
    run_test("expression_precedence", test_expression_precedence);
    run_test(
        "logical_calls_vectors_and_root",
        test_logical_calls_vectors_and_root
    );
    run_test("io_statements_and_ast", test_io_statements_and_ast);
    run_test("io_syntax_errors", test_io_syntax_errors);
    run_test("io_required_delimiters", test_io_required_delimiters);
    run_test(
        "io_recovery_and_partial_destroy",
        test_io_recovery_and_partial_destroy
    );
    run_test("lexical_error_recovery", test_lexical_error_recovery);
    run_test("eof_diagnostic", test_eof_diagnostic);
    run_test("syntax_error_categories", test_syntax_error_categories);
    run_test("invalid_constructs_recover", test_invalid_constructs_recover);
    run_test(
        "multiple_errors_and_progress",
        test_multiple_errors_and_progress
    );
    run_test("parser_parse_once", test_parser_parse_once);
    run_test("parser_source_copy", test_parser_source_copy);
    run_test(
        "void_and_invalid_value_contexts",
        test_void_and_invalid_value_contexts
    );
    run_test(
        "explicit_nul_and_partial_cleanup",
        test_explicit_nul_and_partial_cleanup
    );
    run_test(
        "official_program_and_growth",
        test_official_program_and_growth
    );
    run_test("parser_smoke_fuzz", test_parser_smoke_fuzz);
    run_test("recovery_stress", test_recovery_stress);
    run_test("invalid_argument_lifecycle", test_invalid_argument_lifecycle);
    run_test("call_statement", test_call_statement);
    run_test("missing_semicolon_recovery", test_missing_semicolon_recovery);
    run_test(
        "lexical_error_without_cascade",
        test_lexical_error_without_cascade
    );
    run_test(
        "accented_identifier_single_error",
        test_accented_identifier_single_error
    );
    run_test(
        "extreme_nesting_is_one_error",
        test_extreme_nesting_is_one_error
    );
    run_test("nesting_just_below_limits", test_nesting_just_below_limits);
    run_test("void_variable_single_error", test_void_variable_single_error);
    run_test(
        "record_variables_and_fields",
        test_record_variables_and_fields
    );
    run_test(
        "vector_size_shared_by_declarators",
        test_vector_size_shared_by_declarators
    );

    printf("%zu assertions, %zu failures\n", tests_run, tests_failed);
    return tests_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
