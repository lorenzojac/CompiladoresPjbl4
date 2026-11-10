#!/bin/sh
# tests/run_execucao.sh - executa programas e compara com a saida esperada.
#
# Uso: sh tests/run_execucao.sh dir_testes [--gerar]
#
# Para cada dir_testes/nome.mac:
#   - compila com scripts/construir.sh (alvo pc, configuracao $CONFIG ou C0);
#   - executa com lli (programa + biblioteca) e com o executavel do clang,
#     usando dir_testes/nome.entrada como entrada padrao, se existir;
#   - a saida observada e stdout seguido da linha "[saida N]";
#   - as duas execucoes devem ser identicas a dir_testes/nome.esperado;
#   - com JIT=1, tambem executa com o executor JIT do MLIR (scripts/jit.sh:
#     mlir-translate --import-llvm + mlir-runner), que deve dar a mesma saida.
# Para cada dir_testes/invalidos/nome.mac (se existir):
#   - o compilador deve rejeitar (saida 1), nao gerar .ll, e o relatorio deve
#     conter o codigo de diagnostico escrito em nome.erro.
# --gerar cria .esperado ausentes a partir do executavel (revisar a mao!).
set -u

DIR=$1
GERAR=${2:-}
RAIZ=$(cd "$(dirname "$0")/.." && pwd)
MACARONICA=${MACARONICA:-$RAIZ/macaronica}
LLI=${LLI:-lli}
# Alvo usado para validar os programas invalidos (uno em tests/placa).
ALVO=${ALVO:-pc}
# Configuracao do pipeline MLIR (C0, C1, C2, C3); a saida deve ser a mesma.
CONFIG=${CONFIG:-C0}
JIT=${JIT:-0}
SAIDA=$RAIZ/build/testes/$(basename "$DIR")-$CONFIG
case "$(uname -s)" in
MINGW* | MSYS* | CYGWIN*) EXE=.exe; LIBOMP_PADRAO=libomp.dll ;;
*) EXE= ; LIBOMP_PADRAO=$(${CLANG:-clang} -print-file-name=libomp.so) ;;
esac
# No C3 os nucleos chamam o runtime OpenMP (__kmpc_*): o lli precisa carregar
# a libomp, que o executavel do clang ja recebe por -fopenmp.
LLI_OPCOES=
if [ "$CONFIG" = C3 ]; then
    LLI_OPCOES="--dlopen=${LIBOMP:-$LIBOMP_PADRAO}"
fi

passou=0
falhou=0

executa() {
    # $1 = comando ..., entrada em $ENTRADA; imprime saida + codigo.
    # No Windows o stdout em modo texto poe \r antes de cada \n; ele e
    # retirado para que os .esperado (sempre LF) valham em todo sistema.
    if [ -f "$ENTRADA" ]; then
        "$@" < "$ENTRADA" > "$SAIDA/.bruta" 2>&1
    else
        "$@" < /dev/null > "$SAIDA/.bruta" 2>&1
    fi
    codigo=$?
    tr -d '\r' < "$SAIDA/.bruta"
    echo "[saida $codigo]"
}

mkdir -p "$SAIDA"
for programa in "$DIR"/*.mac; do
    [ -f "$programa" ] || continue
    nome=$(basename "$programa" .mac)
    esperado="$DIR/$nome.esperado"
    ENTRADA="$DIR/$nome.entrada"

    if ! sh "$RAIZ/scripts/construir.sh" "$programa" "$SAIDA" pc "$CONFIG" \
        > "$SAIDA/$nome.compilacao" 2>&1; then
        falhou=$((falhou + 1))
        echo "FAIL $programa (nao compilou; ver $SAIDA/$nome.compilacao)"
        continue
    fi

    executa "$SAIDA/$nome$EXE" > "$SAIDA/$nome.nativo"
    # shellcheck disable=SC2086
    executa "$LLI" $LLI_OPCOES "$SAIDA/$nome.lli.bc" > "$SAIDA/$nome.lli"
    if [ "$JIT" = 1 ]; then
        executa sh "$RAIZ/scripts/jit.sh" "$SAIDA/$nome.final.ll" "$CONFIG"             "$SAIDA/jit" > "$SAIDA/$nome.jit"
    fi

    if [ ! -f "$esperado" ] && [ "$GERAR" = "--gerar" ]; then
        cp "$SAIDA/$nome.nativo" "$esperado"
        echo "GERADO $esperado"
    fi
    if [ -f "$esperado" ] &&
        diff "$esperado" "$SAIDA/$nome.nativo" > "$SAIDA/$nome.diff" 2>&1 &&
        diff "$esperado" "$SAIDA/$nome.lli" >> "$SAIDA/$nome.diff" 2>&1 &&
        { [ "$JIT" != 1 ] ||
            diff "$esperado" "$SAIDA/$nome.jit" >> "$SAIDA/$nome.diff" 2>&1; }; then
        passou=$((passou + 1))
        if [ "$JIT" = 1 ]; then
            echo "PASS $programa (clang, lli e JIT do MLIR)"
        else
            echo "PASS $programa"
        fi
    else
        falhou=$((falhou + 1))
        echo "FAIL $programa (ver $SAIDA/$nome.diff)"
        [ -f "$SAIDA/$nome.diff" ] && head -20 "$SAIDA/$nome.diff"
    fi
done

if [ -d "$DIR/invalidos" ]; then
    for programa in "$DIR"/invalidos/*.mac; do
        [ -f "$programa" ] || continue
        nome=$(basename "$programa" .mac)
        codigo=$(cat "$DIR/invalidos/$nome.erro" 2>/dev/null)
        rm -f "$SAIDA/$nome.invalido.ll"
        "$MACARONICA" "$programa" -o "$SAIDA/$nome.invalido.ll" --alvo="$ALVO" \
            > "$SAIDA/$nome.relatorio" 2>&1
        status=$?
        if [ "$status" -eq 1 ] && [ ! -f "$SAIDA/$nome.invalido.ll" ] &&
            grep -q "$codigo" "$SAIDA/$nome.relatorio"; then
            passou=$((passou + 1))
            echo "PASS $programa (rejeitado: $codigo)"
        else
            falhou=$((falhou + 1))
            echo "FAIL $programa (saida $status; esperado 1 com $codigo)"
        fi
    done
fi

echo "$(basename "$DIR") [$CONFIG]: $passou ok, $falhou falhas"
[ "$falhou" -eq 0 ]
