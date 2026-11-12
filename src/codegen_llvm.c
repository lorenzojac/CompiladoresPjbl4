/*
 * codegen_llvm.c - gerador de LLVM IR textual (ver codegen_llvm.h).
 *
 * Forma "em memoria" (aceita pelo enunciado como ponto de partida): toda
 * variavel local e parametro ganha um alloca no bloco de entrada e e lida e
 * escrita com load/store; o opt -passes=mem2reg promove isso a SSA com phi
 * (docs/decisoes_backend.md, Secao "mem2reg").
 *
 * Organizacao do arquivo:
 *   1. estado do gerador, nomes e tipos LLVM;
 *   2. valores, constantes e conversoes (sitofp/fpext, D16/D38);
 *   3. blocos basicos e verificacoes em tempo de execucao;
 *   4. enderecos (variavel, elemento de vetor, campo de registro);
 *   5. expressoes;
 *   6. DSL vetorial: chamadas aos nucleos MLIR;
 *   7. comandos;
 *   8. funcoes, globais e montagem do modulo.
 *
 * Valores temporarios usam a numeracao implicita do LLVM (%0, %1, ...),
 * emitida estritamente na ordem do texto. Rotulos usam '_' (entao_3,
 * durante_teste_4), caractere que nao existe em identificadores da
 * linguagem (D03), entao nunca colidem com variaveis.
 */
#include "codegen_llvm.h"

#include "codegen_mlir.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Codigos de erro de execucao: mesmos valores de runtime/zl_runtime.h. */
enum {
    ZL_ERRO_INDICE = 2,
    ZL_ERRO_DIVISAO_ZERO = 3,
    ZL_ERRO_RAIZ_NEGATIVA = 4,
    ZL_ERRO_POTENCIA_NEGATIVA = 5
};

/* Funcoes externas: so sao declaradas no modulo se usadas. */
typedef enum {
    RT_IMPRIME_INTEIRA,
    RT_IMPRIME_FLUT,
    RT_IMPRIME_DUPLO,
    RT_IMPRIME_BOOL,
    RT_IMPRIME_TEXTO,
    RT_IMPRIME_NL,
    RT_LE_INTEIRA,
    RT_LE_FLUT,
    RT_LE_DUPLO,
    RT_LE_BOOL,
    RT_LE_TEXTO,
    RT_TEXTO_IGUAL,
    RT_POTENCIA_INTEIRA,
    RT_ERRO,
    RT_PINO_MODO,
    RT_PINO_ESCREVE,
    RT_PINO_LE,
    RT_ESPERA,
    RT_SQRT_F32,
    RT_SQRT_F64,
    RT_POW_F64, /* tambem para flut, em double (D45) */
    RT_POWI_F32,
    RT_POWI_F64,
    RT_COUNT
} RuntimeFunction;

static const char *const RUNTIME_DECLARATIONS[RT_COUNT] = {
    "declare void @zl_imprime_inteira(i32) nounwind",
    "declare void @zl_imprime_flut(float) nounwind",
    "declare void @zl_imprime_duplo(double) nounwind",
    "declare void @zl_imprime_bool(i32) nounwind",
    "declare void @zl_imprime_texto(ptr) nounwind",
    "declare void @zl_imprime_nl() nounwind",
    "declare i32 @zl_le_inteira(i32) nounwind",
    "declare float @zl_le_flut(i32) nounwind",
    "declare double @zl_le_duplo(i32) nounwind",
    "declare i32 @zl_le_bool(i32) nounwind",
    "declare ptr @zl_le_texto(i32) nounwind",
    "declare i32 @zl_texto_igual(ptr, ptr) nounwind",
    "declare i32 @zl_potencia_inteira(i32, i32, i32) nounwind",
    "declare void @zl_erro_execucao(i32, i32) nounwind noreturn",
    "declare void @zl_pino_modo(i32, i32, i32) nounwind",
    "declare void @zl_pino_escreve(i32, i32, i32) nounwind",
    "declare i32 @zl_pino_le(i32, i32) nounwind",
    "declare void @zl_espera(i32) nounwind",
    "declare float @llvm.sqrt.f32(float) nounwind",
    "declare double @llvm.sqrt.f64(double) nounwind",
    "declare double @llvm.pow.f64(double, double) nounwind",
    "declare float @llvm.powi.f32.i32(float, i32) nounwind",
    "declare double @llvm.powi.f64.i32(double, i32) nounwind"
};

/*
 * Nomes de funcoes de bibliotecas C que o LLVM reconhece e pode otimizar
 * pelo nome. Uma funcao do programa com um desses nomes ganha o sufixo
 * ".mac" no IR (@sqrt.mac), para que o otimizador nunca a confunda com a
 * versao da libc.
 */
static const char *const RESERVED_SYMBOLS[] = {
    "main", "setup", "loop", "exit", "abort", "abs", "labs", "fabs",
    "sqrt", "sqrtf", "pow", "powf", "exp", "log", "log2", "log10", "sin",
    "cos", "tan", "floor", "ceil", "round", "trunc", "fmod", "fmin", "fmax",
    "printf", "puts", "putchar", "getchar", "scanf", "malloc", "calloc",
    "realloc", "free", "memcpy", "memset", "memmove", "memcmp", "strlen",
    "strcmp", "strcpy", "strcat", "atoi", "atof", "strtol", "strtod", "rand",
    "srand", "time", "clock", "system", "signal", "raise", "write", "read"
};

typedef struct {
    bool defined;
    bool is_global;
    char name[96];
    AstValueType type;
} VarInfo;

typedef struct {
    char text[96];
    AstTypeKind kind;
    bool constant;
    /* Literal decimal que originou o valor: promocao exata a double. */
    const AstExpression *literal;
} Value;

typedef struct {
    char text[96];
    AstValueType type;
} Address;

/*
 * Faixa provada de uma variavel de controle de repete (D47): dentro do
 * corpo, symbol_id vale entre low e high, inclusive.
 */
typedef struct {
    size_t symbol_id;
    long long low;
    long long high;
} LoopRange;

#define MAX_LOOP_RANGES 32

typedef struct {
    const AstProgram *program;
    VarInfo *vars;
    size_t var_count;

    /* Laços repete aninhados com faixa provada, do mais externo ao atual. */
    LoopRange ranges[MAX_LOOP_RANGES];
    size_t range_count;

    StrBuf types;
    StrBuf globals;
    StrBuf strings;
    StrBuf declarations;
    StrBuf functions;
    StrBuf entry;
    StrBuf body;
    MlirModule mlir;

    size_t temp;
    size_t label;
    size_t string_count;
    char current_label[64];
    bool terminated;
    const AstFunctionDecl *function;

    char (*local_names)[96];
    size_t local_count;
    size_t local_capacity;

    bool runtime_used[RT_COUNT];
    bool failed;
} Codegen;

static Value emit_expression(Codegen *cg, const AstExpression *expression);
static void emit_block(Codegen *cg, const AstBlock *block);
static Value emit_reduction(Codegen *cg, const AstReference *reference,
                            const AstExpression *call);

/* ------------------------------------------------------------------ */
/* 1. Nomes e tipos                                                     */
/* ------------------------------------------------------------------ */

static const char *scalar_type(AstTypeKind kind)
{
    switch (kind) {
    case AST_TYPE_BOOL:
        return "i1";
    case AST_TYPE_INTEIRA:
        return "i32";
    case AST_TYPE_FLUT:
        return "float";
    case AST_TYPE_DUPLOCARPADO:
        return "double";
    case AST_TYPE_PALAVRA:
        return "ptr";
    default:
        return "void";
    }
}

static bool is_real(AstTypeKind kind)
{
    return kind == AST_TYPE_FLUT || kind == AST_TYPE_DUPLOCARPADO;
}

static AstTypeKind wider(AstTypeKind a, AstTypeKind b)
{
    if (a == AST_TYPE_DUPLOCARPADO || b == AST_TYPE_DUPLOCARPADO) {
        return AST_TYPE_DUPLOCARPADO;
    }
    if (a == AST_TYPE_FLUT || b == AST_TYPE_FLUT) {
        return AST_TYPE_FLUT;
    }
    return a;
}

/* Tipo LLVM de um elemento (escalar ou registro, sem o vetor). */
static void element_type(char *out, size_t size, const AstValueType *type)
{
    if (type->kind == AST_TYPE_REGISTRO && type->record != NULL) {
        snprintf(out, size, "%%struct.%s", type->record->name);
    } else {
        snprintf(out, size, "%s", scalar_type(type->kind));
    }
}

/* Tipo LLVM completo: [N x T] para vetores. */
static void full_type(char *out, size_t size, const AstValueType *type)
{
    char element[96];

    element_type(element, sizeof(element), type);
    if (type->vector_size > 0) {
        snprintf(out, size, "[%zu x %s]", type->vector_size, element);
    } else {
        snprintf(out, size, "%s", element);
    }
}

static const char *zero_of(const AstValueType *type)
{
    if (type->vector_size > 0 || type->kind == AST_TYPE_REGISTRO) {
        return "zeroinitializer";
    }
    switch (type->kind) {
    case AST_TYPE_BOOL:
        return "false";
    case AST_TYPE_FLUT:
    case AST_TYPE_DUPLOCARPADO:
        return "0.000000e+00";
    case AST_TYPE_PALAVRA:
        return "null";
    default:
        return "0";
    }
}

static AstValueType scalar_value_type(AstTypeKind kind)
{
    AstValueType type = {0};

    type.kind = kind;
    type.valid = true;
    return type;
}

/* Nome LLVM (sem '@') de uma funcao do programa. */
static void function_symbol(char *out, size_t size,
                            const AstFunctionDecl *function)
{
    size_t i;

    if (function->is_main) {
        snprintf(out, size, "zl_principal");
        return;
    }
    for (i = 0; i < sizeof(RESERVED_SYMBOLS) / sizeof(RESERVED_SYMBOLS[0]);
         i++) {
        if (strcmp(function->name, RESERVED_SYMBOLS[i]) == 0) {
            snprintf(out, size, "%s.mac", function->name);
            return;
        }
    }
    snprintf(out, size, "%s", function->name);
}

static bool local_name_used(const Codegen *cg, const char *name)
{
    size_t i;

    for (i = 0; i < cg->local_count; i++) {
        if (strcmp(cg->local_names[i], name) == 0) {
            return true;
        }
    }
    return false;
}

static void remember_local_name(Codegen *cg, const char *name)
{
    if (cg->local_count == cg->local_capacity) {
        size_t capacity = cg->local_capacity == 0 ? 16
                                                  : cg->local_capacity * 2;
        char (*grown)[96] = realloc(cg->local_names,
                                    capacity * sizeof(*grown));

        if (grown == NULL) {
            cg->failed = true;
            return;
        }
        cg->local_names = grown;
        cg->local_capacity = capacity;
    }
    snprintf(cg->local_names[cg->local_count++], 96, "%s", name);
}

/*
 * Nome unico dentro da funcao derivado do nome do fonte (D25 permite
 * shadowing): "x", depois "x.2", "x.3"... O sufixo opcional forma
 * "x.addr" para parametros.
 */
static void unique_local_name(Codegen *cg, const char *base,
                              const char *suffix, char *out, size_t size)
{
    size_t n = 2;

    snprintf(out, size, "%s%s", base, suffix);
    while (local_name_used(cg, out)) {
        snprintf(out, size, "%s%s.%zu", base, suffix, n++);
    }
    remember_local_name(cg, out);
}

static VarInfo *var_info(Codegen *cg, size_t id)
{
    if (id == 0 || id >= cg->var_count || !cg->vars[id].defined) {
        cg->failed = true;
        return NULL;
    }
    return &cg->vars[id];
}

static void use_runtime(Codegen *cg, RuntimeFunction function)
{
    cg->runtime_used[function] = true;
}

/* ------------------------------------------------------------------ */
/* 2. Valores, constantes e conversoes                                 */
/* ------------------------------------------------------------------ */

static Value new_temp(Codegen *cg, AstTypeKind kind)
{
    Value value = {0};

    snprintf(value.text, sizeof(value.text), "%%%zu", cg->temp++);
    value.kind = kind;
    return value;
}

static Value constant_value(const char *text, AstTypeKind kind)
{
    Value value = {0};

    snprintf(value.text, sizeof(value.text), "%s", text);
    value.kind = kind;
    value.constant = true;
    return value;
}

/*
 * Constante de ponto flutuante no formato hexadecimal exato do LLVM. Para
 * float, o valor e arredondado para float e escrito como double (exigencia
 * do formato textual do LLVM IR).
 */
static Value real_constant(double number, AstTypeKind kind)
{
    char text[40];
    uint64_t bits;
    double exact = kind == AST_TYPE_FLUT ? (double)(float)number : number;

    memcpy(&bits, &exact, sizeof(bits));
    snprintf(text, sizeof(text), "0x%016llX", (unsigned long long)bits);
    return constant_value(text, kind);
}

/*
 * Converte um valor para outro tipo numerico. So existem as promocoes da
 * D16 (inteira -> flut -> duplocarpado). Constantes sao convertidas aqui
 * mesmo; um literal decimal promovido a duplocarpado e relido do texto
 * (D38), para que 0.1 vire o double 0.1, e nao o float 0.1 estendido.
 */
static Value convert(Codegen *cg, Value value, AstTypeKind to)
{
    Value result;

    if (value.kind == to || cg->failed) {
        return value;
    }
    if (value.kind == AST_TYPE_INTEIRA && is_real(to)) {
        if (value.constant) {
            return real_constant(strtod(value.text, NULL), to);
        }
        result = new_temp(cg, to);
        strbuf_appendf(&cg->body, "  %s = sitofp i32 %s to %s\n",
                       result.text, value.text, scalar_type(to));
        return result;
    }
    if (value.kind == AST_TYPE_FLUT && to == AST_TYPE_DUPLOCARPADO) {
        if (value.literal != NULL) {
            return real_constant(
                strtod(value.literal->as.literal.lexeme, NULL),
                AST_TYPE_DUPLOCARPADO
            );
        }
        result = new_temp(cg, to);
        strbuf_appendf(&cg->body, "  %s = fpext float %s to double\n",
                       result.text, value.text);
        return result;
    }
    cg->failed = true;
    return value;
}

/* bool (i1) vai para a biblioteca como i32 (ABI simples em todo alvo). */
static Value widen_bool(Codegen *cg, Value value)
{
    Value result = new_temp(cg, AST_TYPE_INTEIRA);

    strbuf_appendf(&cg->body, "  %s = zext i1 %s to i32\n", result.text,
                   value.text);
    return result;
}

/* Literal de texto: constante global [N x i8] terminada em zero. */
static Value text_constant(Codegen *cg, const AstExpression *literal)
{
    const char *lexeme = literal->as.literal.lexeme;
    size_t length = literal->as.literal.lexeme_length;
    size_t content = length >= 2 ? length - 2 : 0;
    Value value;
    size_t i;

    snprintf(value.text, sizeof(value.text), "@.str.%zu", cg->string_count++);
    value.kind = AST_TYPE_PALAVRA;
    value.constant = true;
    value.literal = NULL;
    strbuf_appendf(&cg->strings,
                   "%s = private unnamed_addr constant [%zu x i8] c\"",
                   value.text, content + 1);
    for (i = 1; i <= content; i++) {
        unsigned char byte = (unsigned char)lexeme[i];

        if (byte >= 0x20 && byte < 0x7F && byte != '"' && byte != '\\') {
            strbuf_appendf(&cg->strings, "%c", (char)byte);
        } else {
            strbuf_appendf(&cg->strings, "\\%02X", byte);
        }
    }
    strbuf_append(&cg->strings, "\\00\"\n");
    return value;
}

/* ------------------------------------------------------------------ */
/* 3. Blocos basicos e verificacoes em tempo de execucao               */
/* ------------------------------------------------------------------ */

static size_t new_label(Codegen *cg)
{
    return cg->label++;
}

/* Instrucao depois de um terminador precisa de um bloco novo (morto). */
static void ensure_block(Codegen *cg)
{
    if (cg->terminated) {
        snprintf(cg->current_label, sizeof(cg->current_label),
                 "inalcancavel_%zu", new_label(cg));
        strbuf_appendf(&cg->body, "%s:\n", cg->current_label);
        cg->terminated = false;
    }
}

static void begin_block(Codegen *cg, const char *label)
{
    if (!cg->terminated) {
        strbuf_appendf(&cg->body, "  br label %%%s\n", label);
    }
    strbuf_appendf(&cg->body, "%s:\n", label);
    snprintf(cg->current_label, sizeof(cg->current_label), "%s", label);
    cg->terminated = false;
}

/* Salto incondicional; depois de um terminador (respost) nao ha o que saltar. */
static void branch(Codegen *cg, const char *label)
{
    if (cg->terminated) {
        return;
    }
    strbuf_appendf(&cg->body, "  br label %%%s\n", label);
    cg->terminated = true;
}

static void conditional_branch(Codegen *cg, const char *condition,
                               const char *if_true, const char *if_false)
{
    ensure_block(cg);
    strbuf_appendf(&cg->body, "  br i1 %s, label %%%s, label %%%s\n",
                   condition, if_true, if_false);
    cg->terminated = true;
}

/*
 * Protege a execucao: se "ok" for falso, chama zl_erro_execucao(codigo,
 * linha) e para (unreachable). O fluxo normal continua em <nome>_ok_N.
 */
static void runtime_check(Codegen *cg, const char *ok, int code,
                          size_t line, const char *name)
{
    size_t id = new_label(cg);
    char good[64];
    char bad[64];

    use_runtime(cg, RT_ERRO);
    snprintf(good, sizeof(good), "%s_ok_%zu", name, id);
    snprintf(bad, sizeof(bad), "%s_erro_%zu", name, id);
    conditional_branch(cg, ok, good, bad);
    strbuf_appendf(&cg->body, "%s:\n", bad);
    strbuf_appendf(&cg->body,
                   "  call void @zl_erro_execucao(i32 %d, i32 %zu)\n"
                   "  unreachable\n",
                   code, line);
    begin_block(cg, good);
}

/* ------------------------------------------------------------------ */
/* 4. Enderecos                                                        */
/* ------------------------------------------------------------------ */

static void base_address(const VarInfo *var, char *out, size_t size)
{
    snprintf(out, size, "%c%s", var->is_global ? '@' : '%', var->name);
}

/*
 * Analise de faixa dos indices (D47).
 *
 * Um repete da forma
 *     repete (i receba A, i < B, i receba i + K) edai [ corpo ]
 * com A, B e K constantes inteiras, K >= 1, i variavel LOCAL inteira (uma
 * chamada nao pode altera-la) e nenhuma atribuicao ou le(i) no corpo,
 * garante que dentro do corpo A <= i <= B - 1. Exige-se tambem que o ultimo
 * incremento nao estoure (B - 1 + K <= 2^31 - 1): com a volta do
 * complemento de dois (D43) i ficaria negativo e o laco continuaria.
 * Um indice formado por essas variaveis, constantes e + - * tem entao uma
 * faixa calculada em tempo de compilacao; se ela cabe em [0, N) a
 * verificacao de execucao e desnecessaria e nao e emitida.
 */
static bool fits_int32(long long value)
{
    return value >= -2147483647LL - 1 && value <= 2147483647LL;
}

static bool is_plain_variable(const AstExpression *expression, size_t id)
{
    return expression != NULL && expression->kind == AST_EXPR_REFERENCE &&
           expression->as.reference.kind == AST_REFERENCE_VARIABLE &&
           expression->as.reference.field_name == NULL &&
           expression->as.reference.symbol_id == id;
}

static bool lvalue_is(const AstLValue *lvalue, size_t id)
{
    return lvalue != NULL && lvalue->symbol_id == id;
}

static bool block_assigns(const AstBlock *block, size_t id);

static bool statement_assigns(const AstStatement *statement, size_t id)
{
    size_t i;

    switch (statement->kind) {
    case AST_STMT_ASSIGNMENT:
        return lvalue_is(statement->as.assignment->target, id);
    case AST_STMT_IO:
        return statement->as.io->kind == AST_IO_READ &&
               lvalue_is(statement->as.io->as.target, id);
    case AST_STMT_IF: {
        const AstIf *s = statement->as.if_statement;

        if (block_assigns(s->then_block, id) ||
            block_assigns(s->else_block, id)) {
            return true;
        }
        for (i = 0; i < s->else_if_count; i++) {
            if (block_assigns(s->else_if_branches[i]->block, id)) {
                return true;
            }
        }
        return false;
    }
    case AST_STMT_WHILE:
        return block_assigns(statement->as.while_statement->body, id);
    case AST_STMT_REPEAT: {
        const AstRepeat *s = statement->as.repeat_statement;

        return (s->initialization != NULL &&
                lvalue_is(s->initialization->target, id)) ||
               (s->update != NULL && lvalue_is(s->update->target, id)) ||
               block_assigns(s->body, id);
    }
    case AST_STMT_BLOCK:
        return block_assigns(statement->as.block, id);
    default:
        /* declaracao (nome novo), respost, imprime, chamada isolada */
        return false;
    }
}

static bool block_assigns(const AstBlock *block, size_t id)
{
    size_t i;

    if (block == NULL) {
        return false;
    }
    for (i = 0; i < block->statement_count; i++) {
        if (statement_assigns(block->statements[i], id)) {
            return true;
        }
    }
    return false;
}

static bool repeat_range(Codegen *cg, const AstRepeat *statement,
                         LoopRange *out)
{
    const AstAssignment *init = statement->initialization;
    const AstAssignment *update = statement->update;
    const AstExpression *condition = statement->condition;
    const AstExpression *step;
    const VarInfo *var;
    long long low;
    long long bound;
    long long increment;
    size_t id;

    if (init == NULL || update == NULL || condition == NULL ||
        init->target == NULL || init->target->kind != AST_LVALUE_VARIABLE ||
        init->target->field_name != NULL) {
        return false;
    }
    id = init->target->symbol_id;
    var = var_info(cg, id);
    if (var == NULL || var->is_global ||
        var->type.kind != AST_TYPE_INTEIRA || var->type.vector_size != 0 ||
        !ast_constant_integer(init->value, &low)) {
        return false;
    }
    /* i < B  ou  B > i */
    if (condition->kind != AST_EXPR_BINARY) {
        return false;
    }
    if (condition->as.binary.op == AST_BINARY_LESS &&
        is_plain_variable(condition->as.binary.left, id) &&
        ast_constant_integer(condition->as.binary.right, &bound)) {
    } else if (condition->as.binary.op == AST_BINARY_GREATER &&
               is_plain_variable(condition->as.binary.right, id) &&
               ast_constant_integer(condition->as.binary.left, &bound)) {
    } else {
        return false;
    }
    /* i receba i + K  ou  i receba K + i, K >= 1 */
    step = update->value;
    if (!lvalue_is(update->target, id) ||
        update->target->kind != AST_LVALUE_VARIABLE ||
        update->target->field_name != NULL || step == NULL ||
        step->kind != AST_EXPR_BINARY || step->as.binary.op != AST_BINARY_ADD) {
        return false;
    }
    if (is_plain_variable(step->as.binary.left, id) &&
        ast_constant_integer(step->as.binary.right, &increment)) {
    } else if (is_plain_variable(step->as.binary.right, id) &&
               ast_constant_integer(step->as.binary.left, &increment)) {
    } else {
        return false;
    }
    if (increment < 1 || !fits_int32(bound - 1 + increment) ||
        block_assigns(statement->body, id)) {
        return false;
    }
    out->symbol_id = id;
    out->low = low;
    out->high = bound - 1;
    return true;
}

/* Faixa [low, high] de uma expressao inteira, se calculavel. */
static bool expression_range(const Codegen *cg, const AstExpression *e,
                             long long *low, long long *high)
{
    long long a_low;
    long long a_high;
    long long b_low;
    long long b_high;
    long long value;
    size_t i;

    if (e == NULL || e->type.kind != AST_TYPE_INTEIRA) {
        return false;
    }
    if (ast_constant_integer(e, &value)) {
        *low = *high = value;
        return true;
    }
    switch (e->kind) {
    case AST_EXPR_REFERENCE:
        if (e->as.reference.kind != AST_REFERENCE_VARIABLE ||
            e->as.reference.field_name != NULL) {
            return false;
        }
        for (i = cg->range_count; i > 0; i--) {
            if (cg->ranges[i - 1].symbol_id == e->as.reference.symbol_id) {
                *low = cg->ranges[i - 1].low;
                *high = cg->ranges[i - 1].high;
                return true;
            }
        }
        return false;
    case AST_EXPR_UNARY:
        if (!expression_range(cg, e->as.unary.operand, &a_low, &a_high)) {
            return false;
        }
        if (e->as.unary.op == AST_UNARY_PLUS) {
            *low = a_low;
            *high = a_high;
        } else if (e->as.unary.op == AST_UNARY_MINUS) {
            *low = -a_high;
            *high = -a_low;
        } else {
            return false;
        }
        break;
    case AST_EXPR_BINARY:
        if (!expression_range(cg, e->as.binary.left, &a_low, &a_high) ||
            !expression_range(cg, e->as.binary.right, &b_low, &b_high)) {
            return false;
        }
        switch (e->as.binary.op) {
        case AST_BINARY_ADD:
            *low = a_low + b_low;
            *high = a_high + b_high;
            break;
        case AST_BINARY_SUBTRACT:
            *low = a_low - b_high;
            *high = a_high - b_low;
            break;
        case AST_BINARY_MULTIPLY: {
            long long p[4];
            int k;

            p[0] = a_low * b_low;
            p[1] = a_low * b_high;
            p[2] = a_high * b_low;
            p[3] = a_high * b_high;
            *low = *high = p[0];
            for (k = 1; k < 4; k++) {
                *low = p[k] < *low ? p[k] : *low;
                *high = p[k] > *high ? p[k] : *high;
            }
            break;
        }
        default:
            return false;
        }
        break;
    default:
        return false;
    }
    /* Fora de 32 bits a conta real daria a volta: faixa desconhecida. */
    return fits_int32(*low) && fits_int32(*high);
}

/*
 * Endereco de nome{indice}.campo. O indice e verificado contra o tamanho
 * em tempo de execucao (icmp ult cobre tambem indices negativos), exceto
 * quando e constante (a semantica ja provou que esta nos limites, D40) ou
 * quando a analise de faixa prova que esta em [0, N) (D47).
 */
static bool access_address(
    Codegen *cg,
    size_t symbol_id,
    const AstExpression *index,
    const char *field_name,
    size_t field_index,
    SourceLocation location,
    Address *out
)
{
    VarInfo *var = var_info(cg, symbol_id);
    char pointer[96];
    char type_text[160];
    long long constant;

    if (var == NULL) {
        return false;
    }
    base_address(var, pointer, sizeof(pointer));
    out->type = var->type;

    if (index != NULL) {
        Value position = convert(cg, emit_expression(cg, index),
                                 AST_TYPE_INTEIRA);
        Value element;
        long long low;
        long long high;

        if (ast_constant_integer(index, &constant)) {
            /* provado pela semantica (D40) */
        } else if (expression_range(cg, index, &low, &high) && low >= 0 &&
                   high < (long long)var->type.vector_size) {
            strbuf_appendf(&cg->body,
                           "  ; linha %zu: indice em [%lld, %lld] dentro de "
                           "[0, %zu), sem verificacao (D47)\n",
                           location.line, low, high, var->type.vector_size);
        } else {
            Value inside = new_temp(cg, AST_TYPE_BOOL);

            strbuf_appendf(&cg->body, "  %s = icmp ult i32 %s, %zu\n",
                           inside.text, position.text, var->type.vector_size);
            runtime_check(cg, inside.text, ZL_ERRO_INDICE, location.line,
                          "indice");
        }
        full_type(type_text, sizeof(type_text), &var->type);
        element = new_temp(cg, AST_TYPE_PALAVRA);
        strbuf_appendf(&cg->body,
                       "  %s = getelementptr inbounds %s, ptr %s, i32 0, "
                       "i32 %s\n",
                       element.text, type_text, pointer, position.text);
        snprintf(pointer, sizeof(pointer), "%s", element.text);
        out->type.vector_size = 0;
    }

    if (field_name != NULL) {
        const AstRecordDecl *record = out->type.record;
        Value field = new_temp(cg, AST_TYPE_PALAVRA);

        if (record == NULL || field_index >= record->field_count) {
            cg->failed = true;
            return false;
        }
        strbuf_appendf(&cg->body,
                       "  %s = getelementptr inbounds %%struct.%s, ptr %s, "
                       "i32 0, i32 %zu\n",
                       field.text, record->name, pointer, field_index);
        snprintf(pointer, sizeof(pointer), "%s", field.text);
        out->type = scalar_value_type(record->fields[field_index]->type.kind);
    }

    snprintf(out->text, sizeof(out->text), "%s", pointer);
    return true;
}

static bool lvalue_address(Codegen *cg, const AstLValue *lvalue,
                           Address *out)
{
    return access_address(
        cg,
        lvalue->symbol_id,
        lvalue->kind == AST_LVALUE_VECTOR ? lvalue->index : NULL,
        lvalue->field_name,
        lvalue->field_index,
        lvalue->location,
        out
    );
}

static Value load(Codegen *cg, const Address *address)
{
    Value value = new_temp(cg, address->type.kind);

    strbuf_appendf(&cg->body, "  %s = load %s, ptr %s\n", value.text,
                   scalar_type(address->type.kind), address->text);
    return value;
}

/* ------------------------------------------------------------------ */
/* 5. Expressoes                                                       */
/* ------------------------------------------------------------------ */

static Value emit_literal(Codegen *cg, const AstExpression *expression)
{
    Value value;

    switch (expression->as.literal.kind) {
    case AST_LITERAL_INTEGER:
        return constant_value(expression->as.literal.lexeme,
                              AST_TYPE_INTEIRA);
    case AST_LITERAL_DECIMAL:
        value = real_constant(strtod(expression->as.literal.lexeme, NULL),
                              AST_TYPE_FLUT);
        value.literal = expression;
        return value;
    case AST_LITERAL_BOOLEAN:
        return constant_value(
            strcmp(expression->as.literal.lexeme, "VER") == 0 ? "true"
                                                              : "false",
            AST_TYPE_BOOL);
    default:
        return text_constant(cg, expression);
    }
}

/* Argumentos ja convertidos para os tipos dos parametros. */
static void append_arguments(StrBuf *out, Value *arguments, size_t count)
{
    size_t i;

    for (i = 0; i < count; i++) {
        strbuf_appendf(out, "%s%s %s", i > 0 ? ", " : "",
                       scalar_type(arguments[i].kind), arguments[i].text);
    }
}

/* Chamada de funcao do programa ou embutida; vazio devolve kind VAZIO. */
static Value emit_call(Codegen *cg, const AstExpression *expression)
{
    const AstReference *reference = &expression->as.reference;
    size_t count = reference->as.call.argument_count;
    Value result = {0};
    size_t line = expression->location.line;

    result.kind = AST_TYPE_VAZIO;
    if (reference->builtin != AST_BUILTIN_NONE) {
        Value a;
        Value b;

        switch (reference->builtin) {
        case AST_BUILTIN_SOMAVET:
        case AST_BUILTIN_MAXVET:
        case AST_BUILTIN_MINVET:
        case AST_BUILTIN_PRODESC:
            return emit_reduction(cg, reference, expression);
        case AST_BUILTIN_PINOMODO:
        case AST_BUILTIN_PINOESCREVE:
            a = convert(cg, emit_expression(cg, reference->as.call.arguments[0]),
                        AST_TYPE_INTEIRA);
            b = convert(cg, emit_expression(cg, reference->as.call.arguments[1]),
                        AST_TYPE_INTEIRA);
            use_runtime(cg, reference->builtin == AST_BUILTIN_PINOMODO
                                ? RT_PINO_MODO
                                : RT_PINO_ESCREVE);
            strbuf_appendf(&cg->body,
                           "  call void @%s(i32 %s, i32 %s, i32 %zu)\n",
                           reference->builtin == AST_BUILTIN_PINOMODO
                               ? "zl_pino_modo"
                               : "zl_pino_escreve",
                           a.text, b.text, line);
            return result;
        case AST_BUILTIN_PINOLE:
            a = convert(cg, emit_expression(cg, reference->as.call.arguments[0]),
                        AST_TYPE_INTEIRA);
            use_runtime(cg, RT_PINO_LE);
            result = new_temp(cg, AST_TYPE_INTEIRA);
            strbuf_appendf(&cg->body,
                           "  %s = call i32 @zl_pino_le(i32 %s, i32 %zu)\n",
                           result.text, a.text, line);
            return result;
        case AST_BUILTIN_ESPERA:
            a = convert(cg, emit_expression(cg, reference->as.call.arguments[0]),
                        AST_TYPE_INTEIRA);
            use_runtime(cg, RT_ESPERA);
            strbuf_appendf(&cg->body, "  call void @zl_espera(i32 %s)\n",
                           a.text);
            return result;
        default:
            cg->failed = true;
            return result;
        }
    }

    {
        const AstFunctionDecl *callee = reference->callee;
        Value *arguments = NULL;
        char symbol[112];
        size_t i;

        if (callee == NULL || callee->parameter_count != count) {
            cg->failed = true;
            return result;
        }
        if (count > 0) {
            arguments = calloc(count, sizeof(*arguments));
            if (arguments == NULL) {
                cg->failed = true;
                return result;
            }
        }
        for (i = 0; i < count; i++) {
            arguments[i] = convert(
                cg,
                emit_expression(cg, reference->as.call.arguments[i]),
                callee->parameters[i]->type.kind
            );
        }
        function_symbol(symbol, sizeof(symbol), callee);
        if (callee->return_type.kind == AST_TYPE_VAZIO) {
            strbuf_appendf(&cg->body, "  call void @%s(", symbol);
        } else {
            result = new_temp(cg, callee->return_type.kind);
            strbuf_appendf(&cg->body, "  %s = call %s @%s(", result.text,
                           scalar_type(callee->return_type.kind), symbol);
        }
        append_arguments(&cg->body, arguments, count);
        strbuf_append(&cg->body, ")\n");
        free(arguments);
        return result;
    }
}

/*
 * E e OU com curto-circuito (decisao D42): o operando da direita so e
 * avaliado se necessario, e o resultado vem de um phi no bloco de juncao.
 */
static Value emit_short_circuit(Codegen *cg, const AstExpression *expression)
{
    bool is_and = expression->as.binary.op == AST_BINARY_AND;
    size_t id = new_label(cg);
    char right_label[64];
    char end_label[64];
    char left_from[64];
    char right_from[64];
    Value left;
    Value right;
    Value result;

    snprintf(right_label, sizeof(right_label), "%s_direita_%zu",
             is_and ? "e" : "ou", id);
    snprintf(end_label, sizeof(end_label), "%s_fim_%zu", is_and ? "e" : "ou",
             id);

    left = emit_expression(cg, expression->as.binary.left);
    ensure_block(cg);
    snprintf(left_from, sizeof(left_from), "%s", cg->current_label);
    if (is_and) {
        conditional_branch(cg, left.text, right_label, end_label);
    } else {
        conditional_branch(cg, left.text, end_label, right_label);
    }
    begin_block(cg, right_label);
    right = emit_expression(cg, expression->as.binary.right);
    ensure_block(cg);
    snprintf(right_from, sizeof(right_from), "%s", cg->current_label);
    branch(cg, end_label);
    begin_block(cg, end_label);
    result = new_temp(cg, AST_TYPE_BOOL);
    strbuf_appendf(&cg->body, "  %s = phi i1 [ %s, %%%s ], [ %s, %%%s ]\n",
                   result.text, is_and ? "false" : "true", left_from,
                   right.text, right_from);
    return result;
}

static bool constant_nonzero(const AstExpression *divisor)
{
    long long value;

    if (ast_constant_integer(divisor, &value)) {
        return value != 0;
    }
    return divisor->kind == AST_EXPR_LITERAL &&
           divisor->as.literal.kind == AST_LITERAL_DECIMAL &&
           strtod(divisor->as.literal.lexeme, NULL) != 0.0;
}

static Value emit_binary(Codegen *cg, const AstExpression *expression)
{
    const AstExpression *left_node = expression->as.binary.left;
    const AstExpression *right_node = expression->as.binary.right;
    AstTypeKind kind = expression->type.kind;
    AstTypeKind operands;
    size_t line = expression->location.line;
    Value left;
    Value right;
    Value result;
    const char *op = NULL;

    switch (expression->as.binary.op) {
    case AST_BINARY_AND:
    case AST_BINARY_OR:
        return emit_short_circuit(cg, expression);
    default:
        break;
    }

    left = emit_expression(cg, left_node);
    right = emit_expression(cg, right_node);
    operands = wider(left_node->type.kind, right_node->type.kind);

    switch (expression->as.binary.op) {
    case AST_BINARY_ADD:
    case AST_BINARY_SUBTRACT:
    case AST_BINARY_MULTIPLY:
        left = convert(cg, left, kind);
        right = convert(cg, right, kind);
        if (expression->as.binary.op == AST_BINARY_ADD) {
            op = is_real(kind) ? "fadd" : "add";
        } else if (expression->as.binary.op == AST_BINARY_SUBTRACT) {
            op = is_real(kind) ? "fsub" : "sub";
        } else {
            op = is_real(kind) ? "fmul" : "mul";
        }
        result = new_temp(cg, kind);
        strbuf_appendf(&cg->body, "  %s = %s %s %s, %s\n", result.text, op,
                       scalar_type(kind), left.text, right.text);
        return result;

    case AST_BINARY_DIVIDE: {
        /* '/' e sempre divisao real (D18/D38); divisor zero e erro (D43). */
        left = convert(cg, left, kind);
        right = convert(cg, right, kind);
        if (!constant_nonzero(right_node)) {
            Value nonzero = new_temp(cg, AST_TYPE_BOOL);

            strbuf_appendf(&cg->body, "  %s = fcmp une %s %s, 0.000000e+00\n",
                           nonzero.text, scalar_type(kind), right.text);
            runtime_check(cg, nonzero.text, ZL_ERRO_DIVISAO_ZERO, line,
                          "divisao");
        }
        result = new_temp(cg, kind);
        strbuf_appendf(&cg->body, "  %s = fdiv %s %s, %s\n", result.text,
                       scalar_type(kind), left.text, right.text);
        return result;
    }

    case AST_BINARY_DIV_INT: {
        /*
         * '//' (D19): sdiv sobre i32. Divisor zero e erro (D43). O unico
         * estouro, -2147483648 // -1, e definido como -2147483648 (resto da
         * aritmetica em complemento de dois, D43): o divisor vira 1.
         */
        long long divisor;
        bool known = ast_constant_integer(right_node, &divisor);

        if (!known || divisor == 0) {
            Value nonzero = new_temp(cg, AST_TYPE_BOOL);

            strbuf_appendf(&cg->body, "  %s = icmp ne i32 %s, 0\n",
                           nonzero.text, right.text);
            runtime_check(cg, nonzero.text, ZL_ERRO_DIVISAO_ZERO, line,
                          "divisao");
        }
        if (!known || divisor == -1) {
            Value is_min = new_temp(cg, AST_TYPE_BOOL);
            Value is_minus_one = new_temp(cg, AST_TYPE_BOOL);
            Value overflow = new_temp(cg, AST_TYPE_BOOL);
            Value safe = new_temp(cg, AST_TYPE_INTEIRA);

            /* new_temp ja reservou os numeros; emite na mesma ordem. */
            strbuf_appendf(&cg->body,
                           "  %s = icmp eq i32 %s, -2147483648\n"
                           "  %s = icmp eq i32 %s, -1\n"
                           "  %s = and i1 %s, %s\n"
                           "  %s = select i1 %s, i32 1, i32 %s\n",
                           is_min.text, left.text, is_minus_one.text,
                           right.text, overflow.text, is_min.text,
                           is_minus_one.text, safe.text, overflow.text,
                           right.text);
            right = safe;
        }
        result = new_temp(cg, AST_TYPE_INTEIRA);
        strbuf_appendf(&cg->body, "  %s = sdiv i32 %s, %s\n", result.text,
                       left.text, right.text);
        return result;
    }

    case AST_BINARY_POWER:
        if (kind == AST_TYPE_INTEIRA) {
            /* inteira ^ inteira: biblioteca (expoente negativo e erro). */
            use_runtime(cg, RT_POTENCIA_INTEIRA);
            result = new_temp(cg, kind);
            strbuf_appendf(&cg->body,
                           "  %s = call i32 @zl_potencia_inteira(i32 %s, "
                           "i32 %s, i32 %zu)\n",
                           result.text, left.text, right.text, line);
            return result;
        }
        left = convert(cg, left, kind);
        if (right_node->type.kind == AST_TYPE_INTEIRA) {
            /* expoente inteiro: llvm.powi (multiplicacoes repetidas). */
            result = new_temp(cg, kind);
            use_runtime(cg, kind == AST_TYPE_FLUT ? RT_POWI_F32 : RT_POWI_F64);
            strbuf_appendf(&cg->body,
                           "  %s = call %s @llvm.powi.%s.i32(%s %s, i32 %s)\n",
                           result.text, scalar_type(kind),
                           kind == AST_TYPE_FLUT ? "f32" : "f64",
                           scalar_type(kind), left.text, right.text);
            return result;
        }
        {
            /*
             * Expoente real: llvm.pow.f64. Para flut, a conta e feita em
             * double e arredondada para float (fpext/fptrunc), em vez de
             * llvm.pow.f32: o powf da avr-libc 2.3.2 trava no Uno com
             * -mdouble=64, e assim PC e placa usam a mesma rotina (D45).
             */
            Value exponent = convert(cg, right, kind);
            Value base = left;
            Value power;

            use_runtime(cg, RT_POW_F64);
            if (kind == AST_TYPE_FLUT) {
                base = new_temp(cg, AST_TYPE_DUPLOCARPADO);
                strbuf_appendf(&cg->body, "  %s = fpext float %s to double\n",
                               base.text, left.text);
                exponent.kind = AST_TYPE_DUPLOCARPADO;
                {
                    Value widened = new_temp(cg, AST_TYPE_DUPLOCARPADO);

                    strbuf_appendf(&cg->body,
                                   "  %s = fpext float %s to double\n",
                                   widened.text, exponent.text);
                    exponent = widened;
                }
            }
            power = new_temp(cg, AST_TYPE_DUPLOCARPADO);
            strbuf_appendf(&cg->body,
                           "  %s = call double @llvm.pow.f64(double %s, "
                           "double %s)\n",
                           power.text, base.text, exponent.text);
            if (kind == AST_TYPE_DUPLOCARPADO) {
                return power;
            }
            result = new_temp(cg, AST_TYPE_FLUT);
            strbuf_appendf(&cg->body, "  %s = fptrunc double %s to float\n",
                           result.text, power.text);
            return result;
        }

    case AST_BINARY_LESS:
    case AST_BINARY_GREATER:
        left = convert(cg, left, operands);
        right = convert(cg, right, operands);
        result = new_temp(cg, AST_TYPE_BOOL);
        if (is_real(operands)) {
            op = expression->as.binary.op == AST_BINARY_LESS ? "fcmp olt"
                                                             : "fcmp ogt";
        } else {
            op = expression->as.binary.op == AST_BINARY_LESS ? "icmp slt"
                                                             : "icmp sgt";
        }
        strbuf_appendf(&cg->body, "  %s = %s %s %s, %s\n", result.text, op,
                       scalar_type(operands), left.text, right.text);
        return result;

    case AST_BINARY_EQUAL:
    case AST_BINARY_DIFFERENT: {
        bool equal = expression->as.binary.op == AST_BINARY_EQUAL;

        if (operands == AST_TYPE_PALAVRA) {
            /* Igualdade de texto compara o conteudo (D42). */
            Value same = new_temp(cg, AST_TYPE_INTEIRA);

            use_runtime(cg, RT_TEXTO_IGUAL);
            strbuf_appendf(&cg->body,
                           "  %s = call i32 @zl_texto_igual(ptr %s, ptr %s)\n",
                           same.text, left.text, right.text);
            result = new_temp(cg, AST_TYPE_BOOL);
            strbuf_appendf(&cg->body, "  %s = icmp %s i32 %s, 0\n",
                           result.text, equal ? "ne" : "eq", same.text);
            return result;
        }
        result = new_temp(cg, AST_TYPE_BOOL);
        if (is_real(operands)) {
            op = equal ? "fcmp oeq" : "fcmp une";
        } else {
            op = equal ? "icmp eq" : "icmp ne";
        }
        strbuf_appendf(&cg->body, "  %s = %s %s %s, %s\n", result.text, op,
                       scalar_type(operands), left.text, right.text);
        return result;
    }

    case AST_BINARY_XOR:
        result = new_temp(cg, AST_TYPE_BOOL);
        strbuf_appendf(&cg->body, "  %s = xor i1 %s, %s\n", result.text,
                       left.text, right.text);
        return result;

    default:
        cg->failed = true;
        return left;
    }
}

static Value emit_unary(Codegen *cg, const AstExpression *expression)
{
    AstTypeKind kind = expression->type.kind;
    Value operand = emit_expression(cg, expression->as.unary.operand);
    Value result;

    switch (expression->as.unary.op) {
    case AST_UNARY_PLUS:
        return operand;
    case AST_UNARY_MINUS:
        if (operand.constant && kind == AST_TYPE_INTEIRA &&
            operand.text[0] != '-' && strlen(operand.text) < 16) {
            Value negated = operand;

            negated.text[0] = '-';
            memcpy(negated.text + 1, operand.text, strlen(operand.text) + 1);
            return negated;
        }
        result = new_temp(cg, kind);
        if (is_real(kind)) {
            strbuf_appendf(&cg->body, "  %s = fneg %s %s\n", result.text,
                           scalar_type(kind), operand.text);
        } else {
            strbuf_appendf(&cg->body, "  %s = sub i32 0, %s\n", result.text,
                           operand.text);
        }
        return result;
    default:
        result = new_temp(cg, AST_TYPE_BOOL);
        strbuf_appendf(&cg->body, "  %s = xor i1 %s, true\n", result.text,
                       operand.text);
        return result;
    }
}

/* raiz(x): llvm.sqrt; argumento negativo e erro de execucao (D43). */
static Value emit_root(Codegen *cg, const AstExpression *expression)
{
    AstTypeKind kind = expression->type.kind;
    const AstExpression *argument_node = expression->as.root.argument;
    Value argument = convert(cg, emit_expression(cg, argument_node), kind);
    Value result;
    long long constant;
    bool known_positive =
        (ast_constant_integer(argument_node, &constant) && constant >= 0) ||
        (argument_node->kind == AST_EXPR_LITERAL &&
         argument_node->as.literal.kind == AST_LITERAL_DECIMAL);

    if (!known_positive) {
        Value valid = new_temp(cg, AST_TYPE_BOOL);

        strbuf_appendf(&cg->body, "  %s = fcmp oge %s %s, 0.000000e+00\n",
                       valid.text, scalar_type(kind), argument.text);
        runtime_check(cg, valid.text, ZL_ERRO_RAIZ_NEGATIVA,
                      expression->location.line, "raiz");
    }
    use_runtime(cg, kind == AST_TYPE_FLUT ? RT_SQRT_F32 : RT_SQRT_F64);
    result = new_temp(cg, kind);
    strbuf_appendf(&cg->body, "  %s = call %s @llvm.sqrt.%s(%s %s)\n",
                   result.text, scalar_type(kind),
                   kind == AST_TYPE_FLUT ? "f32" : "f64", scalar_type(kind),
                   argument.text);
    return result;
}

static Value emit_expression(Codegen *cg, const AstExpression *expression)
{
    Value none = {0};

    if (cg->failed || expression == NULL) {
        cg->failed = true;
        return none;
    }
    ensure_block(cg);
    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        return emit_literal(cg, expression);
    case AST_EXPR_REFERENCE: {
        const AstReference *reference = &expression->as.reference;
        Address address;

        if (reference->kind == AST_REFERENCE_CALL) {
            return emit_call(cg, expression);
        }
        if (!access_address(cg, reference->symbol_id,
                            reference->kind == AST_REFERENCE_VECTOR
                                ? reference->as.vector.index
                                : NULL,
                            reference->field_name, reference->field_index,
                            reference->location, &address)) {
            return none;
        }
        return load(cg, &address);
    }
    case AST_EXPR_UNARY:
        return emit_unary(cg, expression);
    case AST_EXPR_BINARY:
        return emit_binary(cg, expression);
    case AST_EXPR_ROOT:
        return emit_root(cg, expression);
    default:
        cg->failed = true;
        return none;
    }
}

/* ------------------------------------------------------------------ */
/* 6. DSL vetorial: chamada de um nucleo MLIR                          */
/* ------------------------------------------------------------------ */

/*
 * Avalia as folhas escalares (na ordem dos parametros), declara o nucleo
 * e o chama passando o endereco de cada vetor (bare pointer). Devolve o
 * valor de retorno (reducoes) ou kind VAZIO.
 */
static Value call_kernel(Codegen *cg, const MlirKernel *kernel)
{
    Value *arguments;
    Value result = {0};
    size_t i;

    result.kind = AST_TYPE_VAZIO;
    if (kernel == NULL) {
        cg->failed = true;
        return result;
    }
    arguments = calloc(kernel->leaf_count + 1, sizeof(*arguments));
    if (arguments == NULL) {
        cg->failed = true;
        return result;
    }
    for (i = 0; i < kernel->leaf_count; i++) {
        const MlirLeaf *leaf = &kernel->leaves[i];

        if (leaf->kind == MLIR_LEAF_VECTOR) {
            VarInfo *var = var_info(cg, leaf->symbol_id);

            if (var == NULL) {
                free(arguments);
                return result;
            }
            base_address(var, arguments[i].text, sizeof(arguments[i].text));
            arguments[i].kind = AST_TYPE_PALAVRA;
        } else {
            arguments[i] = emit_expression(cg, leaf->expression);
        }
    }

    strbuf_appendf(&cg->declarations, "declare %s @%s(",
                   scalar_type(kernel->result), kernel->name);
    for (i = 0; i < kernel->leaf_count; i++) {
        strbuf_appendf(&cg->declarations, "%s%s", i > 0 ? ", " : "",
                       scalar_type(arguments[i].kind));
    }
    strbuf_append(&cg->declarations, ") nounwind\n");

    if (kernel->result == AST_TYPE_VAZIO) {
        strbuf_appendf(&cg->body, "  call void @%s(", kernel->name);
    } else {
        result = new_temp(cg, kernel->result);
        strbuf_appendf(&cg->body, "  %s = call %s @%s(", result.text,
                       scalar_type(kernel->result), kernel->name);
    }
    append_arguments(&cg->body, arguments, kernel->leaf_count);
    strbuf_append(&cg->body, ")\n");
    free(arguments);
    return result;
}

static Value emit_reduction(Codegen *cg, const AstReference *reference,
                            const AstExpression *call)
{
    const MlirKernel *kernel = mlir_add_reduction(
        &cg->mlir,
        reference->builtin,
        reference->as.call.arguments[0],
        reference->builtin == AST_BUILTIN_PRODESC
            ? reference->as.call.arguments[1]
            : NULL,
        call->type.kind,
        call->location
    );

    strbuf_appendf(&cg->body, "  ; nucleo MLIR: %s\n",
                   kernel != NULL ? kernel->name : "?");
    return call_kernel(cg, kernel);
}

/* ------------------------------------------------------------------ */
/* 7. Comandos                                                         */
/* ------------------------------------------------------------------ */

static void emit_assignment(Codegen *cg, const AstAssignment *assignment)
{
    const AstLValue *target = assignment->target;
    Address address;
    Value value;

    if (target->type.vector_size > 0 && target->field_name == NULL &&
        target->kind == AST_LVALUE_VARIABLE) {
        /* vetor receba expressao vetorial ou escalar: nucleo MLIR (D41). */
        const MlirKernel *kernel = mlir_add_assignment(
            &cg->mlir,
            target->symbol_id,
            target->type.kind,
            target->type.vector_size,
            assignment->value,
            assignment->location
        );

        strbuf_appendf(&cg->body, "  ; nucleo MLIR: %s\n",
                       kernel != NULL ? kernel->name : "?");
        (void)call_kernel(cg, kernel);
        return;
    }

    if (!lvalue_address(cg, target, &address)) {
        return;
    }
    if (address.type.kind == AST_TYPE_REGISTRO) {
        /* Copia de registro inteiro (D39): load/store do agregado. */
        const AstExpression *source = assignment->value;
        Address from;
        Value copy;

        if (source->kind != AST_EXPR_REFERENCE ||
            !access_address(cg, source->as.reference.symbol_id,
                            source->as.reference.kind == AST_REFERENCE_VECTOR
                                ? source->as.reference.as.vector.index
                                : NULL,
                            source->as.reference.field_name,
                            source->as.reference.field_index,
                            source->location, &from)) {
            cg->failed = true;
            return;
        }
        copy = new_temp(cg, AST_TYPE_REGISTRO);
        strbuf_appendf(&cg->body,
                       "  %s = load %%struct.%s, ptr %s\n"
                       "  store %%struct.%s %s, ptr %s\n",
                       copy.text, address.type.record->name, from.text,
                       address.type.record->name, copy.text, address.text);
        return;
    }

    value = convert(cg, emit_expression(cg, assignment->value),
                    address.type.kind);
    ensure_block(cg);
    strbuf_appendf(&cg->body, "  store %s %s, ptr %s\n",
                   scalar_type(address.type.kind), value.text, address.text);
}

static void emit_local_declaration(Codegen *cg,
                                   const AstDeclaration *declaration)
{
    size_t i;

    for (i = 0; i < declaration->declarator_count; i++) {
        const AstDeclarator *declarator = declaration->declarators[i];
        VarInfo *var;
        char type_text[160];

        if (declarator->symbol_id == 0 ||
            declarator->symbol_id >= cg->var_count) {
            cg->failed = true;
            return;
        }
        var = &cg->vars[declarator->symbol_id];
        var->defined = true;
        var->is_global = false;
        var->type.kind = declaration->type.kind;
        var->type.vector_size = declarator->vector_size;
        var->type.record = declaration->record;
        var->type.valid = true;
        unique_local_name(cg, declarator->name, "", var->name,
                          sizeof(var->name));
        full_type(type_text, sizeof(type_text), &var->type);
        strbuf_appendf(&cg->entry, "  %%%s = alloca %s\n", var->name,
                       type_text);
        /* Toda variavel comeca com zero ao executar a declaracao (D42). */
        ensure_block(cg);
        strbuf_appendf(&cg->body, "  store %s %s, ptr %%%s\n", type_text,
                       zero_of(&var->type), var->name);
    }
}

static void emit_print(Codegen *cg, const AstExpression *expression)
{
    Value value = emit_expression(cg, expression);

    switch (value.kind) {
    case AST_TYPE_INTEIRA:
        use_runtime(cg, RT_IMPRIME_INTEIRA);
        strbuf_appendf(&cg->body, "  call void @zl_imprime_inteira(i32 %s)\n",
                       value.text);
        break;
    case AST_TYPE_FLUT:
        use_runtime(cg, RT_IMPRIME_FLUT);
        strbuf_appendf(&cg->body, "  call void @zl_imprime_flut(float %s)\n",
                       value.text);
        break;
    case AST_TYPE_DUPLOCARPADO:
        use_runtime(cg, RT_IMPRIME_DUPLO);
        strbuf_appendf(&cg->body,
                       "  call void @zl_imprime_duplo(double %s)\n",
                       value.text);
        break;
    case AST_TYPE_BOOL:
        value = widen_bool(cg, value);
        use_runtime(cg, RT_IMPRIME_BOOL);
        strbuf_appendf(&cg->body, "  call void @zl_imprime_bool(i32 %s)\n",
                       value.text);
        break;
    case AST_TYPE_PALAVRA:
        use_runtime(cg, RT_IMPRIME_TEXTO);
        strbuf_appendf(&cg->body, "  call void @zl_imprime_texto(ptr %s)\n",
                       value.text);
        break;
    default:
        cg->failed = true;
        return;
    }
    use_runtime(cg, RT_IMPRIME_NL);
    strbuf_append(&cg->body, "  call void @zl_imprime_nl()\n");
}

/* Le um valor do tipo pedido (entrada padrao no PC, serial na placa). */
static Value emit_read_value(Codegen *cg, AstTypeKind kind, size_t line)
{
    Value value;

    switch (kind) {
    case AST_TYPE_INTEIRA:
        use_runtime(cg, RT_LE_INTEIRA);
        value = new_temp(cg, kind);
        strbuf_appendf(&cg->body, "  %s = call i32 @zl_le_inteira(i32 %zu)\n",
                       value.text, line);
        return value;
    case AST_TYPE_FLUT:
        use_runtime(cg, RT_LE_FLUT);
        value = new_temp(cg, kind);
        strbuf_appendf(&cg->body, "  %s = call float @zl_le_flut(i32 %zu)\n",
                       value.text, line);
        return value;
    case AST_TYPE_DUPLOCARPADO:
        use_runtime(cg, RT_LE_DUPLO);
        value = new_temp(cg, kind);
        strbuf_appendf(&cg->body,
                       "  %s = call double @zl_le_duplo(i32 %zu)\n",
                       value.text, line);
        return value;
    case AST_TYPE_PALAVRA:
        use_runtime(cg, RT_LE_TEXTO);
        value = new_temp(cg, kind);
        strbuf_appendf(&cg->body, "  %s = call ptr @zl_le_texto(i32 %zu)\n",
                       value.text, line);
        return value;
    default: {
        Value raw;

        use_runtime(cg, RT_LE_BOOL);
        raw = new_temp(cg, AST_TYPE_INTEIRA);
        strbuf_appendf(&cg->body, "  %s = call i32 @zl_le_bool(i32 %zu)\n",
                       raw.text, line);
        value = new_temp(cg, AST_TYPE_BOOL);
        strbuf_appendf(&cg->body, "  %s = icmp ne i32 %s, 0\n", value.text,
                       raw.text);
        return value;
    }
    }
}

static void emit_read(Codegen *cg, const AstLValue *target, size_t line)
{
    Address address;
    Value value;

    if (!lvalue_address(cg, target, &address)) {
        return;
    }
    value = emit_read_value(cg, address.type.kind, line);
    strbuf_appendf(&cg->body, "  store %s %s, ptr %s\n",
                   scalar_type(address.type.kind), value.text, address.text);
}

/* Valor de saida do processo: respost da principal convertido (D42). */
static Value exit_code(Codegen *cg, Value value)
{
    Value result;

    switch (value.kind) {
    case AST_TYPE_INTEIRA:
        return value;
    case AST_TYPE_BOOL:
        return widen_bool(cg, value);
    case AST_TYPE_FLUT:
    case AST_TYPE_DUPLOCARPADO:
        result = new_temp(cg, AST_TYPE_INTEIRA);
        strbuf_appendf(&cg->body, "  %s = fptosi %s %s to i32\n", result.text,
                       scalar_type(value.kind), value.text);
        return result;
    default:
        return constant_value("0", AST_TYPE_INTEIRA);
    }
}

static void emit_return(Codegen *cg, const AstReturn *statement)
{
    const AstFunctionDecl *function = cg->function;
    Value value = emit_expression(cg, statement->value);

    ensure_block(cg);
    if (function->is_main) {
        if (function->return_type.kind != AST_TYPE_VAZIO) {
            value = convert(cg, value, function->return_type.kind);
        }
        value = exit_code(cg, value);
        strbuf_appendf(&cg->body, "  ret i32 %s\n", value.text);
    } else {
        value = convert(cg, value, function->return_type.kind);
        strbuf_appendf(&cg->body, "  ret %s %s\n",
                       scalar_type(function->return_type.kind), value.text);
    }
    cg->terminated = true;
}

/*
 * cond (c0) edai [B0] casocontrario cond (c1) edai [B1] ...
 * casocontrario edai [S]
 * vira teste_k -> entao_k / teste_{k+1}, cada ramo saltando para fim.
 */
static void emit_if(Codegen *cg, const AstIf *statement)
{
    size_t id = new_label(cg);
    size_t branches = statement->else_if_count + 1;
    char end_label[64];
    size_t k;

    snprintf(end_label, sizeof(end_label), "cond_fim_%zu", id);
    for (k = 0; k < branches; k++) {
        const AstExpression *condition =
            k == 0 ? statement->condition
                   : statement->else_if_branches[k - 1]->condition;
        const AstBlock *block =
            k == 0 ? statement->then_block
                   : statement->else_if_branches[k - 1]->block;
        char then_label[64];
        char next_label[64];
        Value value;

        snprintf(then_label, sizeof(then_label), "entao_%zu_%zu", id, k);
        if (k + 1 < branches) {
            snprintf(next_label, sizeof(next_label), "senao_cond_%zu_%zu", id,
                     k + 1);
        } else if (statement->else_block != NULL) {
            snprintf(next_label, sizeof(next_label), "senao_%zu", id);
        } else {
            snprintf(next_label, sizeof(next_label), "%s", end_label);
        }
        value = emit_expression(cg, condition);
        conditional_branch(cg, value.text, then_label, next_label);
        begin_block(cg, then_label);
        emit_block(cg, block);
        branch(cg, end_label);
        if (k + 1 < branches) {
            begin_block(cg, next_label);
        }
    }
    if (statement->else_block != NULL) {
        char else_label[64];

        snprintf(else_label, sizeof(else_label), "senao_%zu", id);
        begin_block(cg, else_label);
        emit_block(cg, statement->else_block);
        branch(cg, end_label);
    }
    begin_block(cg, end_label);
}

/* durante: teste no cabecalho, corpo, volta ao teste, fim. */
static void emit_while(Codegen *cg, const AstWhile *statement)
{
    size_t id = new_label(cg);
    char test[64];
    char body[64];
    char end[64];
    Value value;

    snprintf(test, sizeof(test), "durante_teste_%zu", id);
    snprintf(body, sizeof(body), "durante_corpo_%zu", id);
    snprintf(end, sizeof(end), "durante_fim_%zu", id);
    begin_block(cg, test);
    value = emit_expression(cg, statement->condition);
    conditional_branch(cg, value.text, body, end);
    begin_block(cg, body);
    emit_block(cg, statement->body);
    branch(cg, test);
    begin_block(cg, end);
}

/* repete: inicializacao; teste; corpo; incremento; volta ao teste. */
static void emit_repeat(Codegen *cg, const AstRepeat *statement)
{
    size_t id = new_label(cg);
    char test[64];
    char body[64];
    char step[64];
    char end[64];
    Value value;
    LoopRange range;
    bool tracked = cg->range_count < MAX_LOOP_RANGES &&
                   repeat_range(cg, statement, &range);

    snprintf(test, sizeof(test), "repete_teste_%zu", id);
    snprintf(body, sizeof(body), "repete_corpo_%zu", id);
    snprintf(step, sizeof(step), "repete_passo_%zu", id);
    snprintf(end, sizeof(end), "repete_fim_%zu", id);
    emit_assignment(cg, statement->initialization);
    begin_block(cg, test);
    value = emit_expression(cg, statement->condition);
    conditional_branch(cg, value.text, body, end);
    begin_block(cg, body);
    if (tracked) {
        cg->ranges[cg->range_count++] = range;
    }
    emit_block(cg, statement->body);
    if (tracked) {
        cg->range_count--;
    }
    begin_block(cg, step);
    emit_assignment(cg, statement->update);
    branch(cg, test);
    begin_block(cg, end);
}

static void emit_statement(Codegen *cg, const AstStatement *statement)
{
    if (cg->failed) {
        return;
    }
    ensure_block(cg);
    strbuf_appendf(&cg->body, "  ; linha %zu\n", statement->location.line);
    switch (statement->kind) {
    case AST_STMT_DECLARATION:
        emit_local_declaration(cg, statement->as.declaration);
        break;
    case AST_STMT_ASSIGNMENT:
        emit_assignment(cg, statement->as.assignment);
        break;
    case AST_STMT_IF:
        emit_if(cg, statement->as.if_statement);
        break;
    case AST_STMT_WHILE:
        emit_while(cg, statement->as.while_statement);
        break;
    case AST_STMT_REPEAT:
        emit_repeat(cg, statement->as.repeat_statement);
        break;
    case AST_STMT_RETURN:
        emit_return(cg, statement->as.return_statement);
        break;
    case AST_STMT_BLOCK:
        emit_block(cg, statement->as.block);
        break;
    case AST_STMT_IO:
        if (statement->as.io->kind == AST_IO_READ) {
            emit_read(cg, statement->as.io->as.target,
                      statement->location.line);
        } else {
            emit_print(cg, statement->as.io->as.value);
        }
        break;
    case AST_STMT_CALL:
        (void)emit_call(cg, statement->as.call);
        break;
    default:
        cg->failed = true;
        break;
    }
}

static void emit_block(Codegen *cg, const AstBlock *block)
{
    size_t i;

    if (block == NULL) {
        cg->failed = true;
        return;
    }
    for (i = 0; i < block->statement_count && !cg->failed; i++) {
        emit_statement(cg, block->statements[i]);
    }
}

/* ------------------------------------------------------------------ */
/* 8. Funcoes, globais e modulo                                        */
/* ------------------------------------------------------------------ */

static void emit_function(Codegen *cg, const AstFunctionDecl *function)
{
    char symbol[112];
    size_t i;

    cg->function = function;
    cg->temp = 0;
    cg->terminated = false;
    cg->local_count = 0;
    strbuf_clear(&cg->entry);
    strbuf_clear(&cg->body);
    snprintf(cg->current_label, sizeof(cg->current_label), "entry");
    function_symbol(symbol, sizeof(symbol), function);

    for (i = 0; i < function->parameter_count; i++) {
        remember_local_name(cg, function->parameters[i]->name);
    }

    strbuf_appendf(&cg->functions, "; linha %zu: %s%s\n",
                   function->location.line,
                   function->is_main ? "principal" : "outrafuncao ",
                   function->is_main ? "" : function->name);
    if (function->is_main) {
        strbuf_appendf(&cg->functions, "define i32 @%s() nounwind {\n",
                       symbol);
    } else {
        strbuf_appendf(&cg->functions, "define internal %s @%s(",
                       scalar_type(function->return_type.kind), symbol);
        for (i = 0; i < function->parameter_count; i++) {
            strbuf_appendf(&cg->functions, "%s%s %%%s", i > 0 ? ", " : "",
                           scalar_type(function->parameters[i]->type.kind),
                           function->parameters[i]->name);
        }
        strbuf_append(&cg->functions, ") nounwind {\n");
    }

    /* Parametros: alloca + store do argumento (forma em memoria). */
    for (i = 0; i < function->parameter_count; i++) {
        const AstParameter *parameter = function->parameters[i];
        VarInfo *var;

        if (parameter->symbol_id == 0 ||
            parameter->symbol_id >= cg->var_count) {
            cg->failed = true;
            return;
        }
        var = &cg->vars[parameter->symbol_id];
        var->defined = true;
        var->is_global = false;
        var->type = scalar_value_type(parameter->type.kind);
        unique_local_name(cg, parameter->name, ".addr", var->name,
                          sizeof(var->name));
        strbuf_appendf(&cg->entry, "  %%%s = alloca %s\n", var->name,
                       scalar_type(parameter->type.kind));
        if (function->is_main) {
            /* Parametros da principal sao lidos da entrada (D42). */
            Value value = emit_read_value(cg, parameter->type.kind,
                                          parameter->location.line);

            strbuf_appendf(&cg->body, "  store %s %s, ptr %%%s\n",
                           scalar_type(parameter->type.kind), value.text,
                           var->name);
        } else {
            strbuf_appendf(&cg->body, "  store %s %%%s, ptr %%%s\n",
                           scalar_type(parameter->type.kind), parameter->name,
                           var->name);
        }
    }

    emit_block(cg, function->body);

    if (!cg->terminated) {
        if (function->is_main) {
            strbuf_append(&cg->body, "  ret i32 0\n");
        } else if (function->return_type.kind == AST_TYPE_VAZIO) {
            strbuf_append(&cg->body, "  ret void\n");
        } else {
            /* Inalcancavel: a semantica exige respost em todo caminho. */
            strbuf_append(&cg->body, "  unreachable\n");
        }
    }

    strbuf_append(&cg->functions, "entry:\n");
    strbuf_append_buffer(&cg->functions, &cg->entry);
    strbuf_append_buffer(&cg->functions, &cg->body);
    strbuf_append(&cg->functions, "}\n\n");
}

static void emit_record_type(Codegen *cg, const AstRecordDecl *record)
{
    size_t i;

    strbuf_appendf(&cg->types, "; linha %zu: registro %s\n",
                   record->location.line, record->name);
    strbuf_appendf(&cg->types, "%%struct.%s = type { ", record->name);
    for (i = 0; i < record->field_count; i++) {
        strbuf_appendf(&cg->types, "%s%s", i > 0 ? ", " : "",
                       scalar_type(record->fields[i]->type.kind));
    }
    strbuf_append(&cg->types, record->field_count > 0 ? " }\n" : "}\n");
}

static void emit_global_declaration(Codegen *cg,
                                    const AstDeclaration *declaration)
{
    size_t i;

    for (i = 0; i < declaration->declarator_count; i++) {
        const AstDeclarator *declarator = declaration->declarators[i];
        VarInfo *var;
        char type_text[160];

        if (declarator->symbol_id == 0 ||
            declarator->symbol_id >= cg->var_count) {
            cg->failed = true;
            return;
        }
        var = &cg->vars[declarator->symbol_id];
        var->defined = true;
        var->is_global = true;
        var->type.kind = declaration->type.kind;
        var->type.vector_size = declarator->vector_size;
        var->type.record = declaration->record;
        var->type.valid = true;
        snprintf(var->name, sizeof(var->name), "%s", declarator->name);
        full_type(type_text, sizeof(type_text), &var->type);
        strbuf_appendf(&cg->globals,
                       "@%s = internal global %s %s  ; linha %zu\n",
                       var->name, type_text, zero_of(&var->type),
                       declarator->location.line);
    }
}

static void assemble_module(Codegen *cg, const char *source_name,
                            StrBuf *out)
{
    size_t i;

    strbuf_appendf(out, "; ModuleID = '%s'\n", source_name);
    strbuf_append(out,
                  "; Gerado pelo compilador Macaronica (PjBL 3).\n"
                  "; IR portavel: sem target triple nem datalayout; o alvo e\n"
                  "; escolhido na construcao (clang --target / llc -mtriple).\n"
                  "; Erros de execucao: 2 indice, 3 divisao por zero,\n"
                  "; 4 raiz de negativo, 5 expoente negativo (inteira).\n");
    strbuf_appendf(out, "source_filename = \"%s\"\n\n", source_name);
    if (cg->types.length > 0) {
        strbuf_append_buffer(out, &cg->types);
        strbuf_append(out, "\n");
    }
    if (cg->globals.length > 0) {
        strbuf_append_buffer(out, &cg->globals);
        strbuf_append(out, "\n");
    }
    if (cg->strings.length > 0) {
        strbuf_append_buffer(out, &cg->strings);
        strbuf_append(out, "\n");
    }
    strbuf_append(out, "; Biblioteca de execucao (runtime/zl_runtime.h)\n");
    for (i = 0; i < RT_COUNT; i++) {
        if (cg->runtime_used[i]) {
            strbuf_appendf(out, "%s\n", RUNTIME_DECLARATIONS[i]);
        }
    }
    if (cg->declarations.length > 0) {
        strbuf_append(out, "; Nucleos da DSL vetorial (gerados em MLIR)\n");
        strbuf_append_buffer(out, &cg->declarations);
    }
    strbuf_append(out, "\n");
    strbuf_append_buffer(out, &cg->functions);
}

bool codegen_llvm_generate(
    const AstProgram *program,
    size_t symbol_count,
    const char *source_name,
    bool fastmath,
    CodegenResult *result
)
{
    Codegen cg = {0};
    size_t i;

    if (program == NULL || result == NULL) {
        return false;
    }
    *result = (CodegenResult){0};
    cg.program = program;
    cg.mlir.fastmath = fastmath;
    cg.var_count = symbol_count + 1;
    cg.vars = calloc(cg.var_count, sizeof(*cg.vars));
    if (cg.vars == NULL) {
        return false;
    }

    /* Tipos e globais primeiro: funcoes podem usar globais posteriores. */
    for (i = 0; i < program->count; i++) {
        const AstTopLevelItem *item = program->items[i];

        if (item->kind == AST_TOP_RECORD) {
            emit_record_type(&cg, item->as.record);
        } else if (item->kind == AST_TOP_DECLARATION) {
            emit_global_declaration(&cg, item->as.declaration);
        }
    }
    for (i = 0; i < program->count && !cg.failed; i++) {
        if (program->items[i]->kind == AST_TOP_FUNCTION) {
            emit_function(&cg, program->items[i]->as.function);
        }
    }

    {
        /* So o nome do arquivo: o IR nao depende de onde ele estava. */
        const char *base = source_name;
        const char *p;

        for (p = source_name; *p != '\0'; p++) {
            if (*p == '/' || *p == '\\') {
                base = p + 1;
            }
        }
        if (cg.mlir.kernel_count > 0) {
            strbuf_appendf(
                &result->mlir,
                "// Nucleos da DSL vetorial de %s (PjBL 4, Parte B).\n"
                "// Rebaixados ate LLVM IR por scripts/pipeline.sh.\n\n",
                base
            );
            strbuf_append_buffer(&result->mlir, &cg.mlir.text);
        }
        assemble_module(&cg, base, &result->ir);
    }
    result->kernel_count = cg.mlir.kernel_count;
    result->ok = !cg.failed && !cg.mlir.failed &&
                 !strbuf_failed(&result->ir) && !result->mlir.failed &&
                 !cg.types.failed && !cg.globals.failed &&
                 !cg.strings.failed && !cg.declarations.failed &&
                 !cg.functions.failed && !cg.entry.failed && !cg.body.failed;

    strbuf_destroy(&cg.types);
    strbuf_destroy(&cg.globals);
    strbuf_destroy(&cg.strings);
    strbuf_destroy(&cg.declarations);
    strbuf_destroy(&cg.functions);
    strbuf_destroy(&cg.entry);
    strbuf_destroy(&cg.body);
    mlir_module_destroy(&cg.mlir);
    free(cg.local_names);
    free(cg.vars);
    return result->ok;
}

void codegen_result_destroy(CodegenResult *result)
{
    if (result != NULL) {
        strbuf_destroy(&result->ir);
        strbuf_destroy(&result->mlir);
        *result = (CodegenResult){0};
    }
}
