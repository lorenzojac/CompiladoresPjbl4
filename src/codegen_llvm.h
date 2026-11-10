#ifndef MACARRONICA_CODEGEN_LLVM_H
#define MACARRONICA_CODEGEN_LLVM_H

/*
 * Gerador de LLVM IR textual (PjBL 3).
 *
 * Entrada: AST valida e ANOTADA pela analise semantica (tipos e symbol_id).
 * O gerador nunca reanalisa nomes ou tipos: le apenas as anotacoes, o que o
 * mantem independente das fases anteriores.
 *
 * Saida: um modulo LLVM IR portavel (docs/decisoes_backend.md, Secao 4.5 do
 * enunciado): sem target triple nem datalayout, tipos de largura fixa, sem
 * funcoes variadicas (so funcoes zl_* da biblioteca de execucao), enderecos
 * por getelementptr com indices i32, tudo nounwind e a principal como
 * @zl_principal. Operacoes da DSL vetorial geram tambem um modulo MLIR com
 * os nucleos (codegen_mlir.h).
 */

#include <stdbool.h>
#include <stddef.h>

#include "ast.h"
#include "strbuf.h"

typedef struct {
    StrBuf ir;            /* modulo LLVM IR */
    StrBuf mlir;          /* nucleos MLIR; vazio se nao ha DSL vetorial */
    size_t kernel_count;
    bool ok;
} CodegenResult;

/*
 * Gera o IR de um programa valido. symbol_count vem de SemanticResult.
 * source_name aparece no cabecalho do modulo (source_filename).
 * fastmath (--fastmath) autoriza reassociar as operacoes reais dos nucleos.
 */
bool codegen_llvm_generate(
    const AstProgram *program,
    size_t symbol_count,
    const char *source_name,
    bool fastmath,
    CodegenResult *result
);

void codegen_result_destroy(CodegenResult *result);

#endif
