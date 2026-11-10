#!/bin/sh
# tests/run_memoria.sh - o compilador nao deixa memoria alocada.
#
# Uso: sh tests/run_memoria.sh
#
# Compila uma copia instrumentada do compilador (tests/contador_alocacoes.c,
# ld --wrap) e roda todos os programas de teste, validos e invalidos, nos
# alvos pc, uno e pico e sem -q (relatorio completo). Em cada execucao, todo bloco
# alocado dentro de main deve ter sido liberado (0 vivos no fim).
set -u

RAIZ=$(cd "$(dirname "$0")/.." && pwd)
CC=${CC:-cc}
SAIDA=$RAIZ/build/memoria
BIN=$SAIDA/macaronica_contado
mkdir -p "$SAIDA"

if ! "$CC" -std=c99 -O1 -I"$RAIZ/src" "$RAIZ"/src/token.c "$RAIZ"/src/lexer.c \
    "$RAIZ"/src/ast.c "$RAIZ"/src/parser.c "$RAIZ"/src/semantic.c \
    "$RAIZ"/src/strbuf.c "$RAIZ"/src/codegen_mlir.c "$RAIZ"/src/codegen_llvm.c \
    "$RAIZ"/src/main.c "$RAIZ/tests/contador_alocacoes.c" \
    -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free,--wrap=main \
    -o "$BIN" 2> "$SAIDA/compilacao.log"; then
    echo "memoria: ligador sem --wrap (nao e o ld do GNU); teste pulado"
    exit 0
fi

execucoes=0
falhas=0
for programa in "$RAIZ"/tests/validos/*.mac "$RAIZ"/tests/invalidos/*.mac \
    "$RAIZ"/tests/execucao/*.mac "$RAIZ"/tests/dsl/*.mac \
    "$RAIZ"/tests/dsl/invalidos/*.mac "$RAIZ"/tests/placa/*.mac \
    "$RAIZ"/tests/placa/invalidos/*.mac "$RAIZ"/tests/placa/manual/*.mac; do
    [ -f "$programa" ] || continue
    for opcoes in "--alvo=pc -q" "--alvo=uno -q" "--alvo=pico -q" "--alvo=pc"; do
        # shellcheck disable=SC2086
        linha=$("$BIN" "$programa" -o "$SAIDA/saida.ll" $opcoes 2>&1 > /dev/null |
            grep "^\[memoria\]")
        execucoes=$((execucoes + 1))
        case "$linha" in
        *": 0") ;;
        *)
            falhas=$((falhas + 1))
            echo "FAIL $programa ($opcoes): ${linha:-sem relatorio}"
            ;;
        esac
    done
done
# caminhos de erro do proprio compilador: sem argumentos, arquivo ausente
for argumentos in "" "$SAIDA/nao_existe.mac"; do
    # shellcheck disable=SC2086
    linha=$("$BIN" $argumentos 2>&1 > /dev/null | grep "^\[memoria\]")
    execucoes=$((execucoes + 1))
    case "$linha" in
    *": 0") ;;
    *) falhas=$((falhas + 1)); echo "FAIL argumentos '$argumentos': $linha" ;;
    esac
done

echo "memoria: $execucoes execucoes do compilador, $falhas com vazamento"
[ "$falhas" -eq 0 ]
