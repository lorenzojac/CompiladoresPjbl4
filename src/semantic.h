#ifndef MACARRONICA_SEMANTIC_H
#define MACARRONICA_SEMANTIC_H

/*
 * Analise semantica da Macaronica, em duas passadas (D26):
 *   1. coleta registros, variaveis globais e assinaturas de funcoes;
 *   2. analisa os corpos das funcoes: escopos, tipos, chamadas, retornos.
 * Todos os erros sao acumulados como SemanticDiagnostic, com codigo estavel.
 *
 * Alem de validar, a analise ANOTA a AST para o gerador de codigo (PjBL 3):
 * tipo de cada expressao (AstExpression.type), identificador unico de cada
 * variavel (symbol_id em declaradores, parametros, referencias e lvalues),
 * indice de campo de registro, funcao chamada e funcao embutida.
 */

#include <stdbool.h>
#include <stddef.h>

#include "ast.h"

typedef enum {
    SEMANTIC_OK,
    SEMANTIC_ERRORS,
    SEMANTIC_INTERNAL_FAILURE
} SemanticStatus;

typedef enum {
    SEM_TYPE_BOOL,
    SEM_TYPE_INTEIRA,
    SEM_TYPE_FLUT,
    SEM_TYPE_PALAVRA,
    SEM_TYPE_DUPLOCARPADO,
    SEM_TYPE_VAZIO,
    /* Registro inteiro (D39); detalhes em AstValueType.record. */
    SEM_TYPE_REGISTRO,
    /* Vetor inteiro, operando da DSL vetorial (D41); detalhes na anotacao. */
    SEM_TYPE_VETOR,
    SEM_TYPE_ERROR
} SemanticType;

typedef enum {
    SEM_UNDECLARED_IDENTIFIER,
    SEM_DUPLICATE_DECLARATION,
    SEM_NAME_KIND_MISMATCH,
    SEM_TYPE_MISMATCH,
    SEM_INVALID_ASSIGNMENT,
    SEM_NOT_A_VECTOR,
    SEM_INVALID_VECTOR_INDEX,
    SEM_INVALID_VECTOR_SIZE,
    SEM_UNKNOWN_FUNCTION,
    SEM_NOT_A_FUNCTION,
    SEM_ARGUMENT_COUNT_MISMATCH,
    SEM_ARGUMENT_TYPE_MISMATCH,
    SEM_VOID_VALUE,
    SEM_INVALID_RETURN,
    SEM_INVALID_CONDITION,
    SEM_INVALID_OPERATOR,
    SEM_INVALID_RECORD_DECLARATION,
    SEM_INVALID_PRINCIPAL,
    /* Funcao nao-vazio que pode chegar ao fim do corpo sem respost (D34). */
    SEM_MISSING_RETURN,
    /* Indice constante fora de [0, tamanho) (D40). */
    SEM_INDEX_OUT_OF_BOUNDS,
    /* Divisor constante igual a zero (D40). */
    SEM_DIVISION_BY_ZERO,
    /* Literal inteiro fora do intervalo de inteira (i32) (D38). */
    SEM_LITERAL_OUT_OF_RANGE,
    /* Acesso a campo inexistente ou em valor que nao e registro (D39). */
    SEM_INVALID_FIELD,
    /* Vetores de tamanhos diferentes na DSL vetorial (D41). */
    SEM_VECTOR_SIZE_MISMATCH,
    /* Operacao nao suportada pela DSL vetorial (D41). */
    SEM_INVALID_VECTOR_OPERATION,
    /* Variaveis excedem a RAM do alvo (--alvo, D45). */
    SEM_TARGET_MEMORY,
    /* Construcao nao suportada no alvo escolhido (--alvo, D45). */
    SEM_TARGET_UNSUPPORTED,
    SEM_INTERNAL
} SemanticDiagnosticCode;

/* Alvo de compilacao (--alvo). Afeta apenas verificacoes, nunca o IR. */
typedef enum {
    SEMANTIC_TARGET_PC,
    SEMANTIC_TARGET_UNO
} SemanticTarget;

typedef struct {
    SemanticDiagnosticCode code;
    SourceLocation location;
    char *symbol_name;
    size_t symbol_name_length;
    const char *message;
} SemanticDiagnostic;

typedef struct SemanticScope SemanticScope;

typedef struct {
    SemanticStatus status;
    SemanticDiagnostic *diagnostics;
    size_t diagnostic_count;
    /* Quantidade de symbol_id atribuidos (ids vao de 1 a symbol_count). */
    size_t symbol_count;
    /* Bytes estimados no alvo: globais + textos, e maior quadro de funcao. */
    size_t global_bytes;
    size_t max_frame_bytes;
} SemanticResult;

typedef struct {
    const AstProgram *program;
    SemanticScope *global_scope;
    SemanticScope *current_scope;
    const AstFunctionDecl *current_function;
    SemanticDiagnostic *diagnostics;
    size_t diagnostic_count;
    size_t diagnostic_capacity;
    bool fatal;
    bool analyzed;
    bool initialized;

    SemanticTarget target;
    size_t next_symbol_id;
    size_t last_symbol_id;
    /* Contabilidade de memoria para --alvo (tamanhos do alvo). */
    size_t global_bytes;
    size_t max_frame_bytes;
} SemanticAnalyzer;

bool semantic_init(
    SemanticAnalyzer *analyzer,
    const AstProgram *program
);

/* Opcional, entre semantic_init e semantic_analyze (padrao: PC). */
void semantic_set_target(SemanticAnalyzer *analyzer, SemanticTarget target);

SemanticResult semantic_analyze(SemanticAnalyzer *analyzer);

void semantic_destroy(SemanticAnalyzer *analyzer);
void semantic_result_destroy(SemanticResult *result);

/* RAM do Arduino Uno e reserva para pilha de chamadas e biblioteca (D45). */
#define SEMANTIC_UNO_RAM_BYTES ((size_t)2048)
#define SEMANTIC_UNO_RESERVED_BYTES ((size_t)512)

#endif
