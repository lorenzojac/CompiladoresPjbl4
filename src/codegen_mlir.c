/*
 * codegen_mlir.c - nucleos da DSL vetorial em MLIR (ver codegen_mlir.h).
 *
 * Cada nucleo percorre os N elementos com um unico affine.for (o laco e
 * "fundido": c receba a + 3 * b faz uma passada so, sem vetores
 * temporarios). Folhas da expressao:
 *   - referencia a vetor do programa  -> parametro memref<N x T>,
 *     carregado com affine.load %v[%i] no corpo do laco;
 *   - literal numerico                -> arith.constant dentro do nucleo;
 *   - qualquer outra expressao escalar-> parametro escalar (avaliado uma
 *     unica vez pelo LLVM IR antes da chamada).
 * As conversoes de tipo seguem a promocao da linguagem (D17): sitofp para
 * inteira -> flut/duplocarpado e extf para flut -> duplocarpado.
 *
 * Reducoes usam affine.for com iter_args (acumulador no laco), forma que o
 * affine-super-vectorize reconhece com vectorize-reductions=true.
 */
#include "codegen_mlir.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Estado durante a geracao do corpo de um nucleo. */
typedef struct {
    MlirModule *module;
    MlirKernel *kernel;
    StrBuf *out;
    size_t temp;
    /* Nome do valor carregado de cada folha vetorial na iteracao atual. */
    char (*loaded)[48];
    const char *indent;
} KernelBuilder;

const char *mlir_element_type(AstTypeKind kind)
{
    switch (kind) {
    case AST_TYPE_INTEIRA:
        return "i32";
    case AST_TYPE_FLUT:
        return "f32";
    case AST_TYPE_DUPLOCARPADO:
        return "f64";
    case AST_TYPE_BOOL:
        return "i1";
    default:
        return "i32";
    }
}

static bool is_float_kind(AstTypeKind kind)
{
    return kind == AST_TYPE_FLUT || kind == AST_TYPE_DUPLOCARPADO;
}

/*
 * Com --fastmath (estudo de otimizacao, Parte C) as operacoes de ponto
 * flutuante dos nucleos levam fastmath<fast>, que vira "fadd fast" no LLVM
 * IR e autoriza reassociar a soma. E a unica forma de dar essa permissao:
 * o -ffast-math do clang nao altera instrucoes de um .ll ja existente.
 */
static const char *fastmath_suffix(const MlirModule *module, bool real)
{
    return (module->fastmath && real) ? " fastmath<fast>" : "";
}

/* ------------------------------------------------------------------ */
/* Folhas                                                              */
/* ------------------------------------------------------------------ */

static bool append_leaf(MlirKernel *kernel, MlirLeaf leaf)
{
    if (kernel->leaf_count == kernel->leaf_capacity) {
        size_t capacity = kernel->leaf_capacity == 0 ? 4
                                                     : kernel->leaf_capacity * 2;
        MlirLeaf *grown = realloc(kernel->leaves, capacity * sizeof(*grown));

        if (grown == NULL) {
            return false;
        }
        kernel->leaves = grown;
        kernel->leaf_capacity = capacity;
    }
    kernel->leaves[kernel->leaf_count++] = leaf;
    return true;
}

static bool is_vector_reference(const AstExpression *expression)
{
    return expression->kind == AST_EXPR_REFERENCE &&
           expression->as.reference.kind == AST_REFERENCE_VARIABLE &&
           expression->type.vector_size > 0;
}

static bool is_numeric_literal(const AstExpression *expression)
{
    return expression->kind == AST_EXPR_LITERAL &&
           (expression->as.literal.kind == AST_LITERAL_INTEGER ||
            expression->as.literal.kind == AST_LITERAL_DECIMAL);
}

static bool add_vector_leaf(
    MlirKernel *kernel,
    size_t symbol_id,
    AstTypeKind element,
    size_t size
)
{
    MlirLeaf leaf = {0};
    size_t i;

    for (i = 0; i < kernel->leaf_count; i++) {
        if (kernel->leaves[i].kind == MLIR_LEAF_VECTOR &&
            kernel->leaves[i].symbol_id == symbol_id) {
            return true;
        }
    }
    leaf.kind = MLIR_LEAF_VECTOR;
    leaf.symbol_id = symbol_id;
    leaf.element = element;
    leaf.size = size;
    return append_leaf(kernel, leaf);
}

/* Coleta as folhas de uma expressao (vetorial ou escalar), em pre-ordem. */
static bool collect_leaves(MlirKernel *kernel, const AstExpression *expression)
{
    if (expression->type.vector_size == 0) {
        MlirLeaf leaf = {0};

        if (is_numeric_literal(expression)) {
            return true;
        }
        leaf.kind = MLIR_LEAF_SCALAR;
        leaf.element = expression->type.kind;
        leaf.expression = expression;
        return append_leaf(kernel, leaf);
    }
    if (is_vector_reference(expression)) {
        return add_vector_leaf(
            kernel,
            expression->as.reference.symbol_id,
            expression->type.kind,
            expression->type.vector_size
        );
    }
    switch (expression->kind) {
    case AST_EXPR_UNARY:
        return collect_leaves(kernel, expression->as.unary.operand);
    case AST_EXPR_BINARY:
        return collect_leaves(kernel, expression->as.binary.left) &&
               collect_leaves(kernel, expression->as.binary.right);
    default:
        return false;
    }
}

static size_t find_vector_leaf(const MlirKernel *kernel, size_t symbol_id)
{
    size_t i;

    for (i = 0; i < kernel->leaf_count; i++) {
        if (kernel->leaves[i].kind == MLIR_LEAF_VECTOR &&
            kernel->leaves[i].symbol_id == symbol_id) {
            return i;
        }
    }
    return SIZE_MAX;
}

static size_t find_scalar_leaf(
    const MlirKernel *kernel,
    const AstExpression *expression
)
{
    size_t i;

    for (i = 0; i < kernel->leaf_count; i++) {
        if (kernel->leaves[i].kind == MLIR_LEAF_SCALAR &&
            kernel->leaves[i].expression == expression) {
            return i;
        }
    }
    return SIZE_MAX;
}

/* ------------------------------------------------------------------ */
/* Emissao do corpo                                                    */
/* ------------------------------------------------------------------ */

static void memref_type(StrBuf *out, const MlirLeaf *leaf)
{
    strbuf_appendf(
        out,
        "memref<%zux%s>",
        leaf->size,
        mlir_element_type(leaf->element)
    );
}

static void new_temp(KernelBuilder *builder, char *name, size_t size)
{
    snprintf(name, size, "%%t%zu", builder->temp++);
}

/* Constante de ponto flutuante exata, no formato hexadecimal do MLIR. */
static void float_constant(
    KernelBuilder *builder,
    const char *lexeme,
    AstTypeKind kind,
    char *name,
    size_t size
)
{
    new_temp(builder, name, size);
    if (kind == AST_TYPE_DUPLOCARPADO) {
        double value = strtod(lexeme, NULL);
        uint64_t bits;

        memcpy(&bits, &value, sizeof(bits));
        strbuf_appendf(
            builder->out,
            "%s%s = arith.constant 0x%016llX : f64\n",
            builder->indent,
            name,
            (unsigned long long)bits
        );
    } else {
        float value = (float)strtod(lexeme, NULL);
        uint32_t bits;

        memcpy(&bits, &value, sizeof(bits));
        strbuf_appendf(
            builder->out,
            "%s%s = arith.constant 0x%08lX : f32\n",
            builder->indent,
            name,
            (unsigned long)bits
        );
    }
}

/*
 * Emite o valor de um literal ja no tipo pedido: um literal decimal
 * promovido a duplocarpado e convertido a partir do texto (exato, D38).
 */
static void literal_in_kind(
    KernelBuilder *builder,
    const AstExpression *literal,
    AstTypeKind kind,
    char *name,
    size_t size
)
{
    if (literal->as.literal.kind == AST_LITERAL_INTEGER) {
        long long inteiro;
        char texto[24];

        /* Literal maior que i32: valor modulo 2^32, como no LLVM IR (D50);
         * promovido a real, converte esse mesmo valor (igual ao sitofp). */
        (void)ast_integer_literal_i32(literal, &inteiro);
        if (kind == AST_TYPE_INTEIRA) {
            new_temp(builder, name, size);
            strbuf_appendf(
                builder->out,
                "%s%s = arith.constant %lld : i32\n",
                builder->indent,
                name,
                inteiro
            );
            return;
        }
        snprintf(texto, sizeof(texto), "%lld", inteiro);
        float_constant(builder, texto, kind, name, size);
        return;
    }
    float_constant(builder, literal->as.literal.lexeme, kind, name, size);
}

/* Converte "value" (tipo from) para o tipo to, com a promocao da D17. */
static void convert(
    KernelBuilder *builder,
    char *value,
    size_t size,
    AstTypeKind from,
    AstTypeKind to
)
{
    char result[48];

    if (from == to) {
        return;
    }
    new_temp(builder, result, sizeof(result));
    if (from == AST_TYPE_INTEIRA) {
        strbuf_appendf(
            builder->out,
            "%s%s = arith.sitofp %s : i32 to %s\n",
            builder->indent,
            result,
            value,
            mlir_element_type(to)
        );
    } else {
        strbuf_appendf(
            builder->out,
            "%s%s = arith.extf %s : f32 to f64\n",
            builder->indent,
            result,
            value
        );
    }
    snprintf(value, size, "%s", result);
}

/*
 * Valor do elemento i de uma expressao, no tipo "wanted". Literais sao
 * emitidos diretamente no tipo desejado.
 */
static void emit_element(
    KernelBuilder *builder,
    const AstExpression *expression,
    AstTypeKind wanted,
    char *name,
    size_t size
)
{
    AstTypeKind kind = expression->type.kind;

    if (is_numeric_literal(expression)) {
        literal_in_kind(builder, expression, wanted, name, size);
        return;
    }
    if (expression->type.vector_size == 0) {
        size_t leaf = find_scalar_leaf(builder->kernel, expression);

        snprintf(name, size, "%%k%zu", leaf);
        convert(builder, name, size, kind, wanted);
        return;
    }
    if (is_vector_reference(expression)) {
        size_t leaf = find_vector_leaf(
            builder->kernel,
            expression->as.reference.symbol_id
        );

        snprintf(name, size, "%s", builder->loaded[leaf]);
        convert(builder, name, size, kind, wanted);
        return;
    }

    if (expression->kind == AST_EXPR_UNARY) {
        char operand[48];

        emit_element(builder, expression->as.unary.operand, kind, operand,
                     sizeof(operand));
        if (expression->as.unary.op == AST_UNARY_MINUS) {
            if (is_float_kind(kind)) {
                new_temp(builder, name, size);
                strbuf_appendf(
                    builder->out,
                    "%s%s = arith.negf %s : %s\n",
                    builder->indent,
                    name,
                    operand,
                    mlir_element_type(kind)
                );
            } else {
                char zero[48];

                new_temp(builder, zero, sizeof(zero));
                strbuf_appendf(
                    builder->out,
                    "%s%s = arith.constant 0 : i32\n",
                    builder->indent,
                    zero
                );
                new_temp(builder, name, size);
                strbuf_appendf(
                    builder->out,
                    "%s%s = arith.subi %s, %s : i32\n",
                    builder->indent,
                    name,
                    zero,
                    operand
                );
            }
        } else {
            snprintf(name, size, "%s", operand);
        }
        convert(builder, name, size, kind, wanted);
        return;
    }

    {
        char left[48];
        char right[48];
        const char *op;
        bool real = is_float_kind(kind);

        emit_element(builder, expression->as.binary.left, kind, left,
                     sizeof(left));
        emit_element(builder, expression->as.binary.right, kind, right,
                     sizeof(right));
        switch (expression->as.binary.op) {
        case AST_BINARY_ADD:
            op = real ? "arith.addf" : "arith.addi";
            break;
        case AST_BINARY_SUBTRACT:
            op = real ? "arith.subf" : "arith.subi";
            break;
        default:
            op = real ? "arith.mulf" : "arith.muli";
            break;
        }
        new_temp(builder, name, size);
        strbuf_appendf(
            builder->out,
            "%s%s = %s %s, %s%s : %s\n",
            builder->indent,
            name,
            op,
            left,
            right,
            fastmath_suffix(builder->module, real),
            mlir_element_type(kind)
        );
        convert(builder, name, size, kind, wanted);
    }
}

/* Carrega o elemento "index" de cada folha vetorial usada como operando. */
static void load_operands(
    KernelBuilder *builder,
    const char *index,
    const char *suffix,
    size_t skip_leaf
)
{
    size_t i;

    for (i = 0; i < builder->kernel->leaf_count; i++) {
        const MlirLeaf *leaf = &builder->kernel->leaves[i];

        if (leaf->kind != MLIR_LEAF_VECTOR || i == skip_leaf) {
            continue;
        }
        snprintf(builder->loaded[i], sizeof(builder->loaded[i]),
                 "%%x%zu%s", i, suffix);
        strbuf_appendf(
            builder->out,
            "%s%s = affine.load %%v%zu[%s] : ",
            builder->indent,
            builder->loaded[i],
            i,
            index
        );
        memref_type(builder->out, leaf);
        strbuf_append(builder->out, "\n");
    }
}

/* Assinatura: %v<i> para vetores e %k<i> para escalares, na ordem. */
static void emit_signature(
    StrBuf *out,
    const MlirKernel *kernel,
    SourceLocation location,
    const char *description
)
{
    size_t i;

    strbuf_appendf(out, "// linha %zu: %s\n", location.line, description);
    strbuf_appendf(out, "func.func @%s(", kernel->name);
    for (i = 0; i < kernel->leaf_count; i++) {
        const MlirLeaf *leaf = &kernel->leaves[i];

        if (i > 0) {
            strbuf_append(out, ", ");
        }
        if (leaf->kind == MLIR_LEAF_VECTOR) {
            strbuf_appendf(out, "%%v%zu: ", i);
            memref_type(out, leaf);
        } else {
            strbuf_appendf(out, "%%k%zu: %s", i,
                           mlir_element_type(leaf->element));
        }
    }
    strbuf_append(out, ")");
    if (kernel->result != AST_TYPE_VAZIO) {
        strbuf_appendf(out, " -> %s", mlir_element_type(kernel->result));
    }
    strbuf_append(out, " attributes {llvm.passthrough = [\"nounwind\"]} {\n");
}

static MlirKernel *new_kernel(MlirModule *module, AstTypeKind result)
{
    MlirKernel *kernel;

    if (module->kernel_count == module->kernel_capacity) {
        size_t capacity = module->kernel_capacity == 0
                              ? 4
                              : module->kernel_capacity * 2;
        MlirKernel **grown = realloc(
            module->kernels,
            capacity * sizeof(*grown)
        );

        if (grown == NULL) {
            module->failed = true;
            return NULL;
        }
        module->kernels = grown;
        module->kernel_capacity = capacity;
    }
    kernel = calloc(1, sizeof(*kernel));
    if (kernel == NULL) {
        module->failed = true;
        return NULL;
    }
    kernel->id = module->kernel_count + 1;
    kernel->result = result;
    snprintf(kernel->name, sizeof(kernel->name), "zl_nucleo_%zu", kernel->id);
    module->kernels[module->kernel_count++] = kernel;
    return kernel;
}

static bool begin_builder(KernelBuilder *builder, MlirModule *module,
                          MlirKernel *kernel)
{
    builder->module = module;
    builder->kernel = kernel;
    builder->out = &module->text;
    builder->temp = 0;
    builder->loaded = calloc(kernel->leaf_count + 1, sizeof(*builder->loaded));
    builder->indent = "    ";
    if (builder->loaded == NULL) {
        module->failed = true;
        return false;
    }
    return true;
}

const MlirKernel *mlir_add_assignment(
    MlirModule *module,
    size_t destination_id,
    AstTypeKind destination_element,
    size_t size,
    const AstExpression *value,
    SourceLocation location
)
{
    MlirKernel *kernel = new_kernel(module, AST_TYPE_VAZIO);
    KernelBuilder builder;
    size_t destination;
    char result[48];
    bool destination_is_operand;

    if (kernel == NULL) {
        return NULL;
    }
    if (!collect_leaves(kernel, value)) {
        module->failed = true;
        return NULL;
    }
    destination_is_operand =
        find_vector_leaf(kernel, destination_id) != SIZE_MAX;
    if (!add_vector_leaf(kernel, destination_id, destination_element, size)) {
        module->failed = true;
        return NULL;
    }
    destination = find_vector_leaf(kernel, destination_id);
    if (!begin_builder(&builder, module, kernel)) {
        return NULL;
    }

    emit_signature(&module->text, kernel, location,
                   value->type.vector_size > 0
                       ? "atribuicao vetorial elemento a elemento"
                       : "preenchimento do vetor com escalar");
    strbuf_appendf(&module->text, "  affine.for %%i = 0 to %zu {\n", size);
    load_operands(&builder, "%i", "", destination_is_operand
                                          ? SIZE_MAX
                                          : destination);
    emit_element(&builder, value, destination_element, result, sizeof(result));
    strbuf_appendf(&module->text, "    affine.store %s, %%v%zu[%%i] : ",
                   result, destination);
    memref_type(&module->text, &kernel->leaves[destination]);
    strbuf_append(&module->text, "\n  }\n  return\n}\n\n");
    free(builder.loaded);
    return kernel;
}

const MlirKernel *mlir_add_reduction(
    MlirModule *module,
    AstBuiltin builtin,
    const AstExpression *first,
    const AstExpression *second,
    AstTypeKind result,
    SourceLocation location
)
{
    MlirKernel *kernel = new_kernel(module, result);
    KernelBuilder builder;
    const char *type = mlir_element_type(result);
    bool real = is_float_kind(result);
    size_t size = first->type.vector_size;
    char element[48];
    char init[48];
    const char *combine;
    const char *description;
    size_t start = 0;

    if (kernel == NULL) {
        return NULL;
    }
    if (!collect_leaves(kernel, first) ||
        (second != NULL && !collect_leaves(kernel, second))) {
        module->failed = true;
        return NULL;
    }
    if (!begin_builder(&builder, module, kernel)) {
        return NULL;
    }

    switch (builtin) {
    case AST_BUILTIN_MAXVET:
        combine = real ? "arith.maxnumf" : "arith.maxsi";
        description = "reducao maxvet";
        break;
    case AST_BUILTIN_MINVET:
        combine = real ? "arith.minnumf" : "arith.minsi";
        description = "reducao minvet";
        break;
    case AST_BUILTIN_PRODESC:
        combine = real ? "arith.addf" : "arith.addi";
        description = "produto escalar prodesc";
        break;
    default:
        combine = real ? "arith.addf" : "arith.addi";
        description = "reducao somavet";
        break;
    }
    emit_signature(&module->text, kernel, location, description);

    /*
     * Soma e produto escalar comecam do zero. Maximo e minimo comecam do
     * elemento 0 (todo vetor tem tamanho >= 1, D28), e o laco vai de 1 a N.
     */
    builder.indent = "  ";
    if (builtin == AST_BUILTIN_MAXVET || builtin == AST_BUILTIN_MINVET) {
        load_operands(&builder, "0", "_0", SIZE_MAX);
        emit_element(&builder, first, result, init, sizeof(init));
        start = 1;
    } else {
        snprintf(init, sizeof(init), "%%zero");
        strbuf_appendf(&module->text, "  %%zero = arith.constant %s : %s\n",
                       real ? "0.000000e+00" : "0", type);
    }
    strbuf_appendf(
        &module->text,
        "  %%r = affine.for %%i = %zu to %zu iter_args(%%acc = %s) -> (%s) {\n",
        start,
        size,
        init,
        type
    );
    builder.indent = "    ";
    load_operands(&builder, "%i", "", SIZE_MAX);
    if (builtin == AST_BUILTIN_PRODESC) {
        char a[48];
        char b[48];

        emit_element(&builder, first, result, a, sizeof(a));
        emit_element(&builder, second, result, b, sizeof(b));
        new_temp(&builder, element, sizeof(element));
        strbuf_appendf(&module->text, "    %s = %s %s, %s%s : %s\n", element,
                       real ? "arith.mulf" : "arith.muli", a, b,
                       fastmath_suffix(module, real), type);
    } else {
        emit_element(&builder, first, result, element, sizeof(element));
    }
    if (builtin == AST_BUILTIN_MAXVET || builtin == AST_BUILTIN_MINVET) {
        /*
         * Maximo/minimo como comparacao + select. As operacoes dedicadas
         * (arith.maxsi, arith.maxnumf) fazem o affine-parallelize do MLIR
         * 23 gerar um affine.parallel invalido ("result type cannot match
         * reduction attribute"); com select o laco continua sequencial no
         * C3, mas todas as configuracoes funcionam, e o LLVM ainda reconhece
         * o padrao (smax/maxnum) no -O2.
         */
        bool is_max = builtin == AST_BUILTIN_MAXVET;

        strbuf_appendf(&module->text, "    %%c = %s %s, %s, %%acc : %s\n",
                       real ? "arith.cmpf" : "arith.cmpi",
                       real ? (is_max ? "ogt" : "olt")
                            : (is_max ? "sgt" : "slt"),
                       element, type);
        strbuf_appendf(&module->text,
                       "    %%n = arith.select %%c, %s, %%acc : %s\n",
                       element, type);
        (void)combine;
    } else {
        strbuf_appendf(&module->text, "    %%n = %s %%acc, %s%s : %s\n",
                       combine, element, fastmath_suffix(module, real), type);
    }
    strbuf_appendf(&module->text, "    affine.yield %%n : %s\n  }\n", type);
    strbuf_appendf(&module->text, "  return %%r : %s\n}\n\n", type);
    free(builder.loaded);
    return kernel;
}

void mlir_module_destroy(MlirModule *module)
{
    size_t i;

    if (module == NULL) {
        return;
    }
    for (i = 0; i < module->kernel_count; i++) {
        free(module->kernels[i]->leaves);
        free(module->kernels[i]);
    }
    free(module->kernels);
    strbuf_destroy(&module->text);
    *module = (MlirModule){0};
}
