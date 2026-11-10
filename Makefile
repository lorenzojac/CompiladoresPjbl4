# Compilador Macaronica - PjBL 1, 3 e 4.
#
#   make              compila o compilador (./macaronica)
#   make test         testes unitarios + programas .mac + execucao (clang,
#                     lli e JIT do MLIR) + DSL vetorial C0/C2/C3 + teste
#                     diferencial curto + placa (PC e Uno no QEMU, C0 e C2)
#   make diferencial  teste diferencial longo (N=300 programas aleatorios)
#   make runtime      build/zl_runtime_posix.bc, para lli --extra-module=
#   make bench        estudo de otimizacao C0-C3 (PjBL 4, Parte C)
#
# Placa (PLACA=pico, padrao: Raspberry Pi Pico 2 W; PLACA=uno: Arduino Uno):
#   make placa PROG=tests/placa/01_laco_vetor.mac   gera placa/<nome>.uf2
#   make gravar PROG=...  [PORTA=COM5]               grava na placa
#   make monitor [PORTA=COM5]                        terminal da placa (USB)
#   make test-pico    programas de tests/placa rodando na Pico conectada
#
# Ferramentas: por padrao vem do PATH. Para usar versoes com sufixo
# (Ubuntu: clang-18, mlir-opt-18), passe LLVM_BIN=/usr/lib/llvm-18/bin/.
# Pico SDK: PICO_SDK_PATH ou um dos lugares de scripts/pico_sdk.sh.

CC ?= cc
CFLAGS ?=
PROJECT_CFLAGS = -std=c99 -Wall -Wextra -Wpedantic

LLVM_BIN ?=
CLANG ?= $(LLVM_BIN)clang
LLC ?= $(LLVM_BIN)llc
LLI ?= $(LLVM_BIN)lli
LLVM_AS ?= $(LLVM_BIN)llvm-as
LLVM_LINK ?= $(LLVM_BIN)llvm-link
OPT ?= $(LLVM_BIN)opt
MLIR_OPT ?= $(LLVM_BIN)mlir-opt
MLIR_TRANSLATE ?= $(LLVM_BIN)mlir-translate
MLIR_RUNNER ?= $(LLVM_BIN)mlir-runner
AVR_GCC ?= avr-gcc
AVR_OBJCOPY ?= avr-objcopy
AVR_SIZE ?= avr-size
AVRDUDE ?= avrdude
QEMU_AVR ?= qemu-system-avr
CMAKE ?= cmake
ARM_SIZE ?= arm-none-eabi-size
PYTHON ?= python3
PLACA ?= pico
PORTA ?=
PROG ?= tests/placa/01_laco_vetor.mac
N ?= 300

export CLANG LLC LLI LLVM_AS LLVM_LINK OPT MLIR_OPT MLIR_TRANSLATE MLIR_RUNNER
export AVR_GCC AVR_OBJCOPY AVR_SIZE QEMU_AVR CMAKE

CPPFLAGS = -Isrc
BUILD_DIR = build
APP_BIN = macaronica

FRONTEND_SOURCES = \
	src/token.c \
	src/lexer.c \
	src/ast.c \
	src/parser.c \
	src/semantic.c

APP_SOURCES = \
	$(FRONTEND_SOURCES) \
	src/strbuf.c \
	src/codegen_mlir.c \
	src/codegen_llvm.c \
	src/main.c

APP_HEADERS = \
	src/token.h \
	src/lexer.h \
	src/ast.h \
	src/parser.h \
	src/semantic.h \
	src/strbuf.h \
	src/codegen_mlir.h \
	src/codegen_llvm.h

TEST_LEXER_BIN = $(BUILD_DIR)/test_lexer
TEST_PARSER_BIN = $(BUILD_DIR)/test_parser
TEST_SEMANTIC_BIN = $(BUILD_DIR)/test_semantic

TEM_QEMU := $(shell command -v $(QEMU_AVR) 2>/dev/null)

.PHONY: all test test-unit test-lexer test-parser test-semantic \
	test-programs test-execucao test-dsl test-jit test-diferencial test-memoria \
	test-placa test-placa-uno test-pico diferencial bench placa gravar monitor \
	runtime clean

all: $(APP_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(APP_BIN): $(APP_SOURCES) $(APP_HEADERS)
	$(CC) $(CFLAGS) $(PROJECT_CFLAGS) $(CPPFLAGS) $(APP_SOURCES) -o $(APP_BIN)

$(TEST_LEXER_BIN): $(BUILD_DIR) src/token.c src/token.h src/lexer.c src/lexer.h tests/test_lexer.c
	$(CC) $(CFLAGS) $(PROJECT_CFLAGS) $(CPPFLAGS) src/token.c src/lexer.c tests/test_lexer.c -o $(TEST_LEXER_BIN)

$(TEST_PARSER_BIN): $(BUILD_DIR) src/token.c src/token.h src/lexer.c src/lexer.h src/ast.c src/ast.h src/parser.c src/parser.h tests/test_parser.c
	$(CC) $(CFLAGS) $(PROJECT_CFLAGS) $(CPPFLAGS) src/token.c src/lexer.c src/ast.c src/parser.c tests/test_parser.c -o $(TEST_PARSER_BIN)

$(TEST_SEMANTIC_BIN): $(BUILD_DIR) $(FRONTEND_SOURCES) src/ast.h src/parser.h src/semantic.h tests/test_semantic.c
	$(CC) $(CFLAGS) $(PROJECT_CFLAGS) $(CPPFLAGS) $(FRONTEND_SOURCES) tests/test_semantic.c -o $(TEST_SEMANTIC_BIN)

# --- PjBL 1: testes unitarios e programas validos/invalidos ---------------
test-lexer: $(TEST_LEXER_BIN)
	./$(TEST_LEXER_BIN)

test-parser: $(TEST_PARSER_BIN)
	./$(TEST_PARSER_BIN)

test-semantic: $(TEST_SEMANTIC_BIN)
	./$(TEST_SEMANTIC_BIN)

test-unit: test-lexer test-parser test-semantic

test-programs: $(APP_BIN)
	sh tests/run_programs.sh ./$(APP_BIN)

# --- PjBL 3: geracao de LLVM IR, execucao com lli e clang -----------------
test-execucao: $(APP_BIN)
	sh tests/run_execucao.sh tests/execucao

# --- PjBL 4: DSL vetorial (MLIR) e placa ----------------------------------
# Os nucleos devem dar o mesmo resultado com e sem vetorizacao/paralelismo.
test-dsl: $(APP_BIN)
	CONFIG=C0 sh tests/run_execucao.sh tests/dsl
	CONFIG=C2 sh tests/run_execucao.sh tests/dsl
	CONFIG=C3 sh tests/run_execucao.sh tests/dsl

# Executor JIT do MLIR (mlir-runner) sobre o mesmo modulo final: programas
# escalares em C0 e nucleos da DSL vetorizados (C2) e paralelos (C3).
test-jit: $(APP_BIN)
	JIT=1 CONFIG=C0 sh tests/run_execucao.sh tests/execucao
	JIT=1 CONFIG=C2 sh tests/run_execucao.sh tests/dsl
	JIT=1 CONFIG=C3 sh tests/run_execucao.sh tests/dsl

# Programas aleatorios contra um interpretador de referencia independente.
test-diferencial: $(APP_BIN)
	$(PYTHON) tests/diferencial.py 60 2026

diferencial: $(APP_BIN)
	$(PYTHON) tests/diferencial.py $(N)

# Nenhum bloco alocado pelo compilador fica sem free (ld --wrap).
test-memoria:
	CC="$(CC)" sh tests/run_memoria.sh

test-placa: $(APP_BIN)
	ALVO=uno sh tests/run_execucao.sh tests/placa
	ALVO=pico sh tests/run_execucao.sh tests/placa/pico

# No Uno emulado: programas da placa em C0 e com os nucleos vetorizados (C2),
# e todos os programas de tests/execucao (mesma saida do PC).
test-placa-uno: $(APP_BIN)
	CONFIG=C0 $(PYTHON) tests/run_placa.py tests/placa
	CONFIG=C2 $(PYTHON) tests/run_placa.py tests/placa
	CONFIG=C0 $(PYTHON) tests/run_placa.py tests/execucao

# Na Raspberry Pi Pico 2 W de verdade (conectada pela USB; nao faz parte do
# make test): grava cada programa, le a saida pela USB e compara. Programas
# da placa em C0 e C2 e todos os de tests/execucao (mesma saida do PC).
test-pico: $(APP_BIN)
	CONFIG=C0 $(PYTHON) tests/run_pico.py tests/placa tests/placa/pico
	CONFIG=C2 $(PYTHON) tests/run_pico.py tests/placa
	CONFIG=C0 $(PYTHON) tests/run_pico.py tests/execucao

TESTES_PC = test-unit test-programs test-execucao test-dsl test-jit \
	test-diferencial test-memoria test-placa

ifeq ($(TEM_QEMU),)
test: $(TESTES_PC)
	@echo "(QEMU AVR nao encontrado: testes no Uno emulado pulados)"
else
test: $(TESTES_PC) test-placa-uno
endif

# Biblioteca do PC como modulo de bitcode, para executar com o lli:
#   lli --extra-module=build/zl_runtime_posix.bc saida.ll
runtime: $(BUILD_DIR)/zl_runtime_posix.bc

$(BUILD_DIR)/zl_runtime_posix.bc: runtime/zl_runtime_posix.c runtime/zl_runtime_comum.c runtime/zl_runtime.h runtime/zl_plataforma.h | $(BUILD_DIR)
	$(CLANG) -O1 -c -emit-llvm runtime/zl_runtime_posix.c -o $@

bench: $(APP_BIN)
	$(PYTHON) scripts/bench/bench.py

NOME_PROG = $(basename $(notdir $(PROG)))
DIR_PLACA = $(BUILD_DIR)/placa-$(PLACA)

# make placa PROG=programa.mac
#   PLACA=pico (padrao) -> placa/<programa>.uf2 (Raspberry Pi Pico 2 W)
#   PLACA=uno           -> placa/<programa>.hex (Arduino Uno)
placa: $(APP_BIN)
	sh scripts/construir.sh $(PROG) $(DIR_PLACA) $(PLACA)
	mkdir -p placa
ifeq ($(PLACA),uno)
	cp $(DIR_PLACA)/$(NOME_PROG).hex placa/
	$(AVR_SIZE) $(DIR_PLACA)/$(NOME_PROG).elf
	@echo "Executavel: placa/$(NOME_PROG).hex"
else
	cp $(DIR_PLACA)/$(NOME_PROG).uf2 placa/
	$(ARM_SIZE) $(DIR_PLACA)/$(NOME_PROG).elf
	@echo "Executavel: placa/$(NOME_PROG).uf2"
endif

# Pico: copia o .uf2 para a unidade RP2350 (na primeira vez, conectar com
# BOOTSEL apertado; depois o script reinicia a placa sozinho pela USB).
# Uno: avrdude pelo bootloader (clones antigos: -b 57600).
gravar: placa
ifeq ($(PLACA),uno)
	$(AVRDUDE) -c arduino -p atmega328p -P $(or $(PORTA),/dev/ttyACM0) \
		-b 115200 -U flash:w:placa/$(NOME_PROG).hex:i
else
	$(PYTHON) scripts/pico.py gravar placa/$(NOME_PROG).uf2 \
		$(if $(PORTA),--porta $(PORTA))
endif

# Terminal da Pico pela USB: abrir roda o programa; fechar reinicia a placa.
monitor:
	$(PYTHON) scripts/pico.py monitor $(if $(PORTA),--porta $(PORTA))

clean:
	rm -rf $(BUILD_DIR) $(APP_BIN) $(APP_BIN).exe
