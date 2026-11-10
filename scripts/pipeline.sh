#!/bin/sh
# scripts/pipeline.sh - rebaixa os nucleos MLIR da DSL vetorial ate LLVM IR.
#
# Uso: sh scripts/pipeline.sh nucleos.mlir saida.ll [C0|C2|C3] [dir_niveis]
#
#   C0  linha de base: affine -> scf -> cf -> dialeto llvm, sem vetorizar
#   C2  vetorizacao explicita no MLIR (affine-super-vectorize, largura 8)
#   C3  paralelismo: affine-parallelize + scf -> OpenMP (ligar com -fopenmp)
#   (C1, "otimizacao generica", nao muda o MLIR: e o clang -O2 aplicado ao
#    IR final, ver scripts/construir.sh e scripts/bench/bench.py)
#
# Se dir_niveis for dado, salva o nucleo em cada nivel do pipeline
# (1-affine.mlir, 2-vector-ou-scf.mlir, 3-llvm.mlir) para o relatorio.
#
# Versao validada: MLIR 23.1.5 (MSYS2 UCRT64). --mlir-disable-threading
# contorna uma falha do verificador multithread do mlir-opt dessa build no
# Windows; nao muda o resultado e e inofensivo em outras plataformas.
set -eu

ENTRADA=$1
SAIDA=$2
CONFIG=${3:-C0}
NIVEIS=${4:-}
MLIR_OPT=${MLIR_OPT:-mlir-opt}
MLIR_TRANSLATE=${MLIR_TRANSLATE:-mlir-translate}
OPCOES="--mlir-disable-threading"

TMP=$(mktemp -d "${TMPDIR:-/tmp}/zlpipe.XXXXXX")
trap 'rm -rf "$TMP"' EXIT

# Passo 1 (so C2/C3): transformacoes no nivel affine, onde a analise de
# dependencias e exata (indices sao funcoes afins de %i).
case "$CONFIG" in
C0)
    cp "$ENTRADA" "$TMP/1.mlir"
    ;;
C2)
    # affine-super-vectorize: troca o laco escalar por um laco de passo 8
    # sobre vector<8xT> (affine.load -> vector.transfer_read). Com
    # vectorize-reductions=true, acumuladores iter_args tambem viram
    # vetores e um vector.reduction final junta as 8 parcelas.
    "$MLIR_OPT" $OPCOES "$ENTRADA" \
        --affine-super-vectorize="virtual-vector-size=8 vectorize-reductions=true" \
        -o "$TMP/1.mlir"
    ;;
C3)
    # affine-parallelize: troca affine.for sem dependencias entre iteracoes
    # por affine.parallel; parallel-reductions=true aceita reducoes
    # (soma, maximo) como reducoes paralelas. max-nested=1: so o laco externo.
    "$MLIR_OPT" $OPCOES "$ENTRADA" \
        --affine-parallelize="max-nested=1 parallel-reductions=true" \
        -o "$TMP/1.mlir"
    ;;
*)
    echo "pipeline.sh: configuracao invalida: $CONFIG (use C0, C2 ou C3)" >&2
    exit 2
    ;;
esac

# Passo 2: sai do dialeto affine.
#   --lower-affine            affine.for/load/store -> scf.for + memref.load/store
#   --convert-vector-to-scf   (C2) operacoes vector de alto nivel -> scf
#   --convert-scf-to-openmp   (C3) scf.parallel -> omp.parallel + omp.wsloop
#   --canonicalize            (C3) limpa o resultado; exigido desde o MLIR 18
case "$CONFIG" in
C0)
    "$MLIR_OPT" $OPCOES "$TMP/1.mlir" --lower-affine -o "$TMP/2.mlir"
    ;;
C2)
    "$MLIR_OPT" $OPCOES "$TMP/1.mlir" --lower-affine --convert-vector-to-scf \
        -o "$TMP/2.mlir"
    ;;
C3)
    "$MLIR_OPT" $OPCOES "$TMP/1.mlir" --lower-affine --convert-scf-to-openmp \
        --canonicalize -o "$TMP/2.mlir"
    ;;
esac

# Passo 3: tudo para o dialeto llvm.
#   --convert-scf-to-cf          lacos estruturados -> blocos e desvios (cf)
#   --convert-vector-to-llvm     vector<8xT> -> <8 x T> do LLVM
#   --convert-arith-to-llvm      arith.addi/mulf/... -> llvm.add/fmul/...
#   --finalize-memref-to-llvm    memref -> descritor/ponteiro + getelementptr
#   --convert-func-to-llvm=use-bare-ptr-memref-call-conv=1
#                                func.func -> llvm.func; cada memref de forma
#                                estatica vira um unico ptr (a mesma ABI que
#                                o codigo gerado pelo compilador usa)
#   --convert-cf-to-llvm         cf.br/cond_br -> llvm.br/cond_br
#   --convert-ub-to-llvm         ub.poison (preenchimento das leituras
#                                vetoriais mascaradas da ultima volta quando N
#                                nao e multiplo de 8) -> llvm.mlir.poison
#   --convert-openmp-to-llvm     (C3) omp.* -> chamadas ao runtime OpenMP
#   --reconcile-unrealized-casts remove conversoes de tipo temporarias
EXTRA=""
if [ "$CONFIG" = C3 ]; then
    EXTRA="--convert-openmp-to-llvm"
fi
"$MLIR_OPT" $OPCOES "$TMP/2.mlir" \
    --convert-scf-to-cf \
    --convert-vector-to-llvm \
    --convert-arith-to-llvm \
    --finalize-memref-to-llvm \
    --convert-func-to-llvm=use-bare-ptr-memref-call-conv=1 \
    --convert-cf-to-llvm \
    --convert-ub-to-llvm \
    $EXTRA \
    --reconcile-unrealized-casts \
    -o "$TMP/3.mlir"

# Passo 4: dialeto llvm -> LLVM IR textual (sem target triple: o alvo e
# escolhido depois, como no resto do programa).
"$MLIR_TRANSLATE" $OPCOES --mlir-to-llvmir "$TMP/3.mlir" -o "$SAIDA"

if [ -n "$NIVEIS" ]; then
    mkdir -p "$NIVEIS"
    cp "$TMP/1.mlir" "$NIVEIS/1-affine.mlir"
    cp "$TMP/2.mlir" "$NIVEIS/2-vector-ou-scf.mlir"
    cp "$TMP/3.mlir" "$NIVEIS/3-llvm.mlir"
fi
