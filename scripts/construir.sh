#!/bin/sh
# scripts/construir.sh - do programa .mac ao executavel.
#
# Uso: sh scripts/construir.sh programa.mac dir_saida [pc|pico|uno] [C0|C1|C2|C3]
#
# Etapas (nome = programa sem extensao):
#   1. macaronica programa.mac -o nome.ll   (e nome.mlir se usar a DSL)
#   2. llvm-as e opt -passes=verify          (IR valido, Secao 4.4)
#   3. pipeline.sh nome.mlir -> nome.nucleos.ll; llvm-link -> nome.final.ll
#   4. pc  : clang nome.final.ll + biblioteca -> nome(.exe)
#            e nome.lli.bc (programa + biblioteca) para rodar com lli
#      pico: llc -mtriple=thumbv8m.main-none-eabi -mcpu=cortex-m33 -> nome.o
#            Pico SDK (CMake) + biblioteca -> nome.elf e nome.uf2
#      uno : llc -mtriple=avr -mcpu=atmega328p -> nome.o
#            avr-gcc + biblioteca -> nome.elf -> nome.hex
#
# C1 = pipeline C0 + clang -O2 (vetorizacao automatica do LLVM); C0 usa -O0;
# C2/C3 usam -O2 sem vetorizacao do LLVM, para isolar o efeito do MLIR.
# Variaveis: MACARONICA, CLANG, LLC, LLVM_AS, LLVM_LINK, OPT, AVR_GCC,
# AVR_OBJCOPY, CMAKE, PICO_SDK_PATH, PICO_BUILD, CFLAGS_PROG (ex.:
# -DZL_SENTINELA para testes no QEMU e na placa).
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
CMAKE=${CMAKE:-cmake}
CFLAGS_PROG=${CFLAGS_PROG:-}
RT=$RAIZ/runtime

case "$ALVO" in
pc | pico | uno) ;;
*)
    echo "construir.sh: alvo invalido: $ALVO (use pc, pico ou uno)" >&2
    exit 2
    ;;
esac

# Caminho no formato das ferramentas nativas (C:/... no MSYS2/Windows).
CAMINHO() {
    if command -v cygpath >/dev/null 2>&1; then
        cygpath -m "$1"
    else
        printf '%s\n' "$1"
    fi
}

case "$(uname -s)" in
MINGW* | MSYS* | CYGWIN*) EXE=.exe ;;
*) EXE= ;;
esac

NOME=$(basename "$PROGRAMA" .mac)
BASE=$SAIDA/$NOME
mkdir -p "$SAIDA"
rm -f "$BASE.ll" "$BASE.mlir" "$BASE.nucleos.ll" "$BASE.final.ll"

# 1. Compilador do grupo (falha = programa invalido; o relatorio aparece).
"$MACARONICA" "$PROGRAMA" -o "$BASE.ll" --alvo="$ALVO" -q

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
elif [ "$ALVO" = pico ]; then
    # Mesmo IR, outra arquitetura: ARM Cortex-M33 da RP2350 (Thumb-2,
    # ARMv8-M Mainline). ABI de float "soft" com FPU = softfp, a mesma do
    # Pico SDK para a RP2350 (D48).
    "$LLC" -mtriple=thumbv8m.main-none-eabi -mcpu=cortex-m33 \
        -float-abi=soft -O2 -filetype=obj \
        -function-sections -data-sections \
        "$BASE.final.ll" -o "$BASE.o"
    # Pico SDK (CMake + Ninja): objeto + zl_runtime_pico.c -> .elf e .uf2.
    # Uma pasta de construcao por variante; a configuracao (lenta) e feita
    # uma vez e depois so o objeto do programa muda.
    SENTINELA=OFF
    case "$CFLAGS_PROG" in *ZL_SENTINELA*) SENTINELA=ON ;; esac
    PICO_BUILD=${PICO_BUILD:-$RAIZ/build/pico-$SENTINELA}
    mkdir -p "$PICO_BUILD"
    cp "$BASE.o" "$PICO_BUILD/programa.o"
    if [ ! -f "$PICO_BUILD/build.ninja" ]; then
        SDK=$(sh "$RAIZ/scripts/pico_sdk.sh") || exit 1
        "$CMAKE" -S "$(CAMINHO "$RT/pico")" -B "$(CAMINHO "$PICO_BUILD")" \
            -G Ninja -DCMAKE_BUILD_TYPE=Release \
            -DPICO_SDK_PATH="$(CAMINHO "$SDK")" \
            -DZL_PROGRAMA_OBJ="$(CAMINHO "$PICO_BUILD/programa.o")" \
            -DZL_SENTINELA=$SENTINELA >"$BASE.cmake.log" 2>&1 || {
            cat "$BASE.cmake.log" >&2
            rm -f "$PICO_BUILD/build.ninja"
            exit 1
        }
    fi
    "$CMAKE" --build "$PICO_BUILD" >"$BASE.ld.log" 2>&1 || {
        cat "$BASE.ld.log" >&2
        exit 1
    }
    cp "$PICO_BUILD/programa.elf" "$BASE.elf"
    cp "$PICO_BUILD/programa.uf2" "$BASE.uf2"
else
    echo "construir.sh: alvo invalido: $ALVO (use pc, pico ou uno)" >&2
    exit 2
fi
