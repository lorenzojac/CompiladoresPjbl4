#!/bin/sh
# scripts/construir.sh - do programa .mac ao executavel.
#
# Uso: sh scripts/construir.sh programa.mac dir_saida [pc|uno] [C0|C1|C2|C3]
#
# Etapas (nome = programa sem extensao):
#   1. macaronica programa.mac -o nome.ll   (e nome.mlir se usar a DSL)
#   2. llvm-as e opt -passes=verify          (IR valido, Secao 4.4)
#   3. pipeline.sh nome.mlir -> nome.nucleos.ll; llvm-link -> nome.final.ll
#   4. pc : clang nome.final.ll + biblioteca -> nome(.exe)
#           e nome.lli.bc (programa + biblioteca) para rodar com lli
#      uno: llc -mtriple=avr -mcpu=atmega328p -> nome.o
#           avr-gcc + biblioteca -> nome.elf -> nome.hex
#
# C1 = pipeline C0 + clang -O2 (vetorizacao automatica do LLVM); C0 usa -O0;
# C2/C3 usam -O2 sem vetorizacao do LLVM, para isolar o efeito do MLIR.
# Variaveis: MACARONICA, CLANG, LLC, LLVM_AS, LLVM_LINK, OPT, AVR_GCC,
# AVR_OBJCOPY, CFLAGS_PROG (ex.: -DZL_SENTINELA para testes no QEMU).
set -eu

PROGRAMA=$1
SAIDA=$2
ALVO=${3:-pc}
CONFIG=${4:-C0}

RAIZ=$(cd "$(dirname "$0")/.." && pwd)
MACARONICA=${MACARONICA:-$RAIZ/macaronica}
CLANG=${CLANG:-clang}
LLC=${LLC:-llc}
LLVM_AS=${LLVM_AS:-llvm-as}
LLVM_LINK=${LLVM_LINK:-llvm-link}
OPT=${OPT:-opt}
AVR_GCC=${AVR_GCC:-avr-gcc}
AVR_OBJCOPY=${AVR_OBJCOPY:-avr-objcopy}
CFLAGS_PROG=${CFLAGS_PROG:-}
RT=$RAIZ/runtime

case "$(uname -s)" in
MINGW* | MSYS* | CYGWIN*) EXE=.exe ;;
*) EXE= ;;
esac

NOME=$(basename "$PROGRAMA" .mac)
BASE=$SAIDA/$NOME
mkdir -p "$SAIDA"
rm -f "$BASE.ll" "$BASE.mlir" "$BASE.nucleos.ll" "$BASE.final.ll"

# 1. Compilador do grupo (falha = programa invalido; o relatorio aparece).
ALVO_SEM=pc
[ "$ALVO" = uno ] && ALVO_SEM=uno
"$MACARONICA" "$PROGRAMA" -o "$BASE.ll" --alvo=$ALVO_SEM -q

# 2. O IR precisa passar no montador e no verificador.
"$LLVM_AS" "$BASE.ll" -o "$BASE.bc"
"$OPT" -passes=verify -disable-output "$BASE.ll"

# 3. Nucleos MLIR (DSL vetorial), unidos ao programa.
if [ -f "$BASE.mlir" ]; then
    MLIR_CONFIG=$CONFIG
    [ "$CONFIG" = C1 ] && MLIR_CONFIG=C0
    sh "$RAIZ/scripts/pipeline.sh" "$BASE.mlir" "$BASE.nucleos.ll" "$MLIR_CONFIG"
    "$LLVM_LINK" -S "$BASE.ll" "$BASE.nucleos.ll" -o "$BASE.final.ll"
else
    cp "$BASE.ll" "$BASE.final.ll"
fi

case "$CONFIG" in
C0) OTIM="-O0" ;;
C1) OTIM="-O2" ;;
*) OTIM="-O2 -fno-vectorize -fno-slp-vectorize" ;;
esac
OMP=
[ "$CONFIG" = C3 ] && OMP=-fopenmp

# 4. Executavel.
if [ "$ALVO" = pc ]; then
    # shellcheck disable=SC2086
    # O comando da Secao 4.4 do enunciado: clang saida.ll zl_runtime_posix.c
    # (a biblioteca do PC inclui a parte comum).
    "$CLANG" -Wno-override-module $OTIM $OMP $CFLAGS_PROG -I"$RT" \
        "$BASE.final.ll" "$RT/zl_runtime_posix.c" -lm -o "$BASE$EXE"
    # Programa + biblioteca em um modulo so, para o lli.
    "$CLANG" -O1 -c -emit-llvm -I"$RT" "$RT/zl_runtime_posix.c" -o "$BASE.rt.bc"
    "$LLVM_LINK" "$BASE.final.ll" "$BASE.rt.bc" -o "$BASE.lli.bc"
    rm -f "$BASE.rt.bc"
elif [ "$ALVO" = uno ]; then
    # Mesmo IR, outra arquitetura: AVR de 8 bits, sem SO.
    "$LLC" -mtriple=avr -mcpu=atmega328p -O2 -filetype=obj \
        "$BASE.final.ll" -o "$BASE.o"
    # shellcheck disable=SC2086
    "$AVR_GCC" -mmcu=atmega328p -DF_CPU=16000000UL -mdouble=64 -Os \
        $CFLAGS_PROG -I"$RT" "$BASE.o" "$RT/zl_runtime_uno.c" \
        -lm -Wl,--gc-sections \
        -Wl,--no-warn-execstack -o "$BASE.elf" 2>"$BASE.ld.log" || {
        cat "$BASE.ld.log" >&2
        exit 1
    }
    "$AVR_OBJCOPY" -O ihex -R .eeprom "$BASE.elf" "$BASE.hex"
else
    echo "construir.sh: alvo invalido: $ALVO (use pc ou uno)" >&2
    exit 2
fi
