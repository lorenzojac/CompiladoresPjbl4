#include "lexer.h"

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

static bool init_lexer(Lexer *lexer, const char *source)
{
    *lexer = (Lexer){0};
    return lexer_init_from_string(lexer, source);
}

static bool expect_token(
    Lexer *lexer,
    TokenType type,
    const char *lexeme,
    size_t line,
    size_t column
)
{
    Token token = {0};
    bool result = lexer_next_token(lexer, &token);

    CHECK(result);
    CHECK(token.type == type);
    CHECK(token.lexeme_length == (lexeme == NULL ? 0 : strlen(lexeme)));
    if (lexeme == NULL) {
        CHECK(token.lexeme == NULL);
        CHECK(token.error_message == NULL);
    } else {
        CHECK(token.lexeme != NULL);
        CHECK(memcmp(token.lexeme, lexeme, token.lexeme_length) == 0);
        CHECK(token.error_message == NULL);
    }
    CHECK(token.line == line);
    CHECK(token.column == column);
    token_destroy(&token);
    return true;
}

static bool expect_invalid(Lexer *lexer, const char *lexeme)
{
    Token token = {0};

    CHECK(lexer_next_token(lexer, &token));
    CHECK(token.type == TOK_INVALIDO);
    CHECK(token.lexeme != NULL);
    CHECK(token.lexeme_length == strlen(lexeme));
    CHECK(strcmp(token.lexeme, lexeme) == 0);
    CHECK(token.error_message != NULL);
    token_destroy(&token);
    return true;
}

static bool expect_invalid_bytes(
    Lexer *lexer,
    const unsigned char *lexeme,
    size_t lexeme_length
)
{
    Token token = {0};

    CHECK(lexer_next_token(lexer, &token));
    CHECK(token.type == TOK_INVALIDO);
    CHECK(token.lexeme != NULL);
    CHECK(token.lexeme_length == lexeme_length);
    CHECK(memcmp(token.lexeme, lexeme, lexeme_length) == 0);
    CHECK(token.error_message != NULL);
    token_destroy(&token);
    return true;
}

static bool test_keywords(void)
{
    static const struct {
        const char *source;
        TokenType type;
    } cases[] = {
        {"receba", TOK_RECEBA}, {"repete", TOK_REPETE},
        {"durante", TOK_DURANTE}, {"cond", TOK_COND},
        {"casocontrario", TOK_CASOCONTRARIO}, {"func", TOK_FUNC},
        {"bool", TOK_BOOL}, {"inteira", TOK_INTEIRA},
        {"flut", TOK_FLUT}, {"palavra", TOK_PALAVRA},
        {"duplocarpado", TOK_DUPLOCARPADO}, {"principal", TOK_PRINCIPAL},
        {"vazio", TOK_VAZIO}, {"respost", TOK_RESPOST},
        {"registro", TOK_REGISTRO}, {"outrafuncao", TOK_OUTRAFUNCAO},
        {"vet", TOK_VET}, {"edai", TOK_EDAI}, {"raiz", TOK_RAIZ},
        {"le", TOK_LE}, {"imprime", TOK_IMPRIME},
        {"OU", TOK_OU}, {"E", TOK_E}, {"XOR", TOK_XOR},
        {"VER", TOK_VER}, {"FAL", TOK_FAL}, {"Inteira", TOK_ID}
    };
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        Lexer lexer;
        CHECK(init_lexer(&lexer, cases[i].source));
        CHECK(expect_token(&lexer, cases[i].type, cases[i].source, 1, 1));
        CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, strlen(cases[i].source) + 1));
        lexer_destroy(&lexer);
    }

    return true;
}

static bool test_io_keyword_boundaries_and_case(void)
{
    Lexer lexer;

    CHECK(init_lexer(&lexer, "le imprime"));
    CHECK(expect_token(&lexer, TOK_LE, "le", 1, 1));
    CHECK(expect_token(&lexer, TOK_IMPRIME, "imprime", 1, 4));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 11));
    lexer_destroy(&lexer);

    CHECK(init_lexer(
        &lexer,
        "leia ler letra le1 leabc imprimex imprimir imprime1 impressao"
    ));
    CHECK(expect_token(&lexer, TOK_ID, "leia", 1, 1));
    CHECK(expect_token(&lexer, TOK_ID, "ler", 1, 6));
    CHECK(expect_token(&lexer, TOK_ID, "letra", 1, 10));
    CHECK(expect_token(&lexer, TOK_ID, "le1", 1, 16));
    CHECK(expect_token(&lexer, TOK_ID, "leabc", 1, 20));
    CHECK(expect_token(&lexer, TOK_ID, "imprimex", 1, 26));
    CHECK(expect_token(&lexer, TOK_ID, "imprimir", 1, 35));
    CHECK(expect_token(&lexer, TOK_ID, "imprime1", 1, 44));
    CHECK(expect_token(&lexer, TOK_ID, "impressao", 1, 53));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 62));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "Le LE Imprime IMPRIME"));
    CHECK(expect_token(&lexer, TOK_ID, "Le", 1, 1));
    CHECK(expect_token(&lexer, TOK_ID, "LE", 1, 4));
    CHECK(expect_token(&lexer, TOK_ID, "Imprime", 1, 7));
    CHECK(expect_token(&lexer, TOK_ID, "IMPRIME", 1, 15));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 22));
    lexer_destroy(&lexer);
    return true;
}

static bool test_future_io_lexical_form(void)
{
    Lexer lexer;

    CHECK(init_lexer(&lexer, "le(x);\nimprime(x + 1);"));
    CHECK(expect_token(&lexer, TOK_LE, "le", 1, 1));
    CHECK(expect_token(&lexer, TOK_ABRE_PAREN, "(", 1, 3));
    CHECK(expect_token(&lexer, TOK_ID, "x", 1, 4));
    CHECK(expect_token(&lexer, TOK_FECHA_PAREN, ")", 1, 5));
    CHECK(expect_token(&lexer, TOK_PONTO_VIRGULA, ";", 1, 6));
    CHECK(expect_token(&lexer, TOK_IMPRIME, "imprime", 2, 1));
    CHECK(expect_token(&lexer, TOK_ABRE_PAREN, "(", 2, 8));
    CHECK(expect_token(&lexer, TOK_ID, "x", 2, 9));
    CHECK(expect_token(&lexer, TOK_MAIS, "+", 2, 11));
    CHECK(expect_token(&lexer, TOK_NUM_INTEIRO, "1", 2, 13));
    CHECK(expect_token(&lexer, TOK_FECHA_PAREN, ")", 2, 14));
    CHECK(expect_token(&lexer, TOK_PONTO_VIRGULA, ";", 2, 15));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 2, 16));
    lexer_destroy(&lexer);
    return true;
}

static bool test_identifiers_and_nao(void)
{
    Lexer lexer;
    const char nao[] = {'N', (char)0xC3, (char)0x83, 'O', '\0'};

    CHECK(init_lexer(&lexer, "x contador contador10 Resultado2 NAO"));
    CHECK(expect_token(&lexer, TOK_ID, "x", 1, 1));
    CHECK(expect_token(&lexer, TOK_ID, "contador", 1, 3));
    CHECK(expect_token(&lexer, TOK_ID, "contador10", 1, 12));
    CHECK(expect_token(&lexer, TOK_ID, "Resultado2", 1, 23));
    CHECK(expect_token(&lexer, TOK_ID, "NAO", 1, 34));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 37));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, nao));
    CHECK(expect_token(&lexer, TOK_NAO, nao, 1, 1));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "NÃO ("));
    CHECK(expect_token(&lexer, TOK_NAO, nao, 1, 1));
    CHECK(expect_token(&lexer, TOK_ABRE_PAREN, "(", 1, 5));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "NÃOx"));
    CHECK(expect_invalid(&lexer, "NÃOx"));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 5));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "NÃO1"));
    CHECK(expect_invalid(&lexer, "NÃO1"));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 5));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "NÃO_"));
    CHECK(expect_invalid(&lexer, "NÃO_"));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 5));
    lexer_destroy(&lexer);
    return true;
}

static bool test_identifier_boundaries(void)
{
    Lexer lexer;

    CHECK(init_lexer(&lexer, "_nome"));
    CHECK(expect_invalid(&lexer, "_"));
    CHECK(expect_token(&lexer, TOK_ID, "nome", 1, 2));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 6));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "abc_"));
    CHECK(expect_token(&lexer, TOK_ID, "abc", 1, 1));
    CHECK(expect_invalid(&lexer, "_"));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 5));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "a__b"));
    CHECK(expect_token(&lexer, TOK_ID, "a", 1, 1));
    CHECK(expect_invalid(&lexer, "_"));
    CHECK(expect_invalid(&lexer, "_"));
    CHECK(expect_token(&lexer, TOK_ID, "b", 1, 4));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 5));
    lexer_destroy(&lexer);
    return true;
}

static bool test_operators_and_delimiters(void)
{
    static const struct {
        const char *source;
        TokenType type;
    } cases[] = {
        {"+", TOK_MAIS}, {"-", TOK_MENOS}, {"*", TOK_MULT},
        {"/", TOK_DIV}, {"//", TOK_DIV_INT}, {"^", TOK_POTENCIA},
        {"<", TOK_MENOR}, {">", TOK_MAIOR}, {"==", TOK_IGUAL_IGUAL},
        {"!", TOK_DIFERENTE}, {"[", TOK_ABRE_COLCHETE},
        {"]", TOK_FECHA_COLCHETE}, {"{", TOK_ABRE_CHAVE},
        {"}", TOK_FECHA_CHAVE}, {"(", TOK_ABRE_PAREN},
        {")", TOK_FECHA_PAREN}, {";", TOK_PONTO_VIRGULA},
        {",", TOK_VIRGULA}, {"@", TOK_ARROBA}
    };
    size_t i;
    Lexer lexer;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        CHECK(init_lexer(&lexer, cases[i].source));
        CHECK(expect_token(&lexer, cases[i].type, cases[i].source, 1, 1));
        lexer_destroy(&lexer);
    }

    CHECK(init_lexer(&lexer, "a // b / c == d"));
    CHECK(expect_token(&lexer, TOK_ID, "a", 1, 1));
    CHECK(expect_token(&lexer, TOK_DIV_INT, "//", 1, 3));
    CHECK(expect_token(&lexer, TOK_ID, "b", 1, 6));
    CHECK(expect_token(&lexer, TOK_DIV, "/", 1, 8));
    CHECK(expect_token(&lexer, TOK_ID, "c", 1, 10));
    CHECK(expect_token(&lexer, TOK_IGUAL_IGUAL, "==", 1, 12));
    CHECK(expect_token(&lexer, TOK_ID, "d", 1, 15));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "="));
    CHECK(expect_invalid(&lexer, "="));
    lexer_destroy(&lexer);
    return true;
}

static bool test_numbers(void)
{
    Lexer lexer;

    CHECK(init_lexer(&lexer, "10 0 10.5 0.25"));
    CHECK(expect_token(&lexer, TOK_NUM_INTEIRO, "10", 1, 1));
    CHECK(expect_token(&lexer, TOK_NUM_INTEIRO, "0", 1, 4));
    CHECK(expect_token(&lexer, TOK_NUM_DECIMAL, "10.5", 1, 6));
    CHECK(expect_token(&lexer, TOK_NUM_DECIMAL, "0.25", 1, 11));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "10."));
    CHECK(expect_invalid(&lexer, "10."));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, ".5"));
    CHECK(expect_invalid(&lexer, ".5"));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "10..5"));
    CHECK(expect_invalid(&lexer, "10."));
    CHECK(expect_invalid(&lexer, ".5"));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "-10"));
    CHECK(expect_token(&lexer, TOK_MENOS, "-", 1, 1));
    CHECK(expect_token(&lexer, TOK_NUM_INTEIRO, "10", 1, 2));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "0."));
    CHECK(expect_invalid(&lexer, "0."));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 3));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "1..2"));
    CHECK(expect_invalid(&lexer, "1."));
    CHECK(expect_invalid(&lexer, ".2"));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 5));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "1.2.3"));
    CHECK(expect_token(&lexer, TOK_NUM_DECIMAL, "1.2", 1, 1));
    CHECK(expect_invalid(&lexer, ".3"));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 6));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "..."));
    CHECK(expect_invalid(&lexer, "."));
    CHECK(expect_invalid(&lexer, "."));
    CHECK(expect_invalid(&lexer, "."));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 4));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "123abc"));
    CHECK(expect_token(&lexer, TOK_NUM_INTEIRO, "123", 1, 1));
    CHECK(expect_token(&lexer, TOK_ID, "abc", 1, 4));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 7));
    lexer_destroy(&lexer);
    return true;
}

static bool test_texts(void)
{
    Lexer lexer;

    CHECK(init_lexer(&lexer, "\"abc\" \"\""));
    CHECK(expect_token(&lexer, TOK_TEXTO, "\"abc\"", 1, 1));
    CHECK(expect_token(&lexer, TOK_TEXTO, "\"\"", 1, 7));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "\"abc\"resto"));
    CHECK(expect_token(&lexer, TOK_TEXTO, "\"abc\"", 1, 1));
    CHECK(expect_token(&lexer, TOK_ID, "resto", 1, 6));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "\"abc"));
    CHECK(expect_invalid(&lexer, "\"abc"));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "\"abc\\\"\""));
    CHECK(expect_token(&lexer, TOK_TEXTO, "\"abc\\\"", 1, 1));
    CHECK(expect_invalid(&lexer, "\""));
    lexer_destroy(&lexer);

    {
        const unsigned char invalid_text[] = {'"', 'a', 0xC3, '(', '"'};
        CHECK(lexer_init(&lexer, (const char *)invalid_text,
                         sizeof(invalid_text)));
        CHECK(expect_invalid_bytes(&lexer, invalid_text,
                                   sizeof(invalid_text)));
        CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 6));
        lexer_destroy(&lexer);
    }

    {
        const unsigned char nul_text[] = {'"', 'a', '\0', '"'};
        Token token = {0};
        CHECK(lexer_init(&lexer, (const char *)nul_text, sizeof(nul_text)));
        CHECK(lexer_next_token(&lexer, &token));
        CHECK(token.type == TOK_INVALIDO);
        CHECK(strcmp(token.error_message, "byte NUL invalido na entrada") == 0);
        token_destroy(&token);
        CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 5));
        lexer_destroy(&lexer);
    }

    {
        const unsigned char valid_text[] = {
            '"', 'a', 0xC3, 0xA7, 0xE2, 0x82, 0xAC,
            0xF0, 0x9F, 0x98, 0x80, '"'
        };
        Token token = {0};
        CHECK(lexer_init(&lexer, (const char *)valid_text,
                         sizeof(valid_text)));
        CHECK(lexer_next_token(&lexer, &token));
        CHECK(token.type == TOK_TEXTO);
        CHECK(token.lexeme_length == sizeof(valid_text));
        CHECK(memcmp(token.lexeme, valid_text, sizeof(valid_text)) == 0);
        CHECK(token.line == 1 && token.column == 1);
        token_destroy(&token);
        CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 7));
        lexer_destroy(&lexer);
    }

    return true;
}

static bool test_positions_and_eof(void)
{
    Lexer lexer;
    Token token = {0};

    CHECK(init_lexer(&lexer, "  inteira\tx;\r\n\tVER\rFAL\n"));
    CHECK(expect_token(&lexer, TOK_INTEIRA, "inteira", 1, 3));
    CHECK(expect_token(&lexer, TOK_ID, "x", 1, 11));
    CHECK(expect_token(&lexer, TOK_PONTO_VIRGULA, ";", 1, 12));
    CHECK(expect_token(&lexer, TOK_VER, "VER", 2, 2));
    CHECK(expect_token(&lexer, TOK_FAL, "FAL", 3, 1));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 4, 1));
    CHECK(lexer_next_token(&lexer, &token));
    CHECK(token.type == TOK_EOF);
    CHECK(token.line == 4 && token.column == 1);
    token_destroy(&token);
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "inteira x;   \t"));
    CHECK(expect_token(&lexer, TOK_INTEIRA, "inteira", 1, 1));
    CHECK(expect_token(&lexer, TOK_ID, "x", 1, 9));
    CHECK(expect_token(&lexer, TOK_PONTO_VIRGULA, ";", 1, 10));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 15));
    lexer_destroy(&lexer);
    return true;
}

static bool test_nul_and_errors(void)
{
    Lexer lexer = {0};
    Token token = {0};
    const char nul_input[] = {'a', '\0', 'b'};

    CHECK(lexer_init(&lexer, nul_input, sizeof(nul_input)));
    CHECK(expect_token(&lexer, TOK_ID, "a", 1, 1));
    {
        const unsigned char nul_lexeme[] = {0};
        CHECK(expect_invalid_bytes(&lexer, nul_lexeme, sizeof(nul_lexeme)));
    }
    CHECK(expect_token(&lexer, TOK_ID, "b", 1, 3));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 4));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "# _ nome_teste"));
    CHECK(expect_invalid(&lexer, "#"));
    CHECK(expect_invalid(&lexer, "_"));
    CHECK(expect_token(&lexer, TOK_ID, "nome", 1, 5));
    CHECK(expect_invalid(&lexer, "_"));
    CHECK(expect_token(&lexer, TOK_ID, "teste", 1, 10));
    lexer_destroy(&lexer);

    {
        const char invalid_utf8[] = {(char)0xC3, (char)0x28};
        CHECK(lexer_init(&lexer, invalid_utf8, sizeof(invalid_utf8)));
        CHECK(expect_invalid(&lexer, "\xC3"));
        CHECK(expect_token(&lexer, TOK_ABRE_PAREN, "(", 1, 2));
        lexer_destroy(&lexer);
    }

    {
        static const unsigned char malformed[][4] = {
            {0xC3, 0x00, 0x00, 0x00},
            {0xE2, 0x82, 0x00, 0x00},
            {0xF0, 0x9F, 0x98, 0x00},
            {0xC0, 0x80, 0x00, 0x00},
            {0xE0, 0x80, 0x80, 0x00},
            {0xED, 0xA0, 0x80, 0x00},
            {0xF0, 0x80, 0x80, 0x80},
            {0xF4, 0x90, 0x80, 0x80}
        };
        static const size_t lengths[] = {1, 2, 3, 2, 3, 3, 4, 4};
        size_t i;

        for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
            size_t token_count = 0;
            CHECK(lexer_init(&lexer, (const char *)malformed[i], lengths[i]));
            for (;;) {
                size_t before = lexer.current;
                CHECK(lexer_next_token(&lexer, &token));
                if (token.type == TOK_EOF) {
                    token_destroy(&token);
                    break;
                }
                CHECK(token.type == TOK_INVALIDO);
                CHECK(lexer.current > before);
                token_destroy(&token);
                token_count++;
                CHECK(token_count <= lengths[i]);
            }
            lexer_destroy(&lexer);
        }
    }

    CHECK(init_lexer(&lexer, "\"a\ninteira"));
    CHECK(expect_invalid(&lexer, "\"a"));
    CHECK(expect_token(&lexer, TOK_INTEIRA, "inteira", 2, 1));
    lexer_destroy(&lexer);

    CHECK(init_lexer(&lexer, "\"a\r\ninteira"));
    CHECK(expect_invalid(&lexer, "\"a"));
    CHECK(expect_token(&lexer, TOK_INTEIRA, "inteira", 2, 1));
    lexer_destroy(&lexer);

    {
        static const unsigned char valid_utf8[][4] = {
            {0xC2, 0xA2, 0x00, 0x00},
            {0xE2, 0x82, 0xAC, 0x00},
            {0xF0, 0x9F, 0x98, 0x80}
        };
        static const size_t lengths[] = {2, 3, 4};
        size_t i;

        for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
            CHECK(lexer_init(&lexer, (const char *)valid_utf8[i], lengths[i]));
            CHECK(expect_invalid_bytes(&lexer, valid_utf8[i], lengths[i]));
            CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 2));
            lexer_destroy(&lexer);
        }
    }

    CHECK(!lexer_next_token(NULL, &token));
    CHECK(!lexer_next_token(NULL, NULL));
    token_destroy(&token);
    return true;
}

static bool test_empty_input_and_initialization_contract(void)
{
    Lexer lexer = {0};
    Token token = {0};

    CHECK(lexer_init_from_string(&lexer, ""));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 1));
    CHECK(lexer_next_token(&lexer, &token));
    CHECK(token.type == TOK_EOF);
    CHECK(token.line == 1 && token.column == 1);
    token_destroy(&token);

    CHECK(!lexer_init_from_string(&lexer, "nao deve reinicializar"));
    lexer_destroy(&lexer);
    CHECK(lexer_init_from_string(&lexer, "x"));
    lexer_destroy(&lexer);
    return true;
}

static bool test_eof_positions_and_source_copy(void)
{
    static const struct {
        const char *source;
        size_t line;
        size_t column;
    } endings[] = {
        {"\t", 1, 2}, {"\n", 2, 1}, {"\r", 2, 1},
        {"\r\n", 2, 1}
    };
    char source[] = "x";
    Lexer lexer;
    Token token = {0};
    size_t i;

    for (i = 0; i < sizeof(endings) / sizeof(endings[0]); i++) {
        CHECK(init_lexer(&lexer, endings[i].source));
        CHECK(expect_token(&lexer, TOK_EOF, NULL,
                           endings[i].line, endings[i].column));
        CHECK(lexer_next_token(&lexer, &token));
        CHECK(token.type == TOK_EOF);
        CHECK(token.line == endings[i].line);
        CHECK(token.column == endings[i].column);
        token_destroy(&token);
        lexer_destroy(&lexer);
    }

    CHECK(lexer_init(&lexer, source, sizeof(source) - 1));
    source[0] = 'y';
    CHECK(expect_token(&lexer, TOK_ID, "x", 1, 1));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 2));
    lexer_destroy(&lexer);
    return true;
}

static bool test_official_flow(void)
{
    Lexer lexer;

    CHECK(init_lexer(&lexer, "inteira x receba 10;"));
    CHECK(expect_token(&lexer, TOK_INTEIRA, "inteira", 1, 1));
    CHECK(expect_token(&lexer, TOK_ID, "x", 1, 9));
    CHECK(expect_token(&lexer, TOK_RECEBA, "receba", 1, 11));
    CHECK(expect_token(&lexer, TOK_NUM_INTEIRO, "10", 1, 18));
    CHECK(expect_token(&lexer, TOK_PONTO_VIRGULA, ";", 1, 20));
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 1, 21));
    lexer_destroy(&lexer);
    return true;
}

static bool test_official_example(void)
{
    static const struct {
        TokenType type;
        const char *lexeme;
    } expected[] = {
        {TOK_OUTRAFUNCAO, "outrafuncao"}, {TOK_INTEIRA, "inteira"},
        {TOK_ID, "quadrado"}, {TOK_ABRE_PAREN, "("},
        {TOK_INTEIRA, "inteira"}, {TOK_ID, "n"},
        {TOK_FECHA_PAREN, ")"}, {TOK_ARROBA, "@"},
        {TOK_ABRE_COLCHETE, "["}, {TOK_RESPOST, "respost"},
        {TOK_ID, "n"}, {TOK_MULT, "*"}, {TOK_ID, "n"},
        {TOK_PONTO_VIRGULA, ";"}, {TOK_FECHA_COLCHETE, "]"},
        {TOK_INTEIRA, "inteira"}, {TOK_PRINCIPAL, "principal"},
        {TOK_ABRE_PAREN, "("}, {TOK_FECHA_PAREN, ")"},
        {TOK_ARROBA, "@"}, {TOK_ABRE_COLCHETE, "["},
        {TOK_INTEIRA, "inteira"}, {TOK_ID, "numero"},
        {TOK_PONTO_VIRGULA, ";"}, {TOK_INTEIRA, "inteira"},
        {TOK_ID, "resultado"}, {TOK_PONTO_VIRGULA, ";"},
        {TOK_ID, "numero"}, {TOK_RECEBA, "receba"},
        {TOK_NUM_INTEIRO, "5"}, {TOK_PONTO_VIRGULA, ";"},
        {TOK_ID, "resultado"}, {TOK_RECEBA, "receba"},
        {TOK_ID, "quadrado"}, {TOK_ABRE_PAREN, "("},
        {TOK_ID, "numero"}, {TOK_FECHA_PAREN, ")"},
        {TOK_PONTO_VIRGULA, ";"}, {TOK_COND, "cond"},
        {TOK_ABRE_PAREN, "("}, {TOK_ID, "resultado"},
        {TOK_MAIOR, ">"}, {TOK_NUM_INTEIRO, "20"},
        {TOK_FECHA_PAREN, ")"}, {TOK_EDAI, "edai"},
        {TOK_ABRE_COLCHETE, "["}, {TOK_RESPOST, "respost"},
        {TOK_NUM_INTEIRO, "1"}, {TOK_PONTO_VIRGULA, ";"},
        {TOK_FECHA_COLCHETE, "]"}, {TOK_CASOCONTRARIO, "casocontrario"},
        {TOK_EDAI, "edai"}, {TOK_ABRE_COLCHETE, "["},
        {TOK_RESPOST, "respost"}, {TOK_NUM_INTEIRO, "0"},
        {TOK_PONTO_VIRGULA, ";"}, {TOK_FECHA_COLCHETE, "]"},
        {TOK_FECHA_COLCHETE, "]"}
    };
    const char *source =
        "outrafuncao inteira quadrado(inteira n) @ [\n"
        "respost n * n ;\n"
        "]\n\n"
        "inteira principal() @ [\n"
        "inteira numero;\n"
        "inteira resultado;\n"
        "numero receba 5;\n"
        "resultado receba quadrado(numero);\n"
        "cond ( resultado > 20 ) edai [\n"
        "respost 1;\n"
        "]\n"
        "casocontrario edai [\n"
        "respost 0;\n"
        "]\n"
        "]";
    Lexer lexer;
    Token token = {0};
    size_t i;

    CHECK(init_lexer(&lexer, source));
    for (i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        CHECK(lexer_next_token(&lexer, &token));
        CHECK(token.type == expected[i].type);
        CHECK(token.lexeme != NULL);
        CHECK(strcmp(token.lexeme, expected[i].lexeme) == 0);
        token_destroy(&token);
    }
    CHECK(expect_token(&lexer, TOK_EOF, NULL, 16, 2));
    lexer_destroy(&lexer);
    return true;
}

static bool test_token_names_and_lifecycle(void)
{
    Token token = {0};
    Lexer lexer = {0};
    const char *names[] = {
        "TOK_ID", "TOK_NUM_INTEIRO", "TOK_NUM_DECIMAL", "TOK_TEXTO",
        "TOK_VER", "TOK_FAL", "TOK_RECEBA", "TOK_REPETE",
        "TOK_DURANTE", "TOK_COND", "TOK_CASOCONTRARIO", "TOK_FUNC",
        "TOK_BOOL", "TOK_INTEIRA", "TOK_FLUT", "TOK_PALAVRA",
        "TOK_DUPLOCARPADO", "TOK_PRINCIPAL", "TOK_VAZIO", "TOK_RESPOST",
        "TOK_REGISTRO", "TOK_OUTRAFUNCAO", "TOK_VET", "TOK_EDAI",
        "TOK_RAIZ", "TOK_LE", "TOK_IMPRIME", "TOK_OU", "TOK_E",
        "TOK_NAO", "TOK_XOR",
        "TOK_MAIS", "TOK_MENOS", "TOK_MULT", "TOK_DIV", "TOK_DIV_INT",
        "TOK_POTENCIA", "TOK_MENOR", "TOK_MAIOR", "TOK_IGUAL_IGUAL",
        "TOK_DIFERENTE", "TOK_ABRE_COLCHETE", "TOK_FECHA_COLCHETE",
        "TOK_ABRE_CHAVE", "TOK_FECHA_CHAVE", "TOK_ABRE_PAREN",
        "TOK_FECHA_PAREN", "TOK_PONTO_VIRGULA", "TOK_VIRGULA",
        "TOK_ARROBA", "TOK_PONTO", "TOK_EOF", "TOK_INVALIDO"
    };
    size_t i;

    for (i = 0; i <= (size_t)TOK_INVALIDO; i++) {
        CHECK(strcmp(token_type_name((TokenType)i), names[i]) == 0);
    }
    CHECK(strcmp(token_type_name(TOK_LE), "TOK_LE") == 0);
    CHECK(strcmp(token_type_name(TOK_IMPRIME), "TOK_IMPRIME") == 0);
    CHECK(strcmp(token_type_name((TokenType)999), "TOK_DESCONHECIDO") == 0);

    token.type = TOK_ID;
    token.lexeme = malloc(2);
    CHECK(token.lexeme != NULL);
    strcpy(token.lexeme, "x");
    token_destroy(&token);
    CHECK(token.lexeme == NULL && token.lexeme_length == 0);
    token_destroy(&token);
    lexer_destroy(&lexer);
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
    run_test("keywords", test_keywords);
    run_test("io_keyword_boundaries_and_case",
             test_io_keyword_boundaries_and_case);
    run_test("future_io_lexical_form", test_future_io_lexical_form);
    run_test("identifiers_and_nao", test_identifiers_and_nao);
    run_test("identifier_boundaries", test_identifier_boundaries);
    run_test("operators_and_delimiters", test_operators_and_delimiters);
    run_test("numbers", test_numbers);
    run_test("texts", test_texts);
    run_test("positions_and_eof", test_positions_and_eof);
    run_test("nul_and_errors", test_nul_and_errors);
    run_test("empty_input_and_initialization_contract",
             test_empty_input_and_initialization_contract);
    run_test("eof_positions_and_source_copy", test_eof_positions_and_source_copy);
    run_test("official_flow", test_official_flow);
    run_test("official_example", test_official_example);
    run_test("token_names_and_lifecycle", test_token_names_and_lifecycle);

    printf("%zu assertions, %zu failures\n", tests_run, tests_failed);
    return tests_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
