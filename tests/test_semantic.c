#include "semantic.h"

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

static bool parse_source(const char *source, ParseResult *parse_result)
{
    Parser parser = {0};

    if (source == NULL || parse_result == NULL ||
        !parser_init_from_string(&parser, source)) {
        return false;
    }
    *parse_result = parser_parse(&parser);
    parser_destroy(&parser);
    return true;
}

static bool analyze_source(
    const char *source,
    ParseResult *parse_result,
    SemanticAnalyzer *analyzer,
    SemanticResult *semantic_result
)
{
    if (parse_result == NULL || analyzer == NULL || semantic_result == NULL) {
        return false;
    }
    *parse_result = (ParseResult){0};
    *analyzer = (SemanticAnalyzer){0};
    *semantic_result = (SemanticResult){0};

    if (!parse_source(source, parse_result) ||
        parse_result->status != PARSE_OK ||
        parse_result->ast == NULL) {
        return false;
    }
    if (!semantic_init(analyzer, parse_result->ast)) {
        return false;
    }
    *semantic_result = semantic_analyze(analyzer);
    return true;
}

static void destroy_analysis(
    ParseResult *parse_result,
    SemanticAnalyzer *analyzer,
    SemanticResult *semantic_result
)
{
    semantic_result_destroy(semantic_result);
    semantic_destroy(analyzer);
    parse_result_destroy(parse_result);
}

static bool expect_internal_failure(const AstProgram *program)
{
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};
    bool valid;

    if (!semantic_init(&analyzer, program)) {
        return false;
    }
    result = semantic_analyze(&analyzer);
    valid = result.status == SEMANTIC_INTERNAL_FAILURE &&
            result.diagnostic_count == 0;
    semantic_result_destroy(&result);
    semantic_destroy(&analyzer);
    return valid;
}

static bool has_code(
    const SemanticResult *result,
    SemanticDiagnosticCode code
)
{
    size_t i;

    if (result == NULL) {
        return false;
    }
    for (i = 0; i < result->diagnostic_count; i++) {
        if (result->diagnostics[i].code == code) {
            return true;
        }
    }
    return false;
}

static const SemanticDiagnostic *find_code(
    const SemanticResult *result,
    SemanticDiagnosticCode code
)
{
    size_t i;

    if (result == NULL) {
        return NULL;
    }
    for (i = 0; i < result->diagnostic_count; i++) {
        if (result->diagnostics[i].code == code) {
            return &result->diagnostics[i];
        }
    }
    return NULL;
}

static bool test_lifecycle_and_ownership(void)
{
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult first = {0};
    SemanticResult second = {0};
    SemanticResult third = {0};
    const SemanticDiagnostic *diagnostic;

    CHECK(!semantic_init(NULL, NULL));
    CHECK(!semantic_init(&analyzer, NULL));
    CHECK(parse_source(
        "inteira principal() @[inteira x; x receba y;]",
        &parse_result
    ));
    CHECK(parse_result.status == PARSE_OK);
    CHECK(semantic_init(&analyzer, parse_result.ast));
    CHECK(!semantic_init(&analyzer, parse_result.ast));

    first = semantic_analyze(&analyzer);
    CHECK(first.status == SEMANTIC_ERRORS);
    CHECK(first.diagnostics != NULL);
    CHECK(first.diagnostic_count > 0);
    diagnostic = find_code(&first, SEM_UNDECLARED_IDENTIFIER);
    CHECK(diagnostic != NULL);
    CHECK(diagnostic->symbol_name != NULL);
    CHECK(diagnostic->symbol_name_length == 1);
    CHECK(strcmp(diagnostic->symbol_name, "y") == 0);

    second = semantic_analyze(&analyzer);
    CHECK(second.status == SEMANTIC_INTERNAL_FAILURE);
    CHECK(second.diagnostics == NULL);
    CHECK(second.diagnostic_count == 0);

    semantic_destroy(&analyzer);
    CHECK(first.diagnostics != NULL);
    CHECK(first.diagnostics[0].message != NULL);

    CHECK(semantic_init(&analyzer, parse_result.ast));
    third = semantic_analyze(&analyzer);
    CHECK(third.status == SEMANTIC_ERRORS);
    semantic_result_destroy(&third);
    semantic_destroy(&analyzer);

    semantic_result_destroy(&first);
    semantic_result_destroy(&first);
    semantic_result_destroy(&second);
    parse_result_destroy(&parse_result);

    semantic_destroy(&analyzer);
    parse_result_destroy(&parse_result);
    return true;
}

static bool test_principal_and_namespace(void)
{
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};

    CHECK(analyze_source(
        "inteira principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    CHECK(result.diagnostic_count == 0);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "vazio principal(inteira argc) @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira x;",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(has_code(&result, SEM_INVALID_PRINCIPAL));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @ [] vazio principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(has_code(&result, SEM_INVALID_PRINCIPAL));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira f;"
        "outrafuncao inteira f() @[respost 1;]"
        "inteira principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(has_code(&result, SEM_DUPLICATE_DECLARATION));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "registro R @ [] inteira R;"
        "inteira principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(has_code(&result, SEM_DUPLICATE_DECLARATION));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira vet{2} v; outrafuncao inteira v() @[respost 1;]"
        "inteira principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_DUPLICATE_DECLARATION));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira vet{2} v; registro v @ [] inteira principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_DUPLICATE_DECLARATION));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira v() @[respost 1;] registro v @ []"
        "inteira principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_DUPLICATE_DECLARATION));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira v; inteira vet{2} v; inteira principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_DUPLICATE_DECLARATION));
    destroy_analysis(&parse_result, &analyzer, &result);
    return true;
}

static bool test_declarations_and_scopes(void)
{
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};

    CHECK(analyze_source(
        "inteira principal() @["
        "inteira x; x receba 1;"
        "[inteira x; x receba 2;]"
        "x receba 3;"
        "]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @["
        "inteira x; inteira x;"
        "]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(has_code(&result, SEM_DUPLICATE_DECLARATION));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @[x receba 1; inteira x;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(has_code(&result, SEM_UNDECLARED_IDENTIFIER));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @["
        "cond (VER) edai [inteira x; x receba 1;]"
        "x receba 2;"
        "]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(has_code(&result, SEM_UNDECLARED_IDENTIFIER));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @[inteira x;"
        "cond (VER) edai [inteira y;]"
        "casocontrario edai [inteira y;]"
        "]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    return true;
}

static bool test_globals_forward_calls_and_recursion(void)
{
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};

    CHECK(analyze_source(
        "inteira principal() @[inteira y; y receba x;] inteira x;",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira a() @[respost b();]"
        "outrafuncao inteira b() @[respost 1;]"
        "inteira principal() @[inteira x; x receba a();]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira f(inteira n) @[respost f(n);]"
        "inteira principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira a() @[respost b();]"
        "outrafuncao inteira b() @[respost a();]"
        "inteira principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);
    return true;
}

static bool test_shadowing_and_name_kinds(void)
{
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};

    CHECK(analyze_source(
        "outrafuncao inteira f() @[respost 1;]"
        "inteira principal() @[inteira f; inteira x; x receba f();]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(has_code(&result, SEM_NOT_A_FUNCTION));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira f() @[respost 1;]"
        "inteira principal() @[inteira x; x receba f;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(has_code(&result, SEM_NAME_KIND_MISMATCH));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @[inteira f; inteira x; x receba f();]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(has_code(&result, SEM_NOT_A_FUNCTION));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira f(inteira f) @[respost f;]"
        "inteira principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira f(inteira x, flut x) @ [inteira x; respost 0;]"
        "inteira principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 2);
    CHECK(has_code(&result, SEM_DUPLICATE_DECLARATION));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira f() @[respost 1;]"
        "inteira principal() @["
        "[inteira f; inteira x; x receba f();]"
        "inteira y; y receba f();"
        "]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_NOT_A_FUNCTION));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira vet{2} v; inteira principal() @["
        "inteira v; inteira x; x receba v{0};"
        "]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_NOT_A_VECTOR));
    destroy_analysis(&parse_result, &analyzer, &result);
    return true;
}

static bool test_assignability_and_returns(void)
{
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};

    CHECK(analyze_source(
        "inteira i; flut f; duplocarpado d; bool b; palavra s;"
        "inteira principal() @["
        "f receba i; d receba i; d receba f;"
        "b receba VER; s receba \"abc\";"
        "]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira i; flut f; duplocarpado d;"
        "inteira principal() @[i receba f; f receba d;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 2);
    CHECK(has_code(&result, SEM_INVALID_ASSIGNMENT));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao flut f() @[respost 1;]"
        "outrafuncao inteira g() @[respost 1.5;]"
        "outrafuncao vazio h() @[respost 1;]"
        "outrafuncao inteira semretorno() @[inteira x;]"
        "inteira principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(has_code(&result, SEM_INVALID_RETURN));
    /* D34: semretorno agora e rejeitada por nao ter respost. */
    CHECK(has_code(&result, SEM_MISSING_RETURN));
    CHECK(strcmp(
        find_code(&result, SEM_MISSING_RETURN)->symbol_name,
        "semretorno"
    ) == 0);
    CHECK(result.diagnostic_count == 3);
    destroy_analysis(&parse_result, &analyzer, &result);
    return true;
}

static bool test_operators_and_conditions(void)
{
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};

    CHECK(analyze_source(
        "inteira i; flut f; duplocarpado d; bool b; palavra s;"
        "inteira principal() @["
        "f receba i + f; d receba f * d;"
        "f receba i / i; i receba i // i;"
        "d receba i ^ f; f receba +i; f receba -i;"
        "b receba i < f; b receba i > i;"
        "b receba i == i; b receba s == s;"
        "b receba VER == FAL; b receba i ! i;"
        "b receba VER E FAL; b receba VER XOR FAL;"
        "b receba VER OU FAL;"
        "b receba NÃO VER; d receba raiz(d); f receba raiz(i);"
        "]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira i; flut f; palavra s; bool b;"
        "inteira principal() @["
        "f receba f // i; b receba i == f;"
        "b receba s < s; b receba NÃO i;"
        "f receba +s;"
        "]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 5);
    CHECK(has_code(&result, SEM_INVALID_OPERATOR));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @["
        "cond (1) edai []"
        "durante (\"abc\") edai []"
        "]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 2);
    CHECK(has_code(&result, SEM_INVALID_CONDITION));
    destroy_analysis(&parse_result, &analyzer, &result);
    return true;
}

static bool test_calls_and_void_values(void)
{
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};

    CHECK(analyze_source(
        "outrafuncao flut f(inteira a, duplocarpado b) @[respost 1;]"
        "inteira principal() @["
        "flut x; x receba f(1, 1.5);"
        "]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira f(inteira a, flut b) @[respost a;]"
        "inteira principal() @["
        "inteira x; x receba f(1);"
        "x receba f(\"abc\", \"abc\", 1);"
        "]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(has_code(&result, SEM_ARGUMENT_COUNT_MISMATCH));
    CHECK(has_code(&result, SEM_ARGUMENT_TYPE_MISMATCH));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira f(inteira a) @[respost a;]"
        "inteira principal() @[inteira x; x receba f(1, inexistente);]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 2);
    CHECK(has_code(&result, SEM_ARGUMENT_COUNT_MISMATCH));
    CHECK(has_code(&result, SEM_UNDECLARED_IDENTIFIER));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @[inteira x; x receba inexistente(1);]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(has_code(&result, SEM_UNKNOWN_FUNCTION));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao vazio f() @ []"
        "inteira principal() @[inteira x; x receba f();]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_VOID_VALUE));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao vazio f() @ []"
        "outrafuncao vazio g() @ []"
        "inteira principal() @[inteira x; x receba f() == g();]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 2);
    CHECK(has_code(&result, SEM_VOID_VALUE));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao vazio f() @ []"
        "outrafuncao inteira takes(inteira a) @[respost a;]"
        "inteira principal() @["
        "inteira x; inteira vet{2} v;"
        "x receba +f();"
        "x receba raiz(f());"
        "x receba v{f()};"
        "durante (f()) edai []"
        "x receba takes(f());"
        "respost f();"
        "]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 6);
    CHECK(has_code(&result, SEM_VOID_VALUE));
    CHECK(!has_code(&result, SEM_INVALID_OPERATOR));
    CHECK(!has_code(&result, SEM_INVALID_CONDITION));
    CHECK(!has_code(&result, SEM_ARGUMENT_TYPE_MISMATCH));
    CHECK(!has_code(&result, SEM_INVALID_RETURN));
    destroy_analysis(&parse_result, &analyzer, &result);
    return true;
}

static bool test_io_semantics(void)
{
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};

    CHECK(analyze_source(
        "inteira principal() @["
        "bool b; inteira i; flut f; palavra p; duplocarpado d;"
        "le(b); le(i); le(f); le(p); le(d);"
        "imprime(b); imprime(i); imprime(f); imprime(p); imprime(d);"
        "respost 0;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    CHECK(result.diagnostic_count == 0);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira f(inteira x) @[le(x);imprime(x);respost x;]"
        "inteira principal() @[inteira y;y receba f(1);]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @["
        "inteira vet{5} valores; inteira i; i receba 0;"
        "le(valores{i}); imprime(valores{i}); respost 0;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira soma(inteira a, inteira b) @[respost a + b;]"
        "inteira principal() @[imprime(1); imprime(1.5);"
        "imprime(\"texto\"); imprime(VER); imprime(soma(1, 2));"
        "respost 0;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @[le(g); imprime(g); respost 0;]"
        "inteira g;",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @[le(x); respost 0;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_UNDECLARED_IDENTIFIER));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @[imprime(x); respost 0;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_UNDECLARED_IDENTIFIER));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @[le(x); imprime(y);"
        "inteira x; inteira y; respost 0;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 2);
    CHECK(has_code(&result, SEM_UNDECLARED_IDENTIFIER));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira vet{5} valores; flut indice;"
        "inteira principal() @[le(valores{indice});"
        "imprime(valores{indice}); respost 0;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 2);
    CHECK(has_code(&result, SEM_INVALID_VECTOR_INDEX));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @[inteira x; le(x{0}); respost 0;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_NOT_A_VECTOR));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira f() @[respost 1;]"
        "inteira principal() @[le(f); respost 0;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_NAME_KIND_MISMATCH));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao vazio f() @[]"
        "inteira principal() @[imprime(f()); respost 0;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_VOID_VALUE));
    CHECK(!has_code(&result, SEM_INVALID_OPERATOR));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @[imprime(inexistente()); respost 0;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_UNKNOWN_FUNCTION));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira f(inteira x) @[respost x;]"
        "inteira principal() @[imprime(f()); respost 0;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_ARGUMENT_COUNT_MISMATCH));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @[inteira x;"
        "[flut x; le(x); imprime(x);]"
        "le(x); imprime(x); respost 0;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    return true;
}

static bool test_io_impossible_ast(void)
{
    ParseResult parse_result = {0};
    AstProgram *program;
    AstFunctionDecl *function;
    AstStatement *read_statement;
    AstStatement *write_statement;
    AstIo *read_io;
    AstIo *write_io;
    bool valid;

    CHECK(parse_source(
        "inteira principal() @[inteira x; le(x); imprime(x); respost 0;]",
        &parse_result
    ));
    CHECK(parse_result.status == PARSE_OK);
    program = parse_result.ast;
    CHECK(program != NULL);
    CHECK(program->count == 1);
    CHECK(program->items[0] != NULL);
    CHECK(program->items[0]->kind == AST_TOP_FUNCTION);
    function = program->items[0]->as.function;
    CHECK(function != NULL);
    CHECK(function->body != NULL);
    CHECK(function->body->statement_count == 4);

    read_statement = function->body->statements[1];
    write_statement = function->body->statements[2];
    CHECK(read_statement != NULL);
    CHECK(read_statement->kind == AST_STMT_IO);
    CHECK(write_statement != NULL);
    CHECK(write_statement->kind == AST_STMT_IO);
    read_io = read_statement->as.io;
    write_io = write_statement->as.io;
    CHECK(read_io != NULL);
    CHECK(read_io->kind == AST_IO_READ);
    CHECK(read_io->as.target != NULL);
    CHECK(write_io != NULL);
    CHECK(write_io->kind == AST_IO_WRITE);
    CHECK(write_io->as.value != NULL);

    {
        AstIo *saved_io = read_statement->as.io;

        read_statement->as.io = NULL;
        valid = expect_internal_failure(program);
        read_statement->as.io = saved_io;
        CHECK(valid);
    }
    {
        AstLValue *saved_target = read_io->as.target;

        read_io->as.target = NULL;
        valid = expect_internal_failure(program);
        read_io->as.target = saved_target;
        CHECK(valid);
    }
    {
        AstExpression *saved_value = write_io->as.value;

        write_io->as.value = NULL;
        valid = expect_internal_failure(program);
        write_io->as.value = saved_value;
        CHECK(valid);
    }
    {
        AstIoKind saved_kind = read_io->kind;

        read_io->kind = (AstIoKind)999;
        valid = expect_internal_failure(program);
        read_io->kind = saved_kind;
        CHECK(valid);
    }
    parse_result_destroy(&parse_result);
    return true;
}

static bool test_vectors_records_and_bounds_policy(void)
{
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};

    CHECK(analyze_source(
        "inteira vet{5} v;"
        "inteira principal() @["
        "inteira i; inteira x; i receba 0;"
        "x receba v{i}; v{i} receba x; x receba v{4}; x receba v{2 * 2};"
        "]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    /* D40: indice constante fora dos limites e erro de compilacao. */
    CHECK(analyze_source(
        "inteira vet{5} v;"
        "inteira principal() @[inteira x; x receba v{999}; v{0 - 1} receba x;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 2);
    CHECK(has_code(&result, SEM_INDEX_OUT_OF_BOUNDS));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira x; inteira vet{0} v;"
        "inteira principal() @[inteira y; y receba x{0}; y receba v;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(has_code(&result, SEM_INVALID_VECTOR_SIZE));
    CHECK(has_code(&result, SEM_NOT_A_VECTOR));
    CHECK(has_code(&result, SEM_NAME_KIND_MISMATCH));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "flut vet{2} v; inteira principal() @[inteira x; x receba v{1.5};]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(has_code(&result, SEM_INVALID_VECTOR_INDEX));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @["
        "inteira i; i receba 0;"
        "repete (i receba 0, i < 10, i receba i + 1) edai []"
        "]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "registro R @[inteira x; flut x;]"
        "inteira principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(has_code(&result, SEM_INVALID_RECORD_DECLARATION));
    destroy_analysis(&parse_result, &analyzer, &result);
    return true;
}

static bool test_multiple_errors_and_locations(void)
{
    const char *source =
        "inteira principal() @["
        "inteira i; i receba \"texto\";"
        "x receba 1;"
        "cond (1) edai []"
        "]";
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};
    const SemanticDiagnostic *undeclared;

    CHECK(analyze_source(
        source,
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 3);
    CHECK(has_code(&result, SEM_INVALID_ASSIGNMENT));
    CHECK(has_code(&result, SEM_UNDECLARED_IDENTIFIER));
    CHECK(has_code(&result, SEM_INVALID_CONDITION));
    undeclared = find_code(&result, SEM_UNDECLARED_IDENTIFIER);
    CHECK(undeclared != NULL);
    CHECK(undeclared->location.line == 1);
    CHECK(undeclared->location.column > 1);
    CHECK(undeclared->symbol_name_length == 1);
    CHECK(strcmp(undeclared->symbol_name, "x") == 0);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @[inteira y; y receba x + 1;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_UNDECLARED_IDENTIFIER));
    CHECK(!has_code(&result, SEM_INVALID_OPERATOR));
    destroy_analysis(&parse_result, &analyzer, &result);
    return true;
}

static bool test_semantic_smoke_and_growth(void)
{
    char source[8192];
    size_t used = 0;
    size_t i;
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};

    used += (size_t)snprintf(
        source + used,
        sizeof(source) - used,
        "inteira principal() @["
    );
    for (i = 0; i < 80 && used < sizeof(source); i++) {
        int written = snprintf(
            source + used,
            sizeof(source) - used,
            "inteira x%zu; x%zu receba %zu;",
            i,
            i,
            i
        );
        if (written < 0 || (size_t)written >= sizeof(source) - used) {
            return false;
        }
        used += (size_t)written;
    }
    if (used + 2 >= sizeof(source)) {
        return false;
    }
    source[used++] = ']';
    source[used] = '\0';

    CHECK(analyze_source(
        source,
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    CHECK(result.diagnostic_count == 0);
    destroy_analysis(&parse_result, &analyzer, &result);
    return true;
}

static bool test_assignability_matrix(void)
{
    static const char *const variable_names[] = {
        "vbool",
        "vinteira",
        "vflut",
        "vpalavra",
        "vduplo"
    };
    static const bool compatible[5][5] = {
        { true, false, false, false, false },
        { false, true, false, false, false },
        { false, true, true, false, false },
        { false, false, false, true, false },
        { false, true, true, false, true }
    };
    char source[1024];
    size_t destination;
    size_t source_type;

    for (destination = 0; destination < 5; destination++) {
        for (source_type = 0; source_type < 5; source_type++) {
            int written = snprintf(
                source,
                sizeof(source),
                "bool vbool; inteira vinteira; flut vflut; "
                "palavra vpalavra; duplocarpado vduplo; "
                "inteira principal() @ [%s receba %s;]",
                variable_names[destination],
                variable_names[source_type]
            );
            ParseResult parse_result = {0};
            SemanticAnalyzer analyzer = {0};
            SemanticResult result = {0};

            CHECK(written > 0 && (size_t)written < sizeof(source));
            CHECK(analyze_source(
                source,
                &parse_result,
                &analyzer,
                &result
            ));
            if (compatible[destination][source_type]) {
                CHECK(result.status == SEMANTIC_OK);
                CHECK(result.diagnostic_count == 0);
            } else {
                CHECK(result.status == SEMANTIC_ERRORS);
                CHECK(has_code(&result, SEM_INVALID_ASSIGNMENT));
            }
            destroy_analysis(&parse_result, &analyzer, &result);
        }
    }
    return true;
}

static bool test_adversarial_scopes_and_passes(void)
{
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};

    CHECK(analyze_source(
        "inteira x; flut x;"
        "inteira principal() @[inteira y; y receba x;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_DUPLICATE_DECLARATION));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira x;"
        "inteira principal() @["
        "flut x;"
        "[palavra x; palavra inner; inner receba x;]"
        "flut outer; outer receba x;"
        "inteira invalid; invalid receba x;"
        "]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_INVALID_ASSIGNMENT));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @["
        "cond (VER) edai [inteira branch;]"
        "casocontrario edai [branch receba 1;]"
        "]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_UNDECLARED_IDENTIFIER));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira f(inteira x) @[inteira x; respost x;]"
        "inteira principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(has_code(&result, SEM_DUPLICATE_DECLARATION));
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira f(inteira x) @["
        "[inteira x; respost x;] respost x;"
        "]inteira principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira a() @[respost b();]"
        "outrafuncao inteira b() @[respost c();]"
        "outrafuncao inteira c() @[respost d();]"
        "outrafuncao inteira d() @[respost 1;]"
        "inteira principal() @[inteira x; x receba a();]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "outrafuncao inteira a() @[respost b();]"
        "outrafuncao inteira b() @[respost c();]"
        "outrafuncao inteira c() @[respost a();]"
        "inteira principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @[inteira x; x receba global;]"
        "outrafuncao inteira f() @[respost global;]"
        "inteira global;",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(analyze_source(
        "inteira principal() @ []\n\n"
        "vazio principal() @ []\n\n"
        "flut principal() @ []",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 2);
    CHECK(result.diagnostics[0].location.line == 3);
    CHECK(result.diagnostics[0].location.column == 1);
    CHECK(result.diagnostics[1].location.line == 5);
    CHECK(result.diagnostics[1].location.column == 1);
    destroy_analysis(&parse_result, &analyzer, &result);
    return true;
}

static bool test_semantic_stress_growth(void)
{
    char source[180000];
    size_t used = 0;
    size_t i;
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};

    for (i = 0; i < 1000; i++) {
        int written = snprintf(
            source + used,
            sizeof(source) - used,
            "inteira g%zu;",
            i
        );
        CHECK(written > 0 && (size_t)written < sizeof(source) - used);
        used += (size_t)written;
    }
    {
        int written = snprintf(
            source + used,
            sizeof(source) - used,
            "inteira principal() @["
        );
        CHECK(written > 0 && (size_t)written < sizeof(source) - used);
        used += (size_t)written;
    }
    for (i = 0; i < 1000; i++) {
        int written = snprintf(
            source + used,
            sizeof(source) - used,
            "u%zu receba 1;",
            i
        );
        CHECK(written > 0 && (size_t)written < sizeof(source) - used);
        used += (size_t)written;
    }
    CHECK(used + 2 < sizeof(source));
    source[used++] = ']';
    source[used] = '\0';

    CHECK(analyze_source(
        source,
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1000);
    CHECK(has_code(&result, SEM_UNDECLARED_IDENTIFIER));
    destroy_analysis(&parse_result, &analyzer, &result);

    used = 0;
    {
        int written = snprintf(
            source + used,
            sizeof(source) - used,
            "inteira principal() @["
        );
        CHECK(written > 0 && (size_t)written < sizeof(source) - used);
        used += (size_t)written;
    }
    for (i = 0; i < 120; i++) {
        int written = snprintf(
            source + used,
            sizeof(source) - used,
            "[inteira x%zu; x%zu receba %zu;]",
            i,
            i,
            i
        );
        CHECK(written > 0 && (size_t)written < sizeof(source) - used);
        used += (size_t)written;
    }
    CHECK(used + 2 < sizeof(source));
    source[used++] = ']';
    source[used] = '\0';

    CHECK(analyze_source(
        source,
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);

    used = 0;
    for (i = 0; i < 300; i++) {
        int written = snprintf(
            source + used,
            sizeof(source) - used,
            "outrafuncao inteira f%zu() @[respost 1;]",
            i
        );
        CHECK(written > 0 && (size_t)written < sizeof(source) - used);
        used += (size_t)written;
    }
    {
        int written = snprintf(
            source + used,
            sizeof(source) - used,
            "inteira principal() @ []"
        );
        CHECK(written > 0 && (size_t)written < sizeof(source) - used);
        used += (size_t)written;
    }
    CHECK(used < sizeof(source));
    source[used] = '\0';

    CHECK(analyze_source(
        source,
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    destroy_analysis(&parse_result, &analyzer, &result);
    return true;
}

static bool test_vector_size_edges(void)
{
    char source[1024];
    size_t used = 0;
    size_t i;
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};
    int written;

    written = snprintf(
        source + used,
        sizeof(source) - used,
        "inteira vet{0001} a; inteira vet{0} b; "
        "inteira vet{000} c; inteira vet{"
    );
    CHECK(written > 0 && (size_t)written < sizeof(source) - used);
    used += (size_t)written;
    for (i = 0; i < 300; i++) {
        CHECK(used + 1 < sizeof(source));
        source[used++] = '9';
    }
    written = snprintf(
        source + used,
        sizeof(source) - used,
        "} d; inteira principal() @[inteira x; x receba d{0};]"
    );
    CHECK(written > 0 && (size_t)written < sizeof(source) - used);
    used += (size_t)written;
    CHECK(used < sizeof(source));
    source[used] = '\0';

    CHECK(analyze_source(
        source,
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 3);
    CHECK(has_code(&result, SEM_INVALID_VECTOR_SIZE));
    destroy_analysis(&parse_result, &analyzer, &result);
    return true;
}

static bool test_impossible_ast_is_internal_failure(void)
{
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};
    AstFunctionDecl *function;
    bool invalid_return_rejected;
    bool invalid_literal_rejected;

    CHECK(parse_source(
        "inteira principal() @ []",
        &parse_result
    ));
    CHECK(parse_result.status == PARSE_OK);
    CHECK(parse_result.ast != NULL);
    CHECK(parse_result.ast->count == 1);
    CHECK(parse_result.ast->items != NULL);
    CHECK(parse_result.ast->items[0] != NULL);
    CHECK(parse_result.ast->items[0]->kind == AST_TOP_FUNCTION);
    function = parse_result.ast->items[0]->as.function;
    CHECK(function != NULL);

    function->return_type.kind = (AstTypeKind)999;
    CHECK(semantic_init(&analyzer, parse_result.ast));
    result = semantic_analyze(&analyzer);
    invalid_return_rejected = result.status == SEMANTIC_INTERNAL_FAILURE;
    CHECK(result.diagnostic_count == 0);
    semantic_result_destroy(&result);
    semantic_destroy(&analyzer);
    parse_result_destroy(&parse_result);
    CHECK(invalid_return_rejected);

    parse_result = (ParseResult){0};
    analyzer = (SemanticAnalyzer){0};
    result = (SemanticResult){0};
    CHECK(parse_source(
        "inteira principal() @[inteira x; x receba 1;]",
        &parse_result
    ));
    CHECK(parse_result.status == PARSE_OK);
    CHECK(parse_result.ast != NULL);
    CHECK(parse_result.ast->count == 1);
    function = parse_result.ast->items[0]->as.function;
    CHECK(function != NULL);
    CHECK(function->body != NULL);
    CHECK(function->body->statement_count == 2);
    CHECK(function->body->statements != NULL);
    CHECK(function->body->statements[1] != NULL);
    CHECK(function->body->statements[1]->kind == AST_STMT_ASSIGNMENT);
    CHECK(function->body->statements[1]->as.assignment != NULL);
    CHECK(function->body->statements[1]->as.assignment->value != NULL);

    {
        AstExpression *literal =
            function->body->statements[1]->as.assignment->value;
        char *saved_lexeme = literal->as.literal.lexeme;
        size_t saved_length = literal->as.literal.lexeme_length;

        CHECK(literal->kind == AST_EXPR_LITERAL);
        CHECK(literal->as.literal.kind == AST_LITERAL_INTEGER);
        literal->as.literal.lexeme = NULL;
        literal->as.literal.lexeme_length = 1;

        CHECK(semantic_init(&analyzer, parse_result.ast));
        result = semantic_analyze(&analyzer);
        invalid_literal_rejected =
            result.status == SEMANTIC_INTERNAL_FAILURE;
        CHECK(result.diagnostic_count == 0);

        literal->as.literal.lexeme = saved_lexeme;
        literal->as.literal.lexeme_length = saved_length;
    }

    semantic_result_destroy(&result);
    semantic_destroy(&analyzer);
    parse_result_destroy(&parse_result);
    CHECK(invalid_literal_rejected);

    parse_result = (ParseResult){0};
    analyzer = (SemanticAnalyzer){0};
    result = (SemanticResult){0};
    CHECK(parse_source(
        "inteira principal() @ []",
        &parse_result
    ));
    CHECK(parse_result.status == PARSE_OK);
    function = parse_result.ast->items[0]->as.function;
    CHECK(function != NULL);
    {
        char *saved_name = function->name;
        char *invalid_name = malloc(2);

        CHECK(invalid_name != NULL);
        invalid_name[0] = 'p';
        invalid_name[1] = '\0';
        function->name = invalid_name;
        CHECK(semantic_init(&analyzer, parse_result.ast));
        result = semantic_analyze(&analyzer);
        CHECK(result.status == SEMANTIC_INTERNAL_FAILURE);
        CHECK(result.diagnostic_count == 0);
        function->name = saved_name;
        free(invalid_name);
    }
    semantic_result_destroy(&result);
    semantic_destroy(&analyzer);
    parse_result_destroy(&parse_result);

    parse_result = (ParseResult){0};
    analyzer = (SemanticAnalyzer){0};
    result = (SemanticResult){0};
    CHECK(parse_source(
        "inteira principal() @ []",
        &parse_result
    ));
    CHECK(parse_result.status == PARSE_OK);
    function = parse_result.ast->items[0]->as.function;
    CHECK(function != NULL);
    {
        AstBlock *saved_body = function->body;

        function->body = NULL;
        CHECK(semantic_init(&analyzer, parse_result.ast));
        result = semantic_analyze(&analyzer);
        CHECK(result.status == SEMANTIC_INTERNAL_FAILURE);
        CHECK(result.diagnostic_count == 0);
        function->body = saved_body;
    }
    semantic_result_destroy(&result);
    semantic_destroy(&analyzer);
    parse_result_destroy(&parse_result);

    parse_result = (ParseResult){0};
    analyzer = (SemanticAnalyzer){0};
    result = (SemanticResult){0};
    CHECK(parse_source(
        "inteira principal(inteira p) @ []",
        &parse_result
    ));
    CHECK(parse_result.status == PARSE_OK);
    function = parse_result.ast->items[0]->as.function;
    CHECK(function != NULL);
    {
        AstParameter **saved_parameters = function->parameters;

        function->parameters = NULL;
        CHECK(semantic_init(&analyzer, parse_result.ast));
        result = semantic_analyze(&analyzer);
        CHECK(result.status == SEMANTIC_INTERNAL_FAILURE);
        CHECK(result.diagnostic_count == 0);
        function->parameters = saved_parameters;
    }
    semantic_result_destroy(&result);
    semantic_destroy(&analyzer);
    parse_result_destroy(&parse_result);

    return true;
}

static bool test_ast_corruption_boundaries(void)
{
    ParseResult parse_result = {0};
    AstProgram *program;
    AstTopLevelItem *item;
    AstFunctionDecl *function;
    AstStatement *statement;
    AstAssignment *assignment;
    AstExpression *literal;
    bool valid;

    CHECK(parse_source(
        "inteira principal() @[inteira x; x receba 1;]",
        &parse_result
    ));
    CHECK(parse_result.status == PARSE_OK);
    program = parse_result.ast;
    CHECK(program != NULL);
    CHECK(program->count == 1);
    CHECK(program->items != NULL);
    item = program->items[0];
    CHECK(item != NULL);
    CHECK(item->kind == AST_TOP_FUNCTION);
    function = item->as.function;
    CHECK(function != NULL);
    CHECK(function->body != NULL);
    CHECK(function->body->statement_count == 2);
    CHECK(function->body->statements != NULL);
    statement = function->body->statements[1];
    CHECK(statement != NULL);
    CHECK(statement->kind == AST_STMT_ASSIGNMENT);
    assignment = statement->as.assignment;
    CHECK(assignment != NULL);
    CHECK(assignment->target != NULL);
    CHECK(assignment->value != NULL);
    literal = assignment->value;
    CHECK(literal->kind == AST_EXPR_LITERAL);

    {
        AstTopLevelItem **saved_items = program->items;

        program->items = NULL;
        valid = expect_internal_failure(program);
        program->items = saved_items;
        CHECK(valid);
    }
    {
        AstTopLevelItem *saved_item = program->items[0];

        program->items[0] = NULL;
        valid = expect_internal_failure(program);
        program->items[0] = saved_item;
        CHECK(valid);
    }
    {
        AstTopLevelItemKind saved_kind = item->kind;

        item->kind = (AstTopLevelItemKind)999;
        valid = expect_internal_failure(program);
        item->kind = saved_kind;
        CHECK(valid);
    }
    {
        AstStatementKind saved_kind = statement->kind;

        statement->kind = (AstStatementKind)999;
        valid = expect_internal_failure(program);
        statement->kind = saved_kind;
        CHECK(valid);
    }
    {
        AstAssignment *saved_assignment = statement->as.assignment;

        statement->as.assignment = NULL;
        valid = expect_internal_failure(program);
        statement->as.assignment = saved_assignment;
        CHECK(valid);
    }
    {
        AstExpressionKind saved_kind = literal->kind;

        literal->kind = (AstExpressionKind)999;
        valid = expect_internal_failure(program);
        literal->kind = saved_kind;
        CHECK(valid);
    }
    {
        AstLiteralKind saved_kind = literal->as.literal.kind;

        literal->as.literal.kind = (AstLiteralKind)999;
        valid = expect_internal_failure(program);
        literal->as.literal.kind = saved_kind;
        CHECK(valid);
    }
    {
        AstLValueKind saved_kind = assignment->target->kind;

        assignment->target->kind = (AstLValueKind)999;
        valid = expect_internal_failure(program);
        assignment->target->kind = saved_kind;
        CHECK(valid);
    }
    parse_result_destroy(&parse_result);

    CHECK(parse_source(
        "inteira x; inteira principal() @ []",
        &parse_result
    ));
    CHECK(parse_result.status == PARSE_OK);
    program = parse_result.ast;
    CHECK(program != NULL);
    CHECK(program->count == 2);
    CHECK(program->items[0] != NULL);
    CHECK(program->items[0]->kind == AST_TOP_DECLARATION);
    {
        AstDeclaration *saved_declaration = program->items[0]->as.declaration;

        program->items[0]->as.declaration = NULL;
        valid = expect_internal_failure(program);
        program->items[0]->as.declaration = saved_declaration;
        CHECK(valid);
    }
    parse_result_destroy(&parse_result);

    CHECK(parse_source(
        "inteira vet{1} v; inteira principal() @ []",
        &parse_result
    ));
    CHECK(parse_result.status == PARSE_OK);
    program = parse_result.ast;
    CHECK(program != NULL);
    CHECK(program->items[0] != NULL);
    CHECK(program->items[0]->kind == AST_TOP_DECLARATION);
    {
        AstDeclaration *declaration = program->items[0]->as.declaration;
        AstDeclarator *declarator;
        char *saved_lexeme;
        size_t saved_length;

        CHECK(declaration != NULL);
        CHECK(declaration->declarator_count == 1);
        CHECK(declaration->declarators != NULL);
        declarator = declaration->declarators[0];
        CHECK(declarator != NULL);
        CHECK(declarator->kind == AST_DECLARATOR_VECTOR);
        saved_lexeme = declarator->vector_size_lexeme;
        saved_length = declarator->vector_size_length;

        declarator->vector_size_lexeme = NULL;
        valid = expect_internal_failure(program);
        declarator->vector_size_lexeme = saved_lexeme;
        CHECK(valid);

        declarator->vector_size_length = 0;
        valid = expect_internal_failure(program);
        declarator->vector_size_length = saved_length;
        CHECK(valid);

        declarator->vector_size_lexeme[0] = 'x';
        valid = expect_internal_failure(program);
        declarator->vector_size_lexeme[0] = '1';
        CHECK(valid);
    }
    parse_result_destroy(&parse_result);

    CHECK(parse_source(
        "registro R @[inteira x;] inteira principal() @ []",
        &parse_result
    ));
    CHECK(parse_result.status == PARSE_OK);
    program = parse_result.ast;
    CHECK(program != NULL);
    CHECK(program->items[0] != NULL);
    CHECK(program->items[0]->kind == AST_TOP_RECORD);
    {
        AstRecordDecl *saved_record = program->items[0]->as.record;

        program->items[0]->as.record = NULL;
        valid = expect_internal_failure(program);
        program->items[0]->as.record = saved_record;
        CHECK(valid);
    }
    {
        AstRecordDecl *record = program->items[0]->as.record;
        AstField **saved_fields = record->fields;

        CHECK(record != NULL);
        record->fields = NULL;
        valid = expect_internal_failure(program);
        record->fields = saved_fields;
        CHECK(valid);
    }
    parse_result_destroy(&parse_result);

    CHECK(parse_source(
        "outrafuncao inteira f(inteira p) @[respost p;]"
        "inteira principal() @ []",
        &parse_result
    ));
    CHECK(parse_result.status == PARSE_OK);
    program = parse_result.ast;
    CHECK(program != NULL);
    CHECK(program->items[0] != NULL);
    CHECK(program->items[0]->kind == AST_TOP_FUNCTION);
    function = program->items[0]->as.function;
    CHECK(function != NULL);
    {
        char *saved_name = function->name;

        function->name = NULL;
        valid = expect_internal_failure(program);
        function->name = saved_name;
        CHECK(valid);
    }
    {
        AstParameter *saved_parameter = function->parameters[0];

        function->parameters[0] = NULL;
        valid = expect_internal_failure(program);
        function->parameters[0] = saved_parameter;
        CHECK(valid);
    }
    {
        char *saved_name = function->parameters[0]->name;

        function->parameters[0]->name = NULL;
        valid = expect_internal_failure(program);
        function->parameters[0]->name = saved_name;
        CHECK(valid);
    }
    {
        AstTypeKind saved_kind = function->parameters[0]->type.kind;

        function->parameters[0]->type.kind = (AstTypeKind)999;
        valid = expect_internal_failure(program);
        function->parameters[0]->type.kind = saved_kind;
        CHECK(valid);
    }
    parse_result_destroy(&parse_result);

    CHECK(parse_source(
        "outrafuncao inteira f(inteira p) @[respost p;]"
        "inteira principal() @[inteira x; x receba f(1);]",
        &parse_result
    ));
    CHECK(parse_result.status == PARSE_OK);
    program = parse_result.ast;
    CHECK(program != NULL);
    function = program->items[1]->as.function;
    CHECK(function != NULL);
    statement = function->body->statements[1];
    CHECK(statement != NULL);
    CHECK(statement->kind == AST_STMT_ASSIGNMENT);
    assignment = statement->as.assignment;
    CHECK(assignment != NULL);
    literal = assignment->value;
    CHECK(literal != NULL);
    CHECK(literal->kind == AST_EXPR_REFERENCE);
    CHECK(literal->as.reference.kind == AST_REFERENCE_CALL);
    {
        AstExpression **saved_arguments = literal->as.reference.as.call.arguments;

        literal->as.reference.as.call.arguments = NULL;
        valid = expect_internal_failure(program);
        literal->as.reference.as.call.arguments = saved_arguments;
        CHECK(valid);
    }
    parse_result_destroy(&parse_result);
    return true;
}

static bool test_error_then_internal_failure(void)
{
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};
    AstFunctionDecl *function;
    AstStatement *statement;
    AstExpression *literal;
    char *saved_lexeme;
    size_t saved_length;

    CHECK(parse_source(
        "inteira principal() @["
        "x receba 1; inteira y; y receba 2;"
        "]",
        &parse_result
    ));
    CHECK(parse_result.status == PARSE_OK);
    function = parse_result.ast->items[0]->as.function;
    CHECK(function != NULL);
    CHECK(function->body->statement_count == 3);
    statement = function->body->statements[2];
    CHECK(statement != NULL);
    CHECK(statement->kind == AST_STMT_ASSIGNMENT);
    literal = statement->as.assignment->value;
    CHECK(literal != NULL);
    CHECK(literal->kind == AST_EXPR_LITERAL);
    saved_lexeme = literal->as.literal.lexeme;
    saved_length = literal->as.literal.lexeme_length;
    literal->as.literal.lexeme = NULL;
    literal->as.literal.lexeme_length = 1;

    CHECK(semantic_init(&analyzer, parse_result.ast));
    result = semantic_analyze(&analyzer);
    literal->as.literal.lexeme = saved_lexeme;
    literal->as.literal.lexeme_length = saved_length;
    CHECK(result.status == SEMANTIC_INTERNAL_FAILURE);
    CHECK(result.diagnostic_count == 1);
    CHECK(result.diagnostics[0].code == SEM_UNDECLARED_IDENTIFIER);
    semantic_destroy(&analyzer);
    parse_result_destroy(&parse_result);
    CHECK(result.diagnostics[0].symbol_name != NULL);
    CHECK(strcmp(result.diagnostics[0].symbol_name, "x") == 0);
    semantic_result_destroy(&result);
    return true;
}

/* ------------------------------------------------------------------ */
/* Testes das correcoes pos-PjBL1 (D33, D34, D18 com dica de //)       */
/* ------------------------------------------------------------------ */

/* Analisa e devolve so o status; *count recebe o numero de erros. */
static SemanticStatus status_of(
    const char *source,
    size_t *count,
    SemanticDiagnosticCode *first_code
)
{
    ParseResult parse_result;
    SemanticAnalyzer analyzer;
    SemanticResult result;
    SemanticStatus status = SEMANTIC_INTERNAL_FAILURE;

    if (analyze_source(source, &parse_result, &analyzer, &result)) {
        status = result.status;
        if (count != NULL) {
            *count = result.diagnostic_count;
        }
        if (first_code != NULL && result.diagnostic_count > 0) {
            *first_code = result.diagnostics[0].code;
        }
    }
    destroy_analysis(&parse_result, &analyzer, &result);
    return status;
}

static bool test_void_function_call_statement(void)
{
    size_t count = 0;
    SemanticDiagnosticCode code = SEM_INTERNAL;

    /* A correcao 2.3: funcao vazio agora pode ser chamada como comando. */
    CHECK(status_of(
        "outrafuncao vazio mostra(inteira x) @ [imprime(x);]"
        "inteira principal() @ [mostra(1); mostra(2 + 3); respost 0;]",
        &count,
        &code
    ) == SEMANTIC_OK);

    /* Funcao com valor tambem pode ser chamada como comando (descarta). */
    CHECK(status_of(
        "outrafuncao inteira f() @ [respost 1;]"
        "inteira principal() @ [f(); respost 0;]",
        &count,
        &code
    ) == SEMANTIC_OK);

    /* Em expressao, vazio continua nao sendo valor (D16). */
    CHECK(status_of(
        "outrafuncao vazio mostra() @ []"
        "inteira principal() @ [inteira a; a receba mostra(); respost 0;]",
        &count,
        &code
    ) == SEMANTIC_ERRORS);
    CHECK(count == 1 && code == SEM_VOID_VALUE);

    /* Os argumentos continuam verificados no comando de chamada. */
    CHECK(status_of(
        "outrafuncao vazio mostra(inteira x) @ []"
        "inteira principal() @ [mostra(VER); mostra(); respost 0;]",
        &count,
        &code
    ) == SEMANTIC_ERRORS);
    CHECK(count == 2);

    CHECK(status_of(
        "inteira principal() @ [naoexiste(1); respost 0;]",
        &count,
        &code
    ) == SEMANTIC_ERRORS);
    CHECK(count == 1 && code == SEM_UNKNOWN_FUNCTION);

    CHECK(status_of(
        "inteira x; inteira principal() @ [x(1); respost 0;]",
        &count,
        &code
    ) == SEMANTIC_ERRORS);
    CHECK(count == 1 && code == SEM_NOT_A_FUNCTION);
    return true;
}

static bool test_return_on_all_paths(void)
{
    static const char *valid[] = {
        "outrafuncao inteira f(inteira x) @ ["
        "  cond (x > 0) edai [respost 1;]"
        "  casocontrario cond (x < 0) edai [respost 2;]"
        "  casocontrario edai [respost 3;]"
        "] inteira principal() @ []",
        "outrafuncao inteira f() @ [[[respost 1;]]] inteira principal() @ []",
        "outrafuncao inteira f() @ [durante (VER) edai []]"
        " inteira principal() @ []",
        "outrafuncao inteira f() @ [inteira i;"
        " repete (i receba 0, VER, i receba i + 1) edai [respost i;]]"
        " inteira principal() @ []",
        "outrafuncao inteira f() @ [respost 1; inteira x;]"
        " inteira principal() @ []",
        "outrafuncao vazio f() @ [] inteira principal() @ []",
        /* principal pode omitir respost: retorno implicito do zero. */
        "flut principal() @ [flut x; x receba 1.5;]"
    };
    static const char *invalid[] = {
        "outrafuncao inteira f() @ [] inteira principal() @ []",
        "outrafuncao inteira f(inteira x) @ [cond (x > 0) edai [respost 1;]]"
        " inteira principal() @ []",
        "outrafuncao inteira f(inteira x) @ ["
        "  cond (x > 0) edai [respost 1;]"
        "  casocontrario cond (x < 0) edai [inteira y;]"
        "  casocontrario edai [respost 3;]"
        "] inteira principal() @ []",
        "outrafuncao inteira f(inteira x) @ ["
        "  durante (x > 0) edai [respost 1;]"
        "] inteira principal() @ []",
        "outrafuncao bool f() @ [durante (FAL) edai [respost VER;]]"
        " inteira principal() @ []"
    };
    size_t i;

    for (i = 0; i < sizeof(valid) / sizeof(valid[0]); i++) {
        CHECK(status_of(valid[i], NULL, NULL) == SEMANTIC_OK);
    }
    for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        size_t count = 0;
        SemanticDiagnosticCode code = SEM_INTERNAL;

        CHECK(status_of(invalid[i], &count, &code) == SEMANTIC_ERRORS);
        CHECK(count == 1 && code == SEM_MISSING_RETURN);
    }
    return true;
}

static bool test_division_hint(void)
{
    ParseResult parse_result;
    SemanticAnalyzer analyzer;
    SemanticResult result;

    CHECK(analyze_source(
        "inteira principal() @ [inteira a; a receba 4 / 2; respost a;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_ERRORS);
    CHECK(result.diagnostic_count == 1);
    CHECK(result.diagnostics[0].code == SEM_INVALID_ASSIGNMENT);
    CHECK(strstr(result.diagnostics[0].message, "//") != NULL);
    destroy_analysis(&parse_result, &analyzer, &result);

    CHECK(status_of(
        "inteira principal() @ [inteira a; a receba 4 // 2; respost a;]",
        NULL,
        NULL
    ) == SEMANTIC_OK);
    return true;
}

/* Muitos simbolos no mesmo escopo: exercita o crescimento do indice hash. */
static bool test_symbol_table_many_names(void)
{
    const size_t names = 5000;
    size_t length = 64 + names * 24;
    char *source = malloc(length);
    char *cursor = source;
    size_t i;
    size_t count = 0;
    SemanticDiagnosticCode code = SEM_INTERNAL;

    CHECK(source != NULL);
    for (i = 0; i < names; i++) {
        cursor += sprintf(cursor, "inteira v%zu;", i);
    }
    /* Duplicata no fim e uso do primeiro e do ultimo nome. */
    cursor += sprintf(cursor, "inteira v17;");
    sprintf(
        cursor,
        "inteira principal() @ [v0 receba v%zu; respost v0;]",
        names - 1
    );
    CHECK(status_of(source, &count, &code) == SEMANTIC_ERRORS);
    free(source);
    CHECK(count == 1 && code == SEM_DUPLICATE_DECLARATION);
    return true;
}

/* ------------------------------------------------------------------ */
/* PjBL 3/4: registros (D39), D40, DSL (D41), embutidas (D44), alvo     */
/* ------------------------------------------------------------------ */

/* Espera exatamente um diagnostico, com o codigo dado. */
static bool single_error(const char *source, SemanticDiagnosticCode wanted)
{
    size_t count = 0;
    SemanticDiagnosticCode code = SEM_INTERNAL;

    return status_of(source, &count, &code) == SEMANTIC_ERRORS &&
           count == 1 && code == wanted;
}

static bool test_records_as_types(void)
{
    CHECK(status_of(
        "registro Ponto @ [inteira x; flut y;]"
        "registro Ponto origem;"
        "inteira principal() @ ["
        "  registro Ponto p, q;"
        "  registro Ponto vet{3} pts;"
        "  p.x receba 1; p.y receba p.x * 2;"
        "  pts{2}.x receba p.x + pts{0}.x;"
        "  q receba p; origem receba pts{1};"
        "  imprime(q.y);"
        "  respost origem.x;"
        "]",
        NULL, NULL
    ) == SEMANTIC_OK);
    /* registro usado antes da definicao (coletado na passada 1) */
    CHECK(status_of(
        "registro R g; registro R @ [bool b;] inteira principal() @ [respost 0;]",
        NULL, NULL
    ) == SEMANTIC_OK);
    CHECK(single_error(
        "inteira principal() @ [registro Nada x; respost 0;]",
        SEM_UNDECLARED_IDENTIFIER
    ));
    CHECK(single_error(
        "inteira q; inteira principal() @ [registro q x; respost 0;]",
        SEM_NAME_KIND_MISMATCH
    ));
    CHECK(single_error(
        "registro P @ [inteira x;] registro P p;"
        "inteira principal() @ [p.z receba 1; respost 0;]",
        SEM_INVALID_FIELD
    ));
    CHECK(single_error(
        "inteira principal() @ [inteira n; n.x receba 1; respost 0;]",
        SEM_INVALID_FIELD
    ));
    CHECK(single_error(
        "registro A @ [inteira x;] registro B @ [inteira x;]"
        "registro A a; registro B b;"
        "inteira principal() @ [a receba b; respost 0;]",
        SEM_INVALID_ASSIGNMENT
    ));
    /* registro inteiro nao e valor escalar */
    CHECK(single_error(
        "registro P @ [inteira x;] registro P p;"
        "inteira principal() @ [imprime(p); respost 0;]",
        SEM_TYPE_MISMATCH
    ));
    /* D51: variavel de registro sem a palavra registro (global e local,
     * escalar e vetor), como nos programas de teste do professor */
    CHECK(status_of(
        "registro Cuidador @ [palavra nome; inteira idade;]"
        "Cuidador chefe;"
        "inteira principal() @ ["
        "  Cuidador responsavel;"
        "  Cuidador vet{3} equipe;"
        "  responsavel.idade receba 34;"
        "  equipe{1} receba responsavel;"
        "  chefe receba equipe{1};"
        "  respost chefe.idade;"
        "]",
        NULL, NULL
    ) == SEMANTIC_OK);
    /* D51: o nome precisa ser um registro */
    CHECK(single_error(
        "inteira principal() @ [Nada x; respost 0;]",
        SEM_UNDECLARED_IDENTIFIER
    ));
    CHECK(single_error(
        "inteira q; inteira principal() @ [q x; respost 0;]",
        SEM_NAME_KIND_MISMATCH
    ));
    return true;
}

/* D50: literal maior que i32 e aviso (programa valido), nao erro. */
static size_t warnings_of(const char *source, SemanticStatus *status)
{
    ParseResult parse_result;
    SemanticAnalyzer analyzer;
    SemanticResult result;
    size_t count = (size_t)-1;

    *status = SEMANTIC_INTERNAL_FAILURE;
    if (analyze_source(source, &parse_result, &analyzer, &result)) {
        *status = result.status;
        count = result.warning_count;
    }
    destroy_analysis(&parse_result, &analyzer, &result);
    return count;
}

static bool test_literal_wrap_warning(void)
{
    SemanticStatus status;

    CHECK(warnings_of("inteira principal() @ [respost 2147483648;]",
                      &status) == 1 && status == SEMANTIC_OK);
    CHECK(warnings_of(
        "inteira vet{2} doc; inteira principal() @ ["
        "  doc{0} receba 12345678901; doc{1} receba 98765432100;"
        "  respost 0;]",
        &status) == 2 && status == SEMANTIC_OK);
    /* o maior inteira cabe: sem aviso */
    CHECK(warnings_of("inteira principal() @ [respost 2147483647;]",
                      &status) == 0 && status == SEMANTIC_OK);
    /* aviso nao esconde erro de verdade */
    CHECK(warnings_of(
        "inteira principal() @ [respost 12345678901 + naoexiste;]",
        &status) == 1 && status == SEMANTIC_ERRORS);
    return true;
}

static bool test_static_checks(void)
{
    CHECK(single_error(
        "inteira vet{4} v; inteira principal() @ [respost v{4};]",
        SEM_INDEX_OUT_OF_BOUNDS
    ));
    CHECK(single_error(
        "inteira vet{4} v; inteira principal() @ [respost v{2 - 3};]",
        SEM_INDEX_OUT_OF_BOUNDS
    ));
    CHECK(single_error(
        "inteira principal() @ [respost 10 // (3 - 3);]",
        SEM_DIVISION_BY_ZERO
    ));
    CHECK(single_error(
        "inteira principal() @ [flut f; f receba 1 / 0.0; respost 0;]",
        SEM_DIVISION_BY_ZERO
    ));
    /* 2147483648 deixou de ser erro (D50): ver test_literal_wrap_warning */
    /* D38: duplocarpado / duplocarpado e duplocarpado (nao cabe em flut) */
    CHECK(single_error(
        "inteira principal() @ [duplocarpado d; flut f; f receba d / d;"
        " respost 0;]",
        SEM_INVALID_ASSIGNMENT
    ));
    CHECK(status_of(
        "inteira principal() @ [duplocarpado d; d receba d / 2;"
        " d receba raiz(d); respost 0;]",
        NULL, NULL
    ) == SEMANTIC_OK);
    return true;
}

static bool test_vector_dsl(void)
{
    CHECK(status_of(
        "inteira vet{8} a, b, c; flut vet{8} f; duplocarpado vet{8} d;"
        "inteira principal() @ ["
        "  inteira k; flut s;"
        "  c receba a + b; c receba 3 * a - b * k; c receba -a; c receba 0;"
        "  f receba a * 0.5; d receba f + a; f receba a;"
        "  k receba somavet(a) + maxvet(b) - minvet(c);"
        "  s receba somavet(f) + prodesc(a, f) + somavet(a * b + c);"
        "  respost k;"
        "]",
        NULL, NULL
    ) == SEMANTIC_OK);
    /* D46: vet{8} vale para todos os nomes seguintes */
    CHECK(status_of(
        "inteira vet{8} a, b; inteira principal() @ [b{7} receba a{7};"
        " respost 0;]",
        NULL, NULL
    ) == SEMANTIC_OK);
    CHECK(single_error(
        "inteira vet{4} a; inteira vet{5} b; inteira principal() @ ["
        "b receba a; respost 0;]",
        SEM_VECTOR_SIZE_MISMATCH
    ));
    CHECK(single_error(
        "inteira vet{4} a; inteira vet{5} b, c; inteira principal() @ ["
        "c receba a + b; respost 0;]",
        SEM_VECTOR_SIZE_MISMATCH
    ));
    CHECK(single_error(
        "inteira vet{4} a; flut vet{4} f; inteira principal() @ ["
        "a receba f; respost 0;]",
        SEM_INVALID_ASSIGNMENT
    ));
    CHECK(single_error(
        "inteira vet{4} a, b; inteira principal() @ [a receba a / b;"
        " respost 0;]",
        SEM_INVALID_VECTOR_OPERATION
    ));
    CHECK(single_error(
        "bool vet{4} m; inteira principal() @ [m receba m; respost 0;]",
        SEM_INVALID_VECTOR_OPERATION
    ));
    CHECK(single_error(
        "inteira vet{4} a; inteira principal() @ [imprime(a + 1); respost 0;]",
        SEM_INVALID_VECTOR_OPERATION
    ));
    CHECK(single_error(
        "inteira vet{4} a; inteira principal() @ [respost somavet(a{0});]",
        SEM_ARGUMENT_TYPE_MISMATCH
    ));
    CHECK(single_error(
        "inteira vet{4} a; inteira principal() @ [respost somavet(a, a);]",
        SEM_ARGUMENT_COUNT_MISMATCH
    ));
    CHECK(single_error(
        "inteira vet{4} a; inteira vet{3} b;"
        "inteira principal() @ [respost prodesc(a, b);]",
        SEM_VECTOR_SIZE_MISMATCH
    ));
    /* nomes embutidos ocupam o namespace global */
    CHECK(single_error(
        "inteira somavet; inteira principal() @ [respost 0;]",
        SEM_DUPLICATE_DECLARATION
    ));
    return true;
}

static bool test_physical_io_builtins(void)
{
    CHECK(status_of(
        "inteira principal() @ ["
        "  pinomodo(13, 1); pinoescreve(13, pinole(2)); espera(100);"
        "  respost pinole(2);"
        "]",
        NULL, NULL
    ) == SEMANTIC_OK);
    CHECK(single_error(
        "inteira principal() @ [inteira x; x receba espera(1); respost 0;]",
        SEM_VOID_VALUE
    ));
    CHECK(single_error(
        "inteira principal() @ [pinomodo(13); respost 0;]",
        SEM_ARGUMENT_COUNT_MISMATCH
    ));
    CHECK(single_error(
        "inteira principal() @ [pinoescreve(13, 0.5); respost 0;]",
        SEM_ARGUMENT_TYPE_MISMATCH
    ));
    return true;
}

/* Analisa para o alvo dado e devolve status e contagens de memoria. */
static SemanticStatus status_for_target(
    const char *source,
    SemanticTarget target,
    SemanticDiagnosticCode *first_code,
    size_t *global_bytes,
    size_t *frame_bytes
)
{
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult result = {0};
    SemanticStatus status = SEMANTIC_INTERNAL_FAILURE;

    if (parse_source(source, &parse_result) &&
        parse_result.status == PARSE_OK &&
        semantic_init(&analyzer, parse_result.ast)) {
        semantic_set_target(&analyzer, target);
        result = semantic_analyze(&analyzer);
        status = result.status;
        if (first_code != NULL && result.diagnostic_count > 0) {
            *first_code = result.diagnostics[0].code;
        }
        *global_bytes = result.global_bytes;
        *frame_bytes = result.max_frame_bytes;
    }
    destroy_analysis(&parse_result, &analyzer, &result);
    return status;
}

static bool test_target_memory(void)
{
    const char *large =
        "inteira vet{200} historico;"
        "outrafuncao flut media() @ [flut vet{300} a; inteira i;"
        " flut soma; soma receba 0; respost soma;]"
        "inteira principal() @ [imprime(media()); respost 0;]";
    const char *fits =
        "registro P @ [inteira x; bool b; duplocarpado d;]"
        "registro P vet{10} pts;"
        "inteira principal() @ [palavra s; s receba \"abc\";"
        " [inteira vet{5} t;] respost 0;]";
    SemanticDiagnosticCode code = SEM_INTERNAL;
    size_t globals = 0;
    size_t frame = 0;

    /* PC aceita; Uno rejeita: 800 + 1208 > 2048 - 512. */
    CHECK(status_for_target(large, SEMANTIC_TARGET_PC, NULL, &globals,
                            &frame) == SEMANTIC_OK);
    CHECK(status_for_target(large, SEMANTIC_TARGET_UNO, &code, &globals,
                            &frame) == SEMANTIC_ERRORS);
    CHECK(code == SEM_TARGET_MEMORY);
    CHECK(globals == 800);
    CHECK(frame == 1208);

    /* Tamanhos do AVR: registro 4+1+8 = 13 bytes x 10; texto "abc" = 4;
     * quadro: palavra (2) + vetor de bloco interno (5 x 4) = 22. */
    CHECK(status_for_target(fits, SEMANTIC_TARGET_UNO, NULL, &globals,
                            &frame) == SEMANTIC_OK);
    CHECK(globals == 134);
    CHECK(frame == 22);
    return true;
}

static bool test_annotations_for_codegen(void)
{
    ParseResult parse_result;
    SemanticAnalyzer analyzer;
    SemanticResult result;
    const AstFunctionDecl *main_function;
    const AstExpression *value;

    CHECK(analyze_source(
        "inteira g;"
        "inteira principal() @ [flut f; f receba g + 1.5; respost g;]",
        &parse_result,
        &analyzer,
        &result
    ));
    CHECK(result.status == SEMANTIC_OK);
    CHECK(result.symbol_count == 2);
    main_function = parse_result.ast->items[1]->as.function;
    value = main_function->body->statements[1]->as.assignment->value;
    CHECK(value->type.valid && value->type.kind == AST_TYPE_FLUT);
    CHECK(value->as.binary.left->type.kind == AST_TYPE_INTEIRA);
    CHECK(value->as.binary.left->as.reference.symbol_id ==
          parse_result.ast->items[0]->as.declaration->declarators[0]
              ->symbol_id);
    CHECK(main_function->body->statements[1]->as.assignment->target
              ->symbol_id ==
          main_function->body->statements[0]->as.declaration
              ->declarators[0]->symbol_id);
    destroy_analysis(&parse_result, &analyzer, &result);
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
    run_test("lifecycle_and_ownership", test_lifecycle_and_ownership);
    run_test("principal_and_namespace", test_principal_and_namespace);
    run_test("declarations_and_scopes", test_declarations_and_scopes);
    run_test(
        "globals_forward_calls_and_recursion",
        test_globals_forward_calls_and_recursion
    );
    run_test("shadowing_and_name_kinds", test_shadowing_and_name_kinds);
    run_test("assignability_and_returns", test_assignability_and_returns);
    run_test("operators_and_conditions", test_operators_and_conditions);
    run_test("calls_and_void_values", test_calls_and_void_values);
    run_test("io_semantics", test_io_semantics);
    run_test("io_impossible_ast", test_io_impossible_ast);
    run_test(
        "vectors_records_and_bounds_policy",
        test_vectors_records_and_bounds_policy
    );
    run_test(
        "multiple_errors_and_locations",
        test_multiple_errors_and_locations
    );
    run_test(
        "semantic_smoke_and_growth",
        test_semantic_smoke_and_growth
    );
    run_test(
        "impossible_ast_is_internal_failure",
        test_impossible_ast_is_internal_failure
    );
    run_test(
        "ast_corruption_boundaries",
        test_ast_corruption_boundaries
    );
    run_test(
        "error_then_internal_failure",
        test_error_then_internal_failure
    );
    run_test("assignability_matrix", test_assignability_matrix);
    run_test(
        "adversarial_scopes_and_passes",
        test_adversarial_scopes_and_passes
    );
    run_test("semantic_stress_growth", test_semantic_stress_growth);
    run_test("vector_size_edges", test_vector_size_edges);
    run_test(
        "void_function_call_statement",
        test_void_function_call_statement
    );
    run_test("return_on_all_paths", test_return_on_all_paths);
    run_test("division_hint", test_division_hint);
    run_test("symbol_table_many_names", test_symbol_table_many_names);
    run_test("records_as_types", test_records_as_types);
    run_test("literal_wrap_warning", test_literal_wrap_warning);
    run_test("static_checks", test_static_checks);
    run_test("vector_dsl", test_vector_dsl);
    run_test("physical_io_builtins", test_physical_io_builtins);
    run_test("target_memory", test_target_memory);
    run_test("annotations_for_codegen", test_annotations_for_codegen);

    printf("%zu assertions, %zu failures\n", tests_run, tests_failed);
    return tests_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
