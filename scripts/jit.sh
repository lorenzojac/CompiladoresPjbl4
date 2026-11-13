#!/bin/sh
# scripts/jit.sh - executa um programa com o executor JIT do MLIR.
#
# Uso: sh scripts/jit.sh programa.mac [C0|C2|C3] [dir_saida]
#      sh scripts/jit.sh programa.final.ll [C0|C2|C3] [dir_saida]  (ja construido;
#      a configuracao so decide se a libomp entra, no C3)
#
# 1. scripts/construir.sh gera programa.final.ll (programa em LLVM IR +
#    nucleos da DSL vetorial rebaixados pelo pipeline MLIR);
# 2. mlir-translate --import-llvm traz o modulo inteiro para o dialeto llvm
#    do MLIR (o atributo de data layout "dlti", que o mlir-runner nao
#    registra, e retirado da primeira linha);
# 3. um involucro @zl_jit_main chama @zl_principal e depois zl_jit_termina,
#    que esvazia a saida e encerra com o valor da principal;
# 4. mlir-runner compila no JIT e executa, ligando a biblioteca de execucao
#    compilada como biblioteca compartilhada (-DZL_SEM_MAIN).
# A saida e a do programa; o codigo de saida e o valor de respost da
# principal (ou 3 em erro de execucao), como no executavel do clang.
set -eu

PROGRAMA=$1
CONFIG=${2:-C0}
RAIZ=$(cd "$(dirname "$0")/.." && pwd)
SAIDA=${3:-$RAIZ/build/jit}
CLANG=${CLANG:-clang}
MLIR_TRANSLATE=${MLIR_TRANSLATE:-mlir-translate}
MLIR_RUNNER=${MLIR_RUNNER:-mlir-runner}
# Ate o LLVM 19 (o do Dockerfile) o executor se chamava mlir-cpu-runner.
if ! command -v "$MLIR_RUNNER" > /dev/null 2>&1 &&
    command -v "${MLIR_RUNNER%mlir-runner}mlir-cpu-runner" > /dev/null 2>&1; then
    MLIR_RUNNER=${MLIR_RUNNER%mlir-runner}mlir-cpu-runner
fi
# Nome sem a extensao (.mac do projeto ou .mcn dos testes do professor).
NOME=$(basename "$PROGRAMA")
NOME=${NOME%.*}

# A biblioteca compartilhada fica sempre em build/jit (uma para todos).
case "$(uname -s)" in
MINGW* | MSYS* | CYGWIN*)
    BIBLIOTECA=$RAIZ/build/jit/zl_runtime_jit.dll
    # o mlir-runner precisa do caminho completo (a libomp.dll fica ao lado
    # do clang) e, sendo programa nativo, no formato do Windows.
    LIBOMP_PADRAO=$(cygpath -m "$(dirname "$(command -v "$CLANG")")/libomp.dll")
    ;;
*)
    BIBLIOTECA=$RAIZ/build/jit/libzl_runtime_jit.so
    LIBOMP_PADRAO=$("$CLANG" -print-file-name=libomp.so)
    ;;
esac
# No C3 os nucleos chamam o runtime OpenMP (__kmpc_*).
BIBLIOTECAS=$BIBLIOTECA
if command -v cygpath > /dev/null 2>&1; then
    BIBLIOTECAS=$(cygpath -m "$BIBLIOTECA")
    # a lista separada por virgula nao deve passar pela conversao de
    # caminhos do MSYS (ja esta no formato do Windows)
    export MSYS2_ARG_CONV_EXCL=--shared-libs
fi
if [ "$CONFIG" = C3 ]; then
    BIBLIOTECAS=$BIBLIOTECAS,${LIBOMP:-$LIBOMP_PADRAO}
fi

mkdir -p "$SAIDA" "$RAIZ/build/jit"
if [ ! -f "$BIBLIOTECA" ] ||
    [ "$RAIZ/runtime/zl_runtime_posix.c" -nt "$BIBLIOTECA" ] ||
    [ "$RAIZ/runtime/zl_runtime_comum.c" -nt "$BIBLIOTECA" ]; then
    "$CLANG" -O1 -shared -fPIC -DZL_SEM_MAIN -I"$RAIZ/runtime" \
        "$RAIZ/runtime/zl_runtime_posix.c" -o "$BIBLIOTECA"
fi

# Aceita tambem um programa.final.ll ja construido (usado por make test).
case "$PROGRAMA" in
*.final.ll)
    NOME=$(basename "$PROGRAMA" .final.ll)
    FINAL=$PROGRAMA
    ;;
*)
    sh "$RAIZ/scripts/construir.sh" "$PROGRAMA" "$SAIDA" pc "$CONFIG" > /dev/null
    FINAL=$SAIDA/$NOME.final.ll
    ;;
esac
"$MLIR_TRANSLATE" --mlir-disable-threading --import-llvm \
    "$FINAL" -o "$SAIDA/$NOME.importado.mlir"

{
    sed -E '1s/^module attributes \{.*\} \{$/module {/; $d' \
        "$SAIDA/$NOME.importado.mlir"
    cat <<'FIM'
  llvm.func @zl_jit_termina(i32)
  llvm.func @zl_jit_main() {
    %0 = llvm.call @zl_principal() : () -> i32
    llvm.call @zl_jit_termina(%0) : (i32) -> ()
    llvm.return
  }
}
FIM
} > "$SAIDA/$NOME.jit.mlir"

# O mlir-runner 23.1.5 do MSYS2 cai as vezes (3 de 40 execucoes, medido) ao
# verificar o modulo: o verificador do MLIR roda em varias threads e o
# mlir-runner, ao contrario do mlir-opt, nao aceita --mlir-disable-threading.
# A queda acontece antes de o programa comecar (nada foi impresso nem lido),
# entao, se a ferramenta caiu sem imprimir nada, tentamos de novo (ate 5x).
SAIDA_JIT="$SAIDA/$NOME.jit.saida"
TENTATIVA=1
while :; do
    STATUS=0
    # Subshell com stderr redirecionado: o aviso "Segmentation fault" que o
    # shell imprime quando o processo cai vai para o .err, nao para a saida.
    # O "exit $?" impede o shell de trocar o subshell pelo proprio processo
    # (senao quem imprime o aviso e o shell de fora).
    ("$MLIR_RUNNER" "$SAIDA/$NOME.jit.mlir" -e zl_jit_main \
        --entry-point-result=void --shared-libs="$BIBLIOTECAS" \
        >"$SAIDA_JIT"
    exit $?) 2>"$SAIDA_JIT.err" || STATUS=$?
    if [ "$STATUS" -eq 0 ] || [ -s "$SAIDA_JIT" ] || [ "$TENTATIVA" -ge 5 ] ||
        ! grep -q "PLEASE submit a bug report" "$SAIDA_JIT.err"; then
        break
    fi
    # Registro fora da saida do programa (os testes comparam stdout+stderr).
    echo "mlir-runner caiu antes de executar; tentativa $((TENTATIVA + 1))" \
        >>"$SAIDA/$NOME.jit.tentativas"
    TENTATIVA=$((TENTATIVA + 1))
done
cat "$SAIDA_JIT"
cat "$SAIDA_JIT.err" >&2
exit "$STATUS"
