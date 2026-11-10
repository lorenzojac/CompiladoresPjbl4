/*
 * main.c - ponto de entrada do compilador.
 *
 *   macaronica entrada.mac                    valida e imprime o relatorio
 *   macaronica entrada.mac -o saida.ll        gera LLVM IR (e saida.mlir se
 *                                             houver DSL vetorial)
 *   opcoes: --alvo=pc|pico|uno  --mlir=arquivo  -q (silencioso se valido)
 *
 * Le o arquivo inteiro para a memoria, executa parser (que conduz o
 * lexer), analise semantica e, com -o, o gerador de codigo. Um programa
 * invalido nunca produz arquivo de saida (arquivos antigos sao removidos).
 * Codigo de saida: 0 = programa valido, 1 = programa invalido,
 * 2 = erro de uso, de leitura/escrita ou falha interna.
 */
#include "codegen_llvm.h"
#include "parser.h"
#include "semantic.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define READ_CHUNK_SIZE ((size_t)4096)

static bool grow_source_buffer(
    char **buffer,
    size_t *capacity
)
{
    size_t new_capacity;
    char *grown;

    if (buffer == NULL || capacity == NULL) {
        return false;
    }

    if (*capacity == 0) {
        new_capacity = READ_CHUNK_SIZE;
    } else {
        if (*capacity > SIZE_MAX / 2) {
            return false;
        }

        new_capacity = *capacity * 2;
    }

    grown = realloc(*buffer, new_capacity);
    if (grown == NULL) {
        return false;
    }

    *buffer = grown;
    *capacity = new_capacity;
    return true;
}

/*
 * Le o arquivo em modo binario, crescendo o buffer em potencias de 2.
 * O tamanho real e devolvido separadamente: bytes NUL no meio do arquivo
 * sao preservados e reportados pelo lexer.
 */
static bool read_file(
    const char *path,
    char **out_buffer,
    size_t *out_length
)
{
    FILE *file;
    char *buffer = NULL;
    size_t length = 0;
    size_t capacity = 0;
    bool success = false;

    if (path == NULL || out_buffer == NULL || out_length == NULL) {
        return false;
    }

    *out_buffer = NULL;
    *out_length = 0;

    file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }

    while (true) {
        size_t available;
        size_t amount;

        if (length == capacity && !grow_source_buffer(&buffer, &capacity)) {
            goto cleanup;
        }

        available = capacity - length;
        amount = fread(buffer + length, 1, available, file);

        if (amount > 0) {
            if (length > SIZE_MAX - amount) {
                goto cleanup;
            }

            length += amount;
        }

        if (amount < available) {
            if (ferror(file)) {
                goto cleanup;
            }

            if (feof(file)) {
                break;
            }

            goto cleanup;
        }
    }

    if (length == SIZE_MAX) {
        goto cleanup;
    }

    if (length == capacity && !grow_source_buffer(&buffer, &capacity)) {
        goto cleanup;
    }

    buffer[length] = '\0';
    *out_buffer = buffer;
    *out_length = length;
    buffer = NULL;
    success = true;

cleanup:
    if (fclose(file) != 0) {
        success = false;
    }

    free(buffer);
    if (!success) {
        free(*out_buffer);
        *out_buffer = NULL;
        *out_length = 0;
    }

    return success;
}

/* Imprime um lexema escapando controles e bytes nao-ASCII (\xNN). */
static void print_escaped_bytes(
    FILE *out,
    const char *data,
    size_t length
)
{
    size_t index;

    if (data == NULL) {
        fputs("<nulo>", out);
        return;
    }

    if (length == 0) {
        fputs("<vazio>", out);
        return;
    }

    for (index = 0; index < length; index++) {
        unsigned char byte = (unsigned char)data[index];

        switch (byte) {
        case '\\':
            fputs("\\\\", out);
            break;
        case '\n':
            fputs("\\n", out);
            break;
        case '\r':
            fputs("\\r", out);
            break;
        case '\t':
            fputs("\\t", out);
            break;
        default:
            if (byte >= 0x20 && byte <= 0x7E) {
                fputc((int)byte, out);
            } else {
                fprintf(out, "\\x%02X", (unsigned int)byte);
            }
            break;
        }
    }
}

static const char *diagnostic_kind_name(DiagnosticKind kind)
{
    switch (kind) {
    case DIAG_LEXICAL:
        return "LEXICO";
    case DIAG_SYNTAX:
        return "SINTATICO";
    case DIAG_INTERNAL:
        return "INTERNO";
    default:
        return "DESCONHECIDO";
    }
}

static const char *semantic_code_name(SemanticDiagnosticCode code)
{
    switch (code) {
    case SEM_UNDECLARED_IDENTIFIER:
        return "SEM_UNDECLARED_IDENTIFIER";
    case SEM_DUPLICATE_DECLARATION:
        return "SEM_DUPLICATE_DECLARATION";
    case SEM_NAME_KIND_MISMATCH:
        return "SEM_NAME_KIND_MISMATCH";
    case SEM_TYPE_MISMATCH:
        return "SEM_TYPE_MISMATCH";
    case SEM_INVALID_ASSIGNMENT:
        return "SEM_INVALID_ASSIGNMENT";
    case SEM_NOT_A_VECTOR:
        return "SEM_NOT_A_VECTOR";
    case SEM_INVALID_VECTOR_INDEX:
        return "SEM_INVALID_VECTOR_INDEX";
    case SEM_INVALID_VECTOR_SIZE:
        return "SEM_INVALID_VECTOR_SIZE";
    case SEM_UNKNOWN_FUNCTION:
        return "SEM_UNKNOWN_FUNCTION";
    case SEM_NOT_A_FUNCTION:
        return "SEM_NOT_A_FUNCTION";
    case SEM_ARGUMENT_COUNT_MISMATCH:
        return "SEM_ARGUMENT_COUNT_MISMATCH";
    case SEM_ARGUMENT_TYPE_MISMATCH:
        return "SEM_ARGUMENT_TYPE_MISMATCH";
    case SEM_VOID_VALUE:
        return "SEM_VOID_VALUE";
    case SEM_INVALID_RETURN:
        return "SEM_INVALID_RETURN";
    case SEM_INVALID_CONDITION:
        return "SEM_INVALID_CONDITION";
    case SEM_INVALID_OPERATOR:
        return "SEM_INVALID_OPERATOR";
    case SEM_INVALID_RECORD_DECLARATION:
        return "SEM_INVALID_RECORD_DECLARATION";
    case SEM_INVALID_PRINCIPAL:
        return "SEM_INVALID_PRINCIPAL";
    case SEM_MISSING_RETURN:
        return "SEM_MISSING_RETURN";
    case SEM_INDEX_OUT_OF_BOUNDS:
        return "SEM_INDEX_OUT_OF_BOUNDS";
    case SEM_DIVISION_BY_ZERO:
        return "SEM_DIVISION_BY_ZERO";
    case SEM_LITERAL_OUT_OF_RANGE:
        return "SEM_LITERAL_OUT_OF_RANGE";
    case SEM_INVALID_FIELD:
        return "SEM_INVALID_FIELD";
    case SEM_VECTOR_SIZE_MISMATCH:
        return "SEM_VECTOR_SIZE_MISMATCH";
    case SEM_INVALID_VECTOR_OPERATION:
        return "SEM_INVALID_VECTOR_OPERATION";
    case SEM_TARGET_MEMORY:
        return "SEM_TARGET_MEMORY";
    case SEM_TARGET_UNSUPPORTED:
        return "SEM_TARGET_UNSUPPORTED";
    case SEM_INTERNAL:
        return "SEM_INTERNAL";
    default:
        return "SEM_UNKNOWN";
    }
}

static void print_parser_diagnostic(const Diagnostic *diagnostic, size_t index)
{
    if (diagnostic == NULL) {
        return;
    }

    printf(
        "  %zu. [%s] linha %zu, coluna %zu\n",
        index,
        diagnostic_kind_name(diagnostic->kind),
        diagnostic->location.line,
        diagnostic->location.column
    );
    printf("     encontrado: %s\n", token_type_name(diagnostic->found_type));
    printf("     lexema: ");
    print_escaped_bytes(
        stdout,
        diagnostic->found_lexeme,
        diagnostic->found_lexeme_length
    );
    putchar('\n');

    if (diagnostic->expected != NULL) {
        printf("     esperado: %s\n", diagnostic->expected);
    }

    if (diagnostic->message != NULL) {
        printf("     mensagem: %s\n", diagnostic->message);
    }
}

static void print_parser_diagnostics(const ParseResult *result)
{
    size_t index;

    if (result == NULL || result->diagnostic_count == 0) {
        return;
    }

    puts("[DIAGNOSTICOS DO PARSER]");
    for (index = 0; index < result->diagnostic_count; index++) {
        print_parser_diagnostic(&result->diagnostics[index], index + 1);
    }
}

/*
 * Status da fase sintatica:
 *   OK      - nenhum erro e nenhum token descartado;
 *   ERRO    - ao menos um erro sintatico relatado;
 *   PARCIAL - sem erro sintatico proprio, mas o parser trabalhou sobre uma
 *             entrada incompleta (tokens invalidos descartados pelo lexico),
 *             entao "OK" seria enganoso.
 */
static void print_parser_report(const ParseResult *result)
{
    const char *lexical_status;
    const char *syntax_status;

    lexical_status = result->lexical_error_count == 0 ? "OK" : "ERRO";
    if (result->syntax_error_count > 0) {
        syntax_status = "ERRO";
    } else if (result->lexical_error_count > 0) {
        syntax_status = "PARCIAL";
    } else {
        syntax_status = "OK";
    }

    puts("[LEXICO]");
    printf("Status: %s\n", lexical_status);
    printf("Erros: %zu\n\n", result->lexical_error_count);

    puts("[SINTATICO]");
    printf("Status: %s\n", syntax_status);
    printf("Erros: %zu\n", result->syntax_error_count);
    if (result->lexical_error_count > 0) {
        puts("Observacao: a analise sintatica ignorou os tokens invalidos "
             "acima; o resultado vale para a entrada sem eles.");
    }
    if (result->suppressed_syntax_error_count > 0) {
        printf(
            "Erros em cascata suprimidos: %zu (consequencia de erro ja "
            "relatado no mesmo comando)\n",
            result->suppressed_syntax_error_count
        );
    }

    if (result->status == PARSE_INTERNAL_FAILURE) {
        puts("Status geral do parser: FALHA INTERNA");
    }

    putchar('\n');
    print_parser_diagnostics(result);
}

static void print_semantic_diagnostic(
    const SemanticDiagnostic *diagnostic,
    size_t index
)
{
    if (diagnostic == NULL) {
        return;
    }

    printf(
        "  %zu. linha %zu, coluna %zu\n",
        index,
        diagnostic->location.line,
        diagnostic->location.column
    );
    printf("     codigo: %s\n", semantic_code_name(diagnostic->code));

    if (diagnostic->symbol_name != NULL) {
        fputs("     simbolo: ", stdout);
        print_escaped_bytes(
            stdout,
            diagnostic->symbol_name,
            diagnostic->symbol_name_length
        );
        putchar('\n');
    }

    if (diagnostic->message != NULL) {
        printf("     mensagem: %s\n", diagnostic->message);
    }
}

static void print_semantic_report(const SemanticResult *result)
{
    size_t index;

    puts("[SEMANTICO]");

    switch (result->status) {
    case SEMANTIC_OK:
        puts("Status: OK");
        puts("Erros: 0");
        return;
    case SEMANTIC_ERRORS:
        puts("Status: ERRO");
        printf("Erros: %zu\n", result->diagnostic_count);
        break;
    case SEMANTIC_INTERNAL_FAILURE:
        puts("Status: FALHA INTERNA");
        printf("Diagnosticos preservados: %zu\n", result->diagnostic_count);
        break;
    default:
        puts("Status: FALHA INTERNA");
        printf("Diagnosticos preservados: %zu\n", result->diagnostic_count);
        break;
    }

    if (result->diagnostic_count > 0) {
        puts("[DIAGNOSTICOS SEMANTICOS]");
        for (index = 0; index < result->diagnostic_count; index++) {
            print_semantic_diagnostic(&result->diagnostics[index], index + 1);
        }
    }
}

static void print_report_header(const char *path)
{
    puts("========================================");
    puts("Macaronica - Relatorio de validacao");
    printf("Arquivo: %s\n", path);
    puts("========================================");
    putchar('\n');
}

typedef struct {
    const char *input;
    const char *output;
    const char *mlir_output;
    SemanticTarget target;
    bool quiet;
    bool fastmath;
} Options;

static void print_usage(void)
{
    fputs(
        "Uso: macaronica <entrada.mac> [-o saida.ll] [--mlir=nucleos.mlir]\n"
        "                [--alvo=pc|pico|uno] [-q]\n"
        "  sem -o      apenas valida e imprime o relatorio das fases\n"
        "  -o          gera LLVM IR; com DSL vetorial gera tambem os nucleos\n"
        "              MLIR (padrao: mesmo nome com extensao .mlir)\n"
        "  --alvo=pico verificacoes da Raspberry Pi Pico 2 W (RAM de 512 KB)\n"
        "  --alvo=uno  verificacoes do Arduino Uno (RAM de 2 KB)\n"
        "  -q         nao imprime o relatorio quando o programa e valido\n"
        "  --fastmath nucleos da DSL podem reassociar operacoes reais\n",
        stderr
    );
}

static bool parse_options(int argc, char **argv, Options *options)
{
    int i;

    *options = (Options){0};
    options->target = SEMANTIC_TARGET_PC;
    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "-o") == 0 && i + 1 < argc) {
            options->output = argv[++i];
        } else if (strncmp(arg, "--mlir=", 7) == 0) {
            options->mlir_output = arg + 7;
        } else if (strcmp(arg, "--alvo=pc") == 0) {
            options->target = SEMANTIC_TARGET_PC;
        } else if (strcmp(arg, "--alvo=uno") == 0) {
            options->target = SEMANTIC_TARGET_UNO;
        } else if (strcmp(arg, "--alvo=pico") == 0) {
            options->target = SEMANTIC_TARGET_PICO;
        } else if (strcmp(arg, "-q") == 0) {
            options->quiet = true;
        } else if (strcmp(arg, "--fastmath") == 0) {
            options->fastmath = true;
        } else if (arg[0] == '-' || options->input != NULL) {
            return false;
        } else {
            options->input = arg;
        }
    }
    return options->input != NULL;
}

/* saida.ll -> saida.mlir (ou acrescenta .mlir). Devolve memoria alocada. */
static char *default_mlir_path(const char *output)
{
    size_t length = strlen(output);
    char *path = malloc(length + 6);

    if (path == NULL) {
        return NULL;
    }
    memcpy(path, output, length + 1);
    if (length > 3 && strcmp(output + length - 3, ".ll") == 0) {
        path[length - 3] = '\0';
    }
    strcat(path, ".mlir");
    return path;
}

static bool write_text_file(const char *path, const StrBuf *text)
{
    FILE *file = fopen(path, "wb");
    bool ok;

    if (file == NULL) {
        return false;
    }
    ok = fwrite(strbuf_text(text), 1, text->length, file) == text->length;
    if (fclose(file) != 0) {
        ok = false;
    }
    return ok;
}

static void print_target_memory(
    SemanticTarget target,
    const SemanticResult *result
)
{
    const SemanticTargetInfo info = semantic_target_info(target);

    printf(
        "[ALVO %s]\n"
        "Memoria estimada: globais e textos %zu bytes; maior quadro de "
        "funcao %zu bytes; limite %zu bytes (%zu de RAM - %zu reservados)\n",
        target == SEMANTIC_TARGET_PICO ? "PICO 2 W" : "UNO",
        result->global_bytes,
        result->max_frame_bytes,
        info.ram_bytes - info.reserved_bytes,
        info.ram_bytes,
        info.reserved_bytes
    );
}

int main(int argc, char **argv)
{
    char *source = NULL;
    size_t source_length = 0;
    Parser parser = {0};
    ParseResult parse_result = {0};
    SemanticAnalyzer analyzer = {0};
    SemanticResult semantic_result = {0};
    CodegenResult codegen = {0};
    Options options;
    char *mlir_path = NULL;
    bool program_valid = false;
    int exit_code = 2;

    if (!parse_options(argc, argv, &options)) {
        print_usage();
        return 2;
    }
    if (options.output != NULL) {
        mlir_path = options.mlir_output != NULL
                        ? malloc(strlen(options.mlir_output) + 1)
                        : default_mlir_path(options.output);
        if (mlir_path == NULL) {
            fprintf(stderr, "Erro interno: sem memoria.\n");
            return 2;
        }
        if (options.mlir_output != NULL) {
            strcpy(mlir_path, options.mlir_output);
        }
        /* Nenhum arquivo de uma compilacao anterior pode sobrar. */
        remove(options.output);
        remove(mlir_path);
    }

    if (!read_file(options.input, &source, &source_length)) {
        fprintf(stderr, "Erro: nao foi possivel ler o arquivo '%s'.\n",
                options.input);
        goto cleanup;
    }

    if (!parser_init(&parser, source, source_length)) {
        fprintf(stderr, "Erro interno: falha ao inicializar o parser.\n");
        goto cleanup;
    }

    free(source);
    source = NULL;

    parse_result = parser_parse(&parser);
    parser_destroy(&parser);

    if (parse_result.status == PARSE_OK && parse_result.ast != NULL) {
        if (!semantic_init(&analyzer, parse_result.ast)) {
            fprintf(stderr, "Erro interno: falha ao inicializar a semantica.\n");
            goto cleanup;
        }
        semantic_set_target(&analyzer, options.target);
        semantic_result = semantic_analyze(&analyzer);
        program_valid = semantic_result.status == SEMANTIC_OK;
    }

    if (!(program_valid && options.quiet)) {
        print_report_header(options.input);
        print_parser_report(&parse_result);
    }

    if (parse_result.status == PARSE_INTERNAL_FAILURE) {
        puts("A analise nao pode ser concluida por falha interna.");
        puts("RESULTADO FINAL: FALHA INTERNA");
        goto cleanup;
    }

    if (parse_result.status == PARSE_INPUT_ERRORS) {
        puts("[SEMANTICO]");
        puts("Status: NAO EXECUTADO");
        puts("Analise semantica nao executada devido a erros anteriores.");
        puts("RESULTADO FINAL: PROGRAMA INVALIDO");
        exit_code = 1;
        goto cleanup;
    }

    if (parse_result.status != PARSE_OK || parse_result.ast == NULL) {
        puts("A analise nao pode ser concluida por falha interna.");
        puts("RESULTADO FINAL: FALHA INTERNA");
        goto cleanup;
    }

    if (!(program_valid && options.quiet)) {
        print_semantic_report(&semantic_result);
        if (options.target != SEMANTIC_TARGET_PC &&
            semantic_result.status != SEMANTIC_INTERNAL_FAILURE) {
            print_target_memory(options.target, &semantic_result);
        }
    }

    switch (semantic_result.status) {
    case SEMANTIC_OK:
        exit_code = 0;
        break;
    case SEMANTIC_ERRORS:
        puts("RESULTADO FINAL: PROGRAMA INVALIDO");
        exit_code = 1;
        goto cleanup;
    default:
        puts("RESULTADO FINAL: FALHA INTERNA");
        exit_code = 2;
        goto cleanup;
    }

    if (options.output == NULL) {
        puts("RESULTADO FINAL: PROGRAMA VALIDO");
        goto cleanup;
    }

    if (!codegen_llvm_generate(parse_result.ast, semantic_result.symbol_count,
                               options.input, options.fastmath, &codegen)) {
        fprintf(stderr, "Erro interno: falha na geracao de codigo.\n");
        exit_code = 2;
        goto cleanup;
    }
    if (!write_text_file(options.output, &codegen.ir) ||
        (codegen.kernel_count > 0 &&
         !write_text_file(mlir_path, &codegen.mlir))) {
        fprintf(stderr, "Erro: nao foi possivel escrever a saida.\n");
        remove(options.output);
        remove(mlir_path);
        exit_code = 2;
        goto cleanup;
    }
    if (!options.quiet) {
        puts("RESULTADO FINAL: PROGRAMA VALIDO");
        printf("LLVM IR gerado em %s", options.output);
        if (codegen.kernel_count > 0) {
            printf("; %zu nucleo(s) MLIR em %s", codegen.kernel_count,
                   mlir_path);
        }
        putchar('\n');
    }

cleanup:
    codegen_result_destroy(&codegen);
    semantic_result_destroy(&semantic_result);
    semantic_destroy(&analyzer);
    parse_result_destroy(&parse_result);
    parser_destroy(&parser);
    free(source);
    free(mlir_path);
    return exit_code;
}
