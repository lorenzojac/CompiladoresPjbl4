#ifndef MACARRONICA_CODEGEN_MLIR_H
#define MACARRONICA_CODEGEN_MLIR_H

/*
 * Gerador dos nucleos da DSL vetorial em MLIR (PjBL 4, Parte B).
 *
 * Estrategia hibrida (docs/relatorio_otimizacao.md): o programa continua em
 * LLVM IR (codegen_llvm.c) e cada operacao vetorial vira uma funcao MLIR
 * independente ("nucleo"), escrita com os dialetos func, arith, memref e
 * affine. O pipeline de scripts/pipeline.sh rebaixa os nucleos ate LLVM IR e
 * o llvm-link os une ao programa.
 *
 * Convencao de chamada: memrefs de forma estatica com o "bare pointer call
 * convention" (cada vetor vira um unico ptr), escalares por valor. O
 * gerador de LLVM IR consulta a lista de folhas (MlirLeaf) para montar a
 * chamada na mesma ordem dos parametros do nucleo.
 */

#include <stdbool.h>
#include <stddef.h>

#include "ast.h"
#include "strbuf.h"

typedef enum {
    MLIR_LEAF_VECTOR,  /* vetor do programa: parametro memref */
    MLIR_LEAF_SCALAR   /* subexpressao escalar avaliada antes: parametro */
} MlirLeafKind;

typedef struct {
    MlirLeafKind kind;
    size_t symbol_id;                 /* MLIR_LEAF_VECTOR */
    AstTypeKind element;              /* tipo do elemento ou do escalar */
    size_t size;                      /* MLIR_LEAF_VECTOR */
    const AstExpression *expression;  /* MLIR_LEAF_SCALAR */
} MlirLeaf;

typedef struct {
    size_t id;
    char name[40];          /* zl_nucleo_<id> */
    MlirLeaf *leaves;       /* parametros, na ordem */
    size_t leaf_count;
    size_t leaf_capacity;
    AstTypeKind result;     /* AST_TYPE_VAZIO para atribuicao vetorial */
} MlirKernel;

typedef struct {
    StrBuf text;
    MlirKernel **kernels;
    size_t kernel_count;
    size_t kernel_capacity;
    bool failed;
    /* --fastmath: operacoes reais com fastmath<fast> (permite reassociar). */
    bool fastmath;
} MlirModule;

/*
 * destino{i} <- valor{i} para todo i, com valor vetorial ou escalar
 * (preenchimento). O destino e uma folha vetorial (deduplicada se tambem
 * aparecer como operando).
 */
const MlirKernel *mlir_add_assignment(
    MlirModule *module,
    size_t destination_id,
    AstTypeKind destination_element,
    size_t size,
    const AstExpression *value,
    SourceLocation location
);

/* somavet/maxvet/minvet(a) ou prodesc(a, b); b == NULL nas tres primeiras. */
const MlirKernel *mlir_add_reduction(
    MlirModule *module,
    AstBuiltin builtin,
    const AstExpression *first,
    const AstExpression *second,
    AstTypeKind result,
    SourceLocation location
);

const char *mlir_element_type(AstTypeKind kind);
void mlir_module_destroy(MlirModule *module);

#endif
