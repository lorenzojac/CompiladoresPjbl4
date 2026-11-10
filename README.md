# Compilador da Linguagem Macaronica

Compilador da linguagem Macaronica escrito em C99, para a disciplina de
Linguagens Formais e Compiladores (PUCPR):

- **PjBL 1**: front-end (léxico, sintático, semântico), já com as correções da
  avaliação;
- **PjBL 3**: geração de LLVM IR portável, executável com `lli`, `clang` e
  nas placas;
- **PjBL 4**: DSL vetorial compilada via MLIR, estudo de otimização
  (vetorização e paralelismo) e execução em placa: **Raspberry Pi Pico 2 W**
  (ARM Cortex-M33), com o Arduino Uno (AVR) como alvo secundário, testado no
  QEMU.

```text
programa.mac -> lexer -> parser -> AST -> semantica (anota a AST)
                                              |
                     +------------------------+------------------------+
                     v                                                 v
            codegen_llvm.c -> programa.ll                codegen_mlir.c -> programa.mlir
                     |                                     scripts/pipeline.sh (C0|C2|C3)
                     |                                                 v
                     +-------------- llvm-link <------------ programa.nucleos.ll
                                         |
         +------------------+------------+----------+------------------------+
         v                  v                       v                        v
  lli (+ runtime)  clang (x86-64, PC)    llc thumbv8m (Pico 2 W)   llc avr (Arduino Uno)
```

## Integrantes

| Nome | Matrícula |
| --- | --- |
| *(preencher)* | *(preencher)* |

## Estrutura

```text
src/        token, lexer, ast, parser, semantic, strbuf, codegen_llvm, codegen_mlir, main
runtime/    zl_runtime.h, zl_plataforma.h, zl_runtime_comum.c, zl_runtime_posix.c (PC),
            zl_runtime_pico.c (Pico 2 W), zl_runtime_uno.c (Arduino Uno),
            pico/ (CMakeLists.txt e pico_sdk_import.cmake: ligacao com o Pico SDK)
scripts/    pipeline.sh (MLIR comentado), construir.sh (.mac -> executavel), jit.sh (JIT do MLIR),
            pico.py (gravar/monitor/executa/portas da Pico), pico_sdk.sh (acha o Pico SDK), bench/ (Parte C)
tests/      test_*.c (unitarios), validos/ invalidos/ (PjBL 1), execucao/ (PjBL 3),
            dsl/ (PjBL 4), placa/ (Parte D; placa/pico/: LED no 25, botao no GP15),
            run_*.sh/.py (automacao; run_pico.py roda na Pico real), diferencial.py
docs/       decisoes.md, decisoes_backend.md, dsl.md, relatorio_otimizacao.md, placa.md, bench/, ir/, mlir/
placa/      executaveis .uf2 (Pico 2 W) e .hex (Arduino Uno)
Dockerfile  ambiente com versoes fixas
```

## Ferramentas e versões

Versões usadas no desenvolvimento e nas medições (Windows 11, MSYS2 UCRT64):

| Ferramenta | Versão |
| --- | --- |
| gcc (compila o compilador) | 16.2.0 (MSYS2; qualquer compilador C99 serve) |
| LLVM: `clang`, `llc`, `lli`, `opt`, `llvm-as`, `llvm-link` | **23.1.5** |
| MLIR: `mlir-opt`, `mlir-translate`, `mlir-runner` (JIT) | **23.1.5** |
| OpenMP (libomp) | 23.1.5 |
| **Pico 2 W:** `arm-none-eabi-gcc` / newlib (biblioteca e Pico SDK) | 16.2.0 / 4.6.0 |
| **Pico 2 W:** Pico SDK (só os submódulos `lib/tinyusb` e `lib/cyw43-driver`) | 2.3.1 |
| **Pico 2 W:** `cmake` / `ninja` (construção do SDK) / `picotool` (gera o `.uf2`) | 4.4.5 / 1.13.2 / 2.3.1 |
| **Uno:** avr-gcc / avr-libc / avrdude | 16.2.0 / 2.3.2 / 8.3 |
| **Uno:** QEMU (`qemu-system-avr`) | 11.1.3 |
| Python + matplotlib (benchmark) + pyserial (`scripts/pico.py`, `tests/run_pico.py`) | 3.14 / 3.11 / 3.5 |

### Windows (MSYS2, sem administrador)

1. Extrair `msys2-base-x86_64-*.sfx.exe` (repo.msys2.org) em `C:\Users\<voce>\msys64`.
2. No terminal **MSYS2 UCRT64**:

   ```sh
   pacman -Syu
   pacman -S make diffutils git mingw-w64-ucrt-x86_64-{gcc,clang,lld,llvm-tools,llvm-openmp,mlir,avr-gcc,avr-libc,avrdude,qemu,python,python-matplotlib}
   pacman -S mingw-w64-ucrt-x86_64-{arm-none-eabi-gcc,arm-none-eabi-newlib,cmake,ninja,picotool,python-pyserial}
   ```

3. Pico SDK 2.3.1, só com os submódulos da USB e do chip sem fio (~75 MB):

   ```sh
   git clone --depth 1 --branch 2.3.1 https://github.com/raspberrypi/pico-sdk ~/pico/pico-sdk
   cd ~/pico/pico-sdk && git submodule update --init --depth 1 lib/tinyusb lib/cyw43-driver
   ```

   `scripts/pico_sdk.sh` procura o SDK em `$PICO_SDK_PATH`, em `../pico-sdk`
   (ao lado do projeto), em `~/pico/pico-sdk` e em `%USERPROFILE%\pico\pico-sdk`.
4. Rodar os comandos abaixo nesse mesmo terminal (todas as ferramentas ficam
   no `PATH`).

### Linux / Docker

O `Dockerfile` (Debian 13, LLVM/MLIR 19, avr-gcc 14, arm-none-eabi-gcc 14,
Pico SDK 2.3.1 e picotool 2.3.1) reproduz o ambiente:

```sh
docker build -t macaronica .
docker run --rm -v "$PWD":/projeto macaronica make test
docker run --rm -v "$PWD":/projeto macaronica make bench
docker run --rm -v "$PWD":/projeto -e PICO_BUILD=/projeto/build/pico-docker \
    macaronica make placa PROG=tests/placa/01_laco_vetor.mac
```

O container gera o `.uf2`; para gravar, copie-o para a unidade RP2350 de fora
do container (ou use `make gravar` fora dele).

> O Dockerfile não foi testado na máquina de desenvolvimento (sem Docker).
> Ele usa os mesmos passes de MLIR, que existem desde a versão 18. Para usar
> ferramentas com sufixo de versão sem Docker:
> `make LLVM_BIN=/usr/lib/llvm-19/bin/`.

Para a Pico 2 W no Linux: `arm-none-eabi-gcc` com newlib, `cmake`, `ninja`,
`picotool`, `pyserial` e o Pico SDK 2.3.1 (mesmo `git clone` do passo 3
acima). A porta da placa costuma ser `/dev/ttyACM0`.

## Uso

```sh
make                                          # compila ./macaronica

./macaronica programa.mac                     # so valida (relatorio do PjBL 1)
./macaronica programa.mac -o programa.ll      # gera LLVM IR (+ programa.mlir se usar a DSL)
./macaronica programa.mac -o programa.ll --alvo=pico  # + verificacao de RAM da Pico 2 W
./macaronica programa.mac -o programa.ll --alvo=uno   # + verificacao de RAM do Arduino Uno

sh scripts/construir.sh programa.mac build/pc pc      # executavel do PC: build/pc/programa(.exe)
lli build/pc/programa.lli.bc                          # mesmo programa no lli
sh scripts/jit.sh programa.mac [C0|C2|C3]             # mesmo programa no JIT do MLIR

make placa PROG=programa.mac                  # placa/programa.uf2 (Raspberry Pi Pico 2 W)
make gravar PROG=programa.mac                 # grava na Pico (opcional: PORTA=COM5)
make monitor                                  # terminal da USB: o programa roda ao abrir
make placa PROG=programa.mac PLACA=uno        # placa/programa.hex (Arduino Uno)
```

`make gravar` e `make monitor` aceitam `PORTA=COMx` (Linux:
`PORTA=/dev/ttyACM0`); `python3 scripts/pico.py portas` lista as portas.

### Comandos manuais (sem os scripts)

Programa **sem** a DSL vetorial: o `.ll` do compilador é o módulo inteiro.
Estes são os comandos do enunciado:

```sh
./macaronica programa.mac -o saida.ll
llvm-as saida.ll -o saida.bc && opt -passes=verify -disable-output saida.ll
clang saida.ll runtime/zl_runtime_posix.c -o prog     # Linux/macOS: acrescentar -lm
make runtime                                          # build/zl_runtime_posix.bc
lli --extra-module=build/zl_runtime_posix.bc saida.ll
```

`zl_runtime_posix.c` inclui `zl_runtime_comum.c`, então basta um arquivo de
biblioteca. O `-lm` é preciso no Linux quando o programa usa `^` real ou
`raiz` (`llvm.pow`, `llvm.sqrt` podem virar chamadas à libm).

Programa **com** a DSL vetorial: o compilador escreve também `saida.mlir`
(os núcleos), que passa pelo pipeline MLIR e é ligado ao `.ll`:

```sh
./macaronica programa.mac -o saida.ll                  # + saida.mlir
sh scripts/pipeline.sh saida.mlir nucleos.ll C2        # C0, C2 ou C3
llvm-link -S saida.ll nucleos.ll -o final.ll
clang -O2 final.ll runtime/zl_runtime_posix.c -o prog  # C3: acrescentar -fopenmp
```

Programa para a **Raspberry Pi Pico 2 W** (o que `make placa` faz, D48). Com
a DSL, o `llc` recebe o `final.ll` do pipeline + `llvm-link` acima no lugar de
`saida.ll`:

```sh
mkdir -p build/pico
./macaronica programa.mac -o saida.ll --alvo=pico      # + verificacao de RAM da Pico
llc -mtriple=thumbv8m.main-none-eabi -mcpu=cortex-m33 -float-abi=soft -O2 \
    -function-sections -data-sections -filetype=obj saida.ll -o build/pico/programa.o
cmake -S runtime/pico -B build/pico -G Ninja -DPICO_SDK_PATH=<pasta do pico-sdk> \
      -DZL_PROGRAMA_OBJ=build/pico/programa.o          # so na primeira vez
cmake --build build/pico                               # build/pico/programa.uf2 (e .elf)
picotool load -x build/pico/programa.uf2               # placa em BOOTSEL
```

`sh scripts/pico_sdk.sh` mostra a pasta do SDK. Em vez do `picotool`, dá para
copiar o `.uf2` para a unidade `RP2350` que aparece com a placa em BOOTSEL. A
biblioteca (`runtime/zl_runtime_pico.c`) e o SDK são compilados pelo
`arm-none-eabi-gcc` dentro do CMake; o `llc` só gera o objeto do programa.

Opções do compilador: `-o saida.ll`, `--mlir=nucleos.mlir`, `--alvo=pc|pico|uno`,
`-q` (silencioso se válido) e `--fastmath` (núcleos da DSL podem reassociar
operações reais; usado no estudo da Parte C). Para um programa inválido, o
relatório de erros é impresso e **nenhum arquivo é gerado**.

Códigos de saída do compilador: 0 válido, 1 inválido, 2 erro de uso, de
leitura/escrita ou falha interna. Código de saída de um programa compilado
no PC: o valor de `respost` da `principal`, ou 3 em erro de execução.

## Placa: Raspberry Pi Pico 2 W

| Item | Valor |
| --- | --- |
| Placa principal | Raspberry Pi Pico 2 W: RP2350A, ARM Cortex-M33 a 150 MHz, 520 KB de SRAM, 4 MB de flash, chip sem fio CYW43439 (D48) |
| Alvo secundário | Arduino Uno (ATmega328P), testado no Uno emulado pelo QEMU (`make test-placa-uno`) |
| Do `.ll` ao `.uf2` | `llc -mtriple=thumbv8m.main-none-eabi -mcpu=cortex-m33 -float-abi=soft`, depois Pico SDK 2.3.1 (CMake) e `picotool` |
| E/S | `imprime` e `le` pela USB da própria placa: porta COM virtual, sem driver (D49) |
| Pinos | GP0–GP22 e GP26–GP28; **25 = LED da placa**; GP23, GP24 e GP29 dão erro 7. O PC simula a mesma numeração |
| Memória (`--alvo=pico`) | globais + textos + maior quadro ≤ 458 752 bytes (512 KB de SRAM principal − 64 KB para o SDK, a USB, o chip sem fio e o heap) |

**Primeira gravação:** segurar o botão BOOTSEL enquanto liga o cabo USB.
Depois disso, `make gravar` põe a placa em BOOTSEL sozinho (abre a porta a
1200 baud) e grava o `.uf2`.

Como o programa roda na placa (D49):

- ligada ao PC, a placa **espera um terminal abrir a porta** (`make monitor`,
  ou o Monitor Serial da IDE do Arduino) com o LED piscando devagar; assim
  nenhuma saída se perde;
- ligada num carregador (sem PC), o programa roda depois de 3 s;
- fechar o terminal reinicia a placa: abrir de novo roda o programa de novo;
- LED aceso fixo: o programa terminou; piscando rápido: erro de execução
  (a mensagem sai pela USB).

Detalhes em [`docs/placa.md`](docs/placa.md) e nas decisões D48 e D49.

## Testes (`make test`)

| Etapa | O que verifica | Resultado |
| --- | --- | --- |
| `test-lexer`, `test-parser`, `test-semantic` | 3 suítes unitárias em C | 2 371 + 8 593 + 3 349 assertions, 0 falhas |
| `test-programs` | 8 programas válidos e 9 inválidos do PjBL 1 (códigos de saída) | 17 ok |
| `test-execucao` | 19 programas do PjBL 3: `.ll` passa no `llvm-as` e no `opt -passes=verify`; executa com **lli** e com **clang**; as duas saídas são iguais ao `.esperado` | 19 ok |
| `test-dsl` | 7 programas da DSL vetorial em **C0, C2 e C3** (mesma saída) + 5 programas rejeitados com o código de erro esperado | 12 ok × 3 |
| `test-jit` | os 19 programas do PjBL 3 (C0) e os da DSL (C2, C3) também pelo **executor JIT do MLIR** (`mlir-runner`, `scripts/jit.sh`) | 19 + 12 + 12 ok |
| `test-diferencial` | 60 programas **aleatórios** válidos, comparados com um interpretador de referência em Python escrito à parte (`tests/diferencial.py`), em clang -O0, clang -O2 e lli | 60 ok (a rodada longa, `make diferencial`, passou 299 de 299) |
| `test-memoria` | **vazamento de memória**: compilador instrumentado (`ld --wrap` de `malloc`/`free`, `tests/contador_alocacoes.c`) sobre todos os programas de teste, válidos e inválidos, nos alvos pc, uno e pico: nenhum bloco alocado fica sem `free` | 222 execuções, 0 vazamentos |
| `test-placa` | programas da placa no PC (`tests/placa` e `tests/placa/pico`) + 2 rejeitados pela verificação de memória (um no `--alvo=uno`, um no `--alvo=pico`) | 8 ok |
| `test-placa-uno` | no **Arduino Uno emulado** (QEMU, alvo secundário), construídos a partir do mesmo `.ll`: programas da placa em C0 e em C2 (núcleos vetorizados) e os 19 de `tests/execucao`; saída idêntica à do PC | 5 + 5 + 19 ok |

Fora do `make test`, porque exige a placa ligada na USB:

| Etapa | O que verifica | Resultado |
| --- | --- | --- |
| `test-pico` | na **Raspberry Pi Pico 2 W real** (`tests/run_pico.py`): programas construídos a partir do mesmo `.ll` do PC, gravados e executados pela USB, com a saída comparada à esperada: 5 da placa + 1 só da Pico em C0, 5 em C2 e os 19 de `tests/execucao` | 30 ok, 0 falhas (placa real, 10/11/2026) |

Cada `.esperado` foi conferido à mão (valores calculados independentemente).
Os `.esperado` usam fim de linha LF (`.gitattributes`). No Windows, o `\r`
que o modo texto acrescenta é retirado antes da comparação, então os mesmos
arquivos valem em Linux, macOS e Windows. Na Pico, o SDK envia `\r\n` pela
USB, e `tests/run_pico.py` também retira o `\r`.

## Benchmark (`make bench`)

Gera os núcleos com o compilador, mede C0, C1, C1-fastmath, C2, C2+C1 e C3
(1 a 20 threads) com N = 10⁶ e 10⁷ (mediana de 5), coleta `-Rpass` e o IR, e
escreve `docs/bench/resultados.md` e os gráficos. Análise em
[`docs/relatorio_otimizacao.md`](docs/relatorio_otimizacao.md).

## Documentação

| Documento | Conteúdo |
| --- | --- |
| [`docs/decisoes.md`](docs/decisoes.md) | Decisões D01–D49 (PjBL 1, correções, PjBL 3/4; placa Pico 2 W em D48 e D49) |
| [`docs/decisoes_backend.md`](docs/decisoes_backend.md) | PjBL 3: tipos, construções, portabilidade, biblioteca de execução e comandos por alvo (PC, Pico 2 W, Uno), lacunas da Seção 4.3, análise de mem2reg/phi e -O2 |
| [`docs/dsl.md`](docs/dsl.md) | PjBL 4 A: especificação formal da DSL vetorial e lacunas da Seção 5.5 |
| [`docs/relatorio_otimizacao.md`](docs/relatorio_otimizacao.md) | PjBL 4 B e C: estratégia híbrida, pipeline, núcleo em três níveis, medições e pergunta obrigatória |
| [`docs/placa.md`](docs/placa.md) | PjBL 4 D: Raspberry Pi Pico 2 W (e o Arduino Uno como alvo secundário), ferramentas, comandos, decisões do alvo, memória, tamanhos, vídeo |
| [`docs/README_gramatica.md`](docs/README_gramatica.md), [`docs/tokens.md`](docs/tokens.md) | Gramática e léxico efetivos |
| [`docs/especificacao_macaronica.md`](docs/especificacao_macaronica.md), [`docs/README_tabela.md`](docs/README_tabela.md) | Especificação original e o que ela define ou deixa em aberto |

## Mapa da rubrica

> A rubrica do enunciado cita as Seções 4.4 (lacunas do back-end), 4.6 (IR
> portável) e 5.6 (lacunas do PjBL 4). No corpo do enunciado essas seções são
> numeradas 4.3, 4.5 e 5.5, e é essa numeração que a documentação usa.

### PjBL 3

| Critério | Onde |
| --- | --- |
| 1. Corretude da IR | `make test-execucao` (lli + clang + verify em 19 programas) e `make test-diferencial` (programas aleatórios contra um interpretador independente); IR com nomes do fonte e `; linha N` |
| 2. Cobertura | todas as construções da Seção 4.1, incluindo registros (D39); verificação de limites e erros com linha (`decisoes_backend.md` §6.4) |
| 3. Tipos e regras | mapeamento e conversões em `decisoes_backend.md` §2 e §6.1; verificações só quando não provadas estaticamente (constantes, D40; análise de faixa dos laços, D47), com IR de evidência (§6.4) |
| 4. SSA e ferramentas | `decisoes_backend.md` §8: IR antes/depois de mem2reg e -O2, explicação de cada `phi` |
| 5. Testes | `make test` integra PjBL 1, 3 e 4 |
| 6. Arquitetura | gerador separado, só lê a AST anotada; regras da Seção 4.5 (§4); sem warnings; sem vazamento de memória (`make test-memoria`) |
| 7. Lacunas | `decisoes_backend.md` §6 (todas as da Seção 4.3) e §7 (adicionais) |

### PjBL 4

| Critério | Onde |
| --- | --- |
| 1. DSL | `docs/dsl.md`: tokens, gramática, tipos, extras (`minvet`, expressões fundidas, preenchimento, negação) |
| 2. MLIR | `scripts/pipeline.sh` comentado; núcleo em três níveis + OpenMP (`relatorio_otimizacao.md` §3); `vector` via `affine-super-vectorize`; executor JIT do MLIR (`scripts/jit.sh`, `make test-jit`, §2) |
| 3. Otimização | C0–C3 com mediana de 5, máquina descrita, `-Rpass`, IR, escalabilidade, resultados inesperados (§4–§8) |
| 4. Placa | **Raspberry Pi Pico 2 W** (D48, D49): mesmo `.ll` do PC compilado para ARM Cortex-M33 (`llc` + Pico SDK), `make placa`/`gravar`/`monitor`, runtime da placa (`zl_runtime_pico.c`: E/S pela USB, LED, pinos), E/S física (LED no pino 25, botão no GP15), verificação de memória (`--alvo=pico`), testes na placa real (`make test-pico`: 30 execuções, saída idêntica à do PC, inclusive `le` pela USB e núcleos MLIR em C2). **Arduino Uno** como alvo secundário: saída idêntica no Uno emulado (5 da placa em C0 e C2, mais os 19 do PjBL 3), `duplocarpado` de 64 bits, núcleos MLIR na placa, textos da biblioteca na flash (`placa.md`) |
| 5. Reprodutibilidade | versões fixadas, `make test`/`make bench` com um comando, Dockerfile |

## Correções do PjBL 1

| Item da avaliação | Correção |
| --- | --- |
| 2.2 Estouro de pilha com aninhamento extremo | Limites de aninhamento (256) e altura de expressão (1024): um erro sintático (D35) |
| 2.3 Função `vazio` impossível de chamar | Chamada isolada `f(x);` é comando (D33) |
| 2.4 Cascatas (`;` ausente, erro léxico), relatório ambíguo | `;` inserido virtualmente, modo pânico, status `PARCIAL`/`NAO EXECUTADO` (D36) |
| 2.4 UTF-8: um erro por caractere | Identificador acentuado é um único erro (D37) |
| 2.4 Sem retorno em todos os caminhos / sem verificação de limites | `SEM_MISSING_RETURN` (D34); limites verificados (D40 e PjBL 3) |
| 2.5 Pouco comentário, arquivos longos, busca linear | Comentários de módulo e função; índice hash na tabela de símbolos |
| 2.6 README, `.mac` fora do `make test` | README reescrito; os programas `.mac` entram no `make test` (`test-programs`) |
| 2.6 Integrantes | **Pendente**: preencher a tabela "Integrantes" acima |

## Pendências do grupo antes da entrega

- Preencher a tabela "Integrantes".
- **Avisar o professor que a placa da demonstração é a Raspberry Pi Pico 2 W**
  (o Arduino Uno continua como alvo secundário, testado no QEMU).
- Gravar o vídeo da Parte D **com a Pico 2 W**: `make placa` + `make gravar`,
  a saída no terminal (`make monitor`) e o LED piscando.
- Testar o programa do botão (`tests/placa/pico/manual/06_botao_pico.mac`)
  com um botão entre o GP15 e o GND, e rodar `make test-pico` de novo antes
  da entrega.
- Revisar a seção "Uso de IA generativa".

## Uso de IA generativa

*(Revisar antes da entrega e ajustar ao que o grupo de fato fez.)*

- PjBL 1: ferramentas de IA generativa como apoio à implementação e à
  depuração.
- Correções do PjBL 1 e PjBL 3/4: o **Claude (Anthropic), via Claude Code**,
  escreveu, sob orientação do grupo, a maior parte do código novo (gerador de
  LLVM IR, gerador de MLIR, bibliotecas de execução, scripts de pipeline,
  construção, testes e benchmark), os testes e esta documentação, e executou
  as medições. O grupo definiu os objetivos e revisou o material, e cada
  integrante deve ser capaz de explicar e defender qualquer trecho na
  arguição.

## Limitações conhecidas

- Registros não podem ser parâmetros, retorno, `le`/`imprime` ou operandos
  (só atribuição e campos, D39).
- Vetores locais grandes ficam na pilha; no PC (Windows) use vetores globais
  acima de ~200 mil elementos.
- Nas placas (Pico 2 W e Uno), a profundidade de recursão não é verificada
  estaticamente.
- Na Pico 2 W, o programa usa só um dos dois núcleos, e o chip sem fio só
  serve para o LED (sem Wi-Fi nem Bluetooth).
- A linguagem não tem comentários.
