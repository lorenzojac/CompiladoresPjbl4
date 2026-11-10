/*
 * lexer.c - analisador lexico da Macaronica.
 *
 * Le o fonte byte a byte (com tamanho explicito, para que um NUL no meio
 * do arquivo vire erro em vez de truncar a entrada) e produz um Token por
 * chamada de lexer_next_token. Prioridades (docs/tokens.md):
 *   - NAO com acento e reconhecido antes de identificadores;
 *   - palavras reservadas tem prioridade sobre ID;
 *   - lexema mais longo vence: // antes de /, == antes de =.
 * Erros lexicos nao interrompem a analise: o lexer devolve TOK_INVALIDO
 * (ou TOK_ID com error_message, D37) e continua no proximo caractere.
 * Linhas e colunas contam caracteres, nao bytes (UTF-8 multibyte = 1).
 */
#include "lexer.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *lexeme;
    TokenType type;
} Keyword;

static const Keyword KEYWORDS[] = {
    {"receba", TOK_RECEBA},
    {"repete", TOK_REPETE},
    {"durante", TOK_DURANTE},
    {"cond", TOK_COND},
    {"casocontrario", TOK_CASOCONTRARIO},
    {"func", TOK_FUNC},
    {"bool", TOK_BOOL},
    {"inteira", TOK_INTEIRA},
    {"flut", TOK_FLUT},
    {"palavra", TOK_PALAVRA},
    {"duplocarpado", TOK_DUPLOCARPADO},
    {"principal", TOK_PRINCIPAL},
    {"vazio", TOK_VAZIO},
    {"respost", TOK_RESPOST},
    {"registro", TOK_REGISTRO},
    {"outrafuncao", TOK_OUTRAFUNCAO},
    {"vet", TOK_VET},
    {"edai", TOK_EDAI},
    {"raiz", TOK_RAIZ},
    {"le", TOK_LE},
    {"imprime", TOK_IMPRIME},
    {"VER", TOK_VER},
    {"FAL", TOK_FAL},
    {"OU", TOK_OU},
    {"E", TOK_E},
    {"XOR", TOK_XOR}
};

static const unsigned char NAO_UTF8[] = {'N', 0xC3, 0x83, 'O'};

static const char *const ERROR_INVALID_CHARACTER =
    "simbolo invalido ou fora do alfabeto efetivo";
static const char *const ERROR_EQUALS =
    "use ==; '=' isolado nao e token valido";
static const char *const ERROR_DECIMAL =
    "numero decimal invalido: esperado digito apos '.'";
static const char *const ERROR_DOT = "ponto nao inicia numero";
static const char *const ERROR_TEXT = "literal textual nao terminado";
static const char *const ERROR_NUL = "byte NUL invalido na entrada";
static const char *const ERROR_UTF8 = "sequencia UTF-8 invalida";
static const char *const ERROR_UNSUPPORTED_UTF8 =
    "caractere UTF-8 nao suportado fora de NAO";
static const char *const ERROR_NON_ASCII_IDENTIFIER =
    "identificador com caractere fora do alfabeto (acentos so em NAO); "
    "use apenas letras a-z, A-Z e digitos";
static const char *const ERROR_NAO_BOUNDARY =
    "NAO deve ser um token completo, nao prefixo de identificador";

static bool at_end(const Lexer *lexer)
{
    return lexer->current >= lexer->length;
}

/* Byte atual (0..255) ou -1 no fim da entrada. */
static int peek_byte(const Lexer *lexer)
{
    if (at_end(lexer)) {
        return -1;
    }

    return (unsigned char)lexer->source[lexer->current];
}

static int peek_next_byte(const Lexer *lexer)
{
    if (lexer->current >= lexer->length ||
        lexer->length - lexer->current < 2) {
        return -1;
    }

    return (unsigned char)lexer->source[lexer->current + 1];
}

static bool is_ascii_letter(int byte)
{
    return (byte >= 'A' && byte <= 'Z') ||
           (byte >= 'a' && byte <= 'z');
}

static bool is_ascii_digit(int byte)
{
    return byte >= '0' && byte <= '9';
}

static bool is_ascii_identifier_start(int byte)
{
    return is_ascii_letter(byte);
}

static bool is_ascii_identifier_part(int byte)
{
    return is_ascii_letter(byte) || is_ascii_digit(byte);
}

static bool is_ascii_word_continuation(int byte)
{
    return is_ascii_identifier_part(byte) || byte == '_';
}

static void begin_token(Lexer *lexer)
{
    lexer->token_start = lexer->current;
    lexer->token_line = lexer->line;
    lexer->token_column = lexer->column;
}

/* Consome um byte ASCII e avanca uma coluna. */
static void advance_ascii(Lexer *lexer)
{
    if (at_end(lexer)) {
        return;
    }

    lexer->current++;
    lexer->column++;
}

/*
 * Valida a sequencia UTF-8 que comeca em index (RFC 3629: rejeita
 * sequencias longas demais, surrogates e valores acima de U+10FFFF) e
 * devolve seu tamanho em bytes.
 */
static bool utf8_length_at(const Lexer *lexer, size_t index, size_t *length)
{
    unsigned char first;
    size_t available;
    unsigned char second;

    if (index >= lexer->length) {
        return false;
    }

    first = (unsigned char)lexer->source[index];
    available = lexer->length - index;

    if (first < 0x80) {
        *length = 1;
        return true;
    }

    if (first >= 0xC2 && first <= 0xDF) {
        if (available < 2) {
            return false;
        }

        second = (unsigned char)lexer->source[index + 1];
        if (second < 0x80 || second > 0xBF) {
            return false;
        }

        *length = 2;
        return true;
    }

    if (first >= 0xE0 && first <= 0xEF) {
        if (available < 3) {
            return false;
        }

        second = (unsigned char)lexer->source[index + 1];
        if ((first == 0xE0 && (second < 0xA0 || second > 0xBF)) ||
            (first == 0xED && (second < 0x80 || second > 0x9F)) ||
            (first != 0xE0 && first != 0xED &&
             (second < 0x80 || second > 0xBF)) ||
            (unsigned char)lexer->source[index + 2] < 0x80 ||
            (unsigned char)lexer->source[index + 2] > 0xBF) {
            return false;
        }

        *length = 3;
        return true;
    }

    if (first >= 0xF0 && first <= 0xF4) {
        if (available < 4) {
            return false;
        }

        second = (unsigned char)lexer->source[index + 1];
        if ((first == 0xF0 && (second < 0x90 || second > 0xBF)) ||
            (first == 0xF4 && (second < 0x80 || second > 0x8F)) ||
            (first != 0xF0 && first != 0xF4 &&
             (second < 0x80 || second > 0xBF)) ||
            (unsigned char)lexer->source[index + 2] < 0x80 ||
            (unsigned char)lexer->source[index + 2] > 0xBF ||
            (unsigned char)lexer->source[index + 3] < 0x80 ||
            (unsigned char)lexer->source[index + 3] > 0xBF) {
            return false;
        }

        *length = 4;
        return true;
    }

    return false;
}

/* Consome um caractere UTF-8 inteiro; conta como uma unica coluna. */
static bool advance_utf8(Lexer *lexer)
{
    size_t length;

    if (!utf8_length_at(lexer, lexer->current, &length)) {
        return false;
    }

    lexer->current += length;
    lexer->column++;
    return true;
}

/* Trata \n, \r e \r\n como uma quebra de linha. */
static void consume_newline(Lexer *lexer)
{
    int byte = peek_byte(lexer);

    if (byte == '\r') {
        lexer->current++;
        if (!at_end(lexer) && peek_byte(lexer) == '\n') {
            lexer->current++;
        }
    } else if (byte == '\n') {
        lexer->current++;
    } else {
        return;
    }

    lexer->line++;
    lexer->column = 1;
}

/* Espacos, tabulacoes e quebras de linha sao ignorados (D04). */
static void skip_whitespace(Lexer *lexer)
{
    while (!at_end(lexer)) {
        int byte = peek_byte(lexer);

        if (byte == ' ' || byte == '\t') {
            advance_ascii(lexer);
        } else if (byte == '\n' || byte == '\r') {
            consume_newline(lexer);
        } else {
            break;
        }
    }
}

static void fail_internal(Lexer *lexer)
{
    lexer->failed = true;
}

/* Cria um token com o lexema [token_start, current) copiado. */
static bool make_token(
    Lexer *lexer,
    Token *token,
    TokenType type,
    const char *error_message
)
{
    size_t length = lexer->current - lexer->token_start;
    char *lexeme;

    if (length == SIZE_MAX) {
        fail_internal(lexer);
        return false;
    }

    lexeme = malloc(length + 1);
    if (lexeme == NULL) {
        fail_internal(lexer);
        return false;
    }

    if (length > 0) {
        memcpy(lexeme, lexer->source + lexer->token_start, length);
    }
    lexeme[length] = '\0';

    token->type = type;
    token->lexeme = lexeme;
    token->lexeme_length = length;
    token->line = lexer->token_line;
    token->column = lexer->token_column;
    token->error_message = error_message;
    return true;
}

static void make_eof(const Lexer *lexer, Token *token)
{
    token->type = TOK_EOF;
    token->line = lexer->line;
    token->column = lexer->column;
}

/* Palavra reservada correspondente ao lexema, ou TOK_ID. */
static TokenType lookup_keyword(const char *lexeme, size_t length)
{
    size_t count = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);
    size_t index;

    for (index = 0; index < count; index++) {
        if (strlen(KEYWORDS[index].lexeme) == length &&
            memcmp(KEYWORDS[index].lexeme, lexeme, length) == 0) {
            return KEYWORDS[index].type;
        }
    }

    return TOK_ID;
}

/* true se ha um caractere UTF-8 multibyte valido na posicao atual. */
static bool at_valid_multibyte(const Lexer *lexer)
{
    size_t length;

    return peek_byte(lexer) >= 0x80 &&
           utf8_length_at(lexer, lexer->current, &length) &&
           length > 1;
}

/*
 * ID -> LETRA (LETRA | DIGITO)*, com prioridade para palavras reservadas.
 *
 * Recuperacao (D37): se a palavra contem caracteres UTF-8 fora do alfabeto
 * (por exemplo "funcao" escrito com cedilha e til), ela inteira vira UM
 * token TOK_ID com error_message preenchido. O parser registra um unico
 * erro lexico e continua a analise normalmente, sem a cascata de um erro
 * por caractere acentuado seguido de erros sintaticos.
 */
static bool scan_word(Lexer *lexer, Token *token, bool has_non_ascii)
{
    for (;;) {
        if (is_ascii_identifier_part(peek_byte(lexer))) {
            advance_ascii(lexer);
        } else if (at_valid_multibyte(lexer)) {
            advance_utf8(lexer);
            has_non_ascii = true;
        } else {
            break;
        }
    }

    if (has_non_ascii) {
        return make_token(
            lexer,
            token,
            TOK_ID,
            ERROR_NON_ASCII_IDENTIFIER
        );
    }
    return make_token(
        lexer,
        token,
        lookup_keyword(
            lexer->source + lexer->token_start,
            lexer->current - lexer->token_start
        ),
        NULL
    );
}

static bool scan_identifier_or_keyword(Lexer *lexer, Token *token)
{
    return scan_word(lexer, token, false);
}

/* Retorna 1 se produziu token, 0 se nao era a sequencia especial e -1 em falha. */
/*
 * NAO acentuado ('N', 0xC3 0x83, 'O'). Se continuar com letra/digito
 * (NAOx), e erro de fronteira: NAO nao pode prefixar identificador.
 */
static int try_scan_nao(Lexer *lexer, Token *token)
{
    size_t index = lexer->current;
    size_t nao_length = sizeof(NAO_UTF8);
    size_t remaining;
    int next;

    if (peek_byte(lexer) != 'N') {
        return 0;
    }

    remaining = lexer->length - index;
    if (remaining < nao_length ||
        memcmp(lexer->source + index, NAO_UTF8, nao_length) != 0) {
        return 0;
    }

    next = index + nao_length < lexer->length
               ? (unsigned char)lexer->source[index + nao_length]
               : -1;
    begin_token(lexer);
    advance_ascii(lexer);
    if (!advance_utf8(lexer)) {
        fail_internal(lexer);
        return -1;
    }
    advance_ascii(lexer);

    if (is_ascii_word_continuation(next)) {
        while (is_ascii_word_continuation(peek_byte(lexer))) {
            advance_ascii(lexer);
        }

        if (!make_token(lexer, token, TOK_INVALIDO, ERROR_NAO_BOUNDARY)) {
            return -1;
        }
        return 1;
    }

    if (!make_token(lexer, token, TOK_NAO, NULL)) {
        return -1;
    }
    return 1;
}

/* NUM -> DIGITO+ ('.' DIGITO+)?  ('1.' e erro: falta a parte decimal). */
static bool scan_number(Lexer *lexer, Token *token)
{
    bool has_fraction = false;

    while (is_ascii_digit(peek_byte(lexer))) {
        advance_ascii(lexer);
    }

    if (peek_byte(lexer) == '.') {
        advance_ascii(lexer);
        if (!is_ascii_digit(peek_byte(lexer))) {
            return make_token(lexer, token, TOK_INVALIDO, ERROR_DECIMAL);
        }

        has_fraction = true;
        while (is_ascii_digit(peek_byte(lexer))) {
            advance_ascii(lexer);
        }
    }

    return make_token(
        lexer,
        token,
        has_fraction ? TOK_NUM_DECIMAL : TOK_NUM_INTEIRO,
        NULL
    );
}

/*
 * '.' seguido de letra e o operador de acesso a campo de registro (D39,
 * "p.x"). Qualquer outro '.' fora de numero continua sendo erro (".5").
 */
static bool scan_dot_invalid(Lexer *lexer, Token *token)
{
    advance_ascii(lexer);
    if (is_ascii_letter(peek_byte(lexer))) {
        return make_token(lexer, token, TOK_PONTO, NULL);
    }
    while (is_ascii_digit(peek_byte(lexer))) {
        advance_ascii(lexer);
    }

    return make_token(lexer, token, TOK_INVALIDO, ERROR_DOT);
}

/*
 * TEXTO -> '"' qualquer caractere exceto '"' e quebra de linha '"'.
 * UTF-8 valido e aceito dentro de textos.
 */
static bool scan_text(Lexer *lexer, Token *token)
{
    bool invalid_content = false;
    const char *error_message = ERROR_TEXT;

    advance_ascii(lexer);

    while (!at_end(lexer)) {
        int byte = peek_byte(lexer);

        if (byte == '"') {
            advance_ascii(lexer);
            return make_token(
                lexer,
                token,
                invalid_content ? TOK_INVALIDO : TOK_TEXTO,
                invalid_content ? error_message : NULL
            );
        }

        if (byte == '\n' || byte == '\r') {
            return make_token(lexer, token, TOK_INVALIDO, error_message);
        }

        if (byte == '\0') {
            invalid_content = true;
            error_message = ERROR_NUL;
            advance_ascii(lexer);
            continue;
        }

        if ((unsigned char)byte >= 0x80) {
            if (!advance_utf8(lexer)) {
                invalid_content = true;
                error_message = ERROR_UTF8;
                advance_ascii(lexer);
            }
            continue;
        }

        advance_ascii(lexer);
    }

    return make_token(
        lexer,
        token,
        TOK_INVALIDO,
        error_message
    );
}

/*
 * Caractere fora do alfabeto. Um caractere UTF-8 multibyte valido gera um
 * unico token (nunca um erro por byte); se ele inicia uma palavra
 * ("exito" com acento no e), a palavra inteira vira um identificador com
 * erro (D37). Bytes que nao formam UTF-8 valido sao reportados um a um,
 * pois nao ha como saber onde termina o "caractere".
 */
static bool scan_non_ascii_invalid(Lexer *lexer, Token *token)
{
    if ((unsigned char)peek_byte(lexer) >= 0x80) {
        if (!advance_utf8(lexer)) {
            advance_ascii(lexer);
            return make_token(lexer, token, TOK_INVALIDO, ERROR_UTF8);
        }
        if (is_ascii_identifier_part(peek_byte(lexer)) ||
            at_valid_multibyte(lexer)) {
            return scan_word(lexer, token, true);
        }

        return make_token(lexer, token, TOK_INVALIDO, ERROR_UNSUPPORTED_UTF8);
    }

    advance_ascii(lexer);
    return make_token(lexer, token, TOK_INVALIDO, ERROR_INVALID_CHARACTER);
}

/* Operadores e delimitadores; '=' isolado tem mensagem propria (use ==). */
static bool scan_operator_or_symbol(Lexer *lexer, Token *token)
{
    int byte = peek_byte(lexer);
    TokenType type;

    switch (byte) {
    case '+':
        advance_ascii(lexer);
        type = TOK_MAIS;
        break;
    case '-':
        advance_ascii(lexer);
        type = TOK_MENOS;
        break;
    case '*':
        advance_ascii(lexer);
        type = TOK_MULT;
        break;
    case '^':
        advance_ascii(lexer);
        type = TOK_POTENCIA;
        break;
    case '<':
        advance_ascii(lexer);
        type = TOK_MENOR;
        break;
    case '>':
        advance_ascii(lexer);
        type = TOK_MAIOR;
        break;
    case '!':
        advance_ascii(lexer);
        type = TOK_DIFERENTE;
        break;
    case '/':
        if (peek_next_byte(lexer) == '/') {
            advance_ascii(lexer);
            advance_ascii(lexer);
            type = TOK_DIV_INT;
        } else {
            advance_ascii(lexer);
            type = TOK_DIV;
        }
        break;
    case '=':
        if (peek_next_byte(lexer) == '=') {
            advance_ascii(lexer);
            advance_ascii(lexer);
            type = TOK_IGUAL_IGUAL;
        } else {
            advance_ascii(lexer);
            return make_token(lexer, token, TOK_INVALIDO, ERROR_EQUALS);
        }
        break;
    case '[':
        advance_ascii(lexer);
        type = TOK_ABRE_COLCHETE;
        break;
    case ']':
        advance_ascii(lexer);
        type = TOK_FECHA_COLCHETE;
        break;
    case '{':
        advance_ascii(lexer);
        type = TOK_ABRE_CHAVE;
        break;
    case '}':
        advance_ascii(lexer);
        type = TOK_FECHA_CHAVE;
        break;
    case '(':
        advance_ascii(lexer);
        type = TOK_ABRE_PAREN;
        break;
    case ')':
        advance_ascii(lexer);
        type = TOK_FECHA_PAREN;
        break;
    case ';':
        advance_ascii(lexer);
        type = TOK_PONTO_VIRGULA;
        break;
    case ',':
        advance_ascii(lexer);
        type = TOK_VIRGULA;
        break;
    case '@':
        advance_ascii(lexer);
        type = TOK_ARROBA;
        break;
    default:
        return scan_non_ascii_invalid(lexer, token);
    }

    return make_token(lexer, token, type, NULL);
}

bool lexer_init(Lexer *lexer, const char *source, size_t length)
{
    char *copy;

    if (lexer == NULL || source == NULL || lexer->source != NULL ||
        length == SIZE_MAX) {
        return false;
    }

    copy = malloc(length + 1);
    if (copy == NULL) {
        return false;
    }

    if (length > 0) {
        memcpy(copy, source, length);
    }
    copy[length] = '\0';

    *lexer = (Lexer){0};
    lexer->source = copy;
    lexer->length = length;
    lexer->line = 1;
    lexer->column = 1;
    return true;
}

bool lexer_init_from_string(Lexer *lexer, const char *source)
{
    if (source == NULL) {
        return false;
    }

    return lexer_init(lexer, source, strlen(source));
}

void lexer_destroy(Lexer *lexer)
{
    if (lexer == NULL) {
        return;
    }

    free(lexer->source);
    *lexer = (Lexer){0};
}

/*
 * Despacha pelo primeiro caractere do proximo token. Depois de EOF,
 * continua devolvendo TOK_EOF (o parser pode pedir mais de uma vez).
 */
bool lexer_next_token(Lexer *lexer, Token *out_token)
{
    int special_nao;

    if (lexer == NULL || out_token == NULL || lexer->source == NULL) {
        return false;
    }

    *out_token = (Token){0};

    if (lexer->failed) {
        return false;
    }

    if (lexer->finished) {
        make_eof(lexer, out_token);
        return true;
    }

    if (at_end(lexer)) {
        lexer->finished = true;
        make_eof(lexer, out_token);
        return true;
    }

    skip_whitespace(lexer);

    /* O espaco final tambem pode levar current ate length. */
    if (at_end(lexer)) {
        lexer->finished = true;
        make_eof(lexer, out_token);
        return true;
    }

    begin_token(lexer);

    special_nao = try_scan_nao(lexer, out_token);
    if (special_nao != 0) {
        return special_nao > 0;
    }

    if (is_ascii_identifier_start(peek_byte(lexer))) {
        return scan_identifier_or_keyword(lexer, out_token);
    }

    if (is_ascii_digit(peek_byte(lexer))) {
        return scan_number(lexer, out_token);
    }

    if (peek_byte(lexer) == '.') {
        return scan_dot_invalid(lexer, out_token);
    }

    if (peek_byte(lexer) == '"') {
        return scan_text(lexer, out_token);
    }

    return scan_operator_or_symbol(lexer, out_token);
}
