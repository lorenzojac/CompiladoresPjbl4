# Compilador da Linguagem Macaronica

Compilador da linguagem Macaronica escrito em C99, para a disciplina de
Linguagens Formais e Compiladores (PUCPR):

- **PjBL 1**: front-end (léxico, sintático, semântico), já com as correções da
  avaliação;
- **PjBL 3**: geração de LLVM IR portável, executável com `lli`, `clang` e no
  Arduino Uno;
- **PjBL 4**: DSL vetorial compilada via MLIR, estudo de otimização
  (vetorização e paralelismo) e execução em placa (Arduino Uno).

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
                 +-----------------------+------------------------+
                 v                       v                        v
           lli (+ runtime)     clang (x86-64, PC)     llc -mtriple=avr (Arduino Uno)
```

## Integrantes

| Nome | Matrícula |
| --- | --- |
| *(preencher)* | *(preencher)* |

## Estrutura

```text
src/        token, lexer, ast, parser, semantic, strbuf, codegen_llvm, codegen_mlir, main
runtime/    zl_runtime.h, zl_runtime_comum.c, zl_runtime_posix.c (PC), zl_runtime_uno.c (Arduino Uno)
scripts/    pipeline.sh (MLIR comentado), construir.sh (.mac -> executavel), jit.sh (JIT do MLIR), bench/ (Parte C)
tests/      test_*.c (unitarios), validos/ invalidos/ (PjBL 1), execucao/ (PjBL 3),
            dsl/ (PjBL 4), placa/ (Parte D), run_*.sh/.py (automacao), diferencial.py
docs/       decisoes.md, decisoes_backend.md, dsl.md, relatorio_otimizacao.md, placa.md, bench/, ir/, mlir/
placa/      executaveis .hex para o Arduino Uno
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
| avr-gcc / avr-libc / avrdude | 16.2.0 / 2.3.2 / 8.3 |
| QEMU (`qemu-system-avr`) | 11.1.3 |
| Python + matplotlib (scripts de benchmark) | 3.14 / 3.11 |

### Windows (MSYS2, sem administrador)

1. Extrair `msys2-base-x86_64-*.sfx.exe` (repo.msys2.org) em `C:\Users\<voce>\msys64`.
2. No terminal **MSYS2 UCRT64**:

   ```sh
   pacman -Syu
   pacman -S make diffutils mingw-w64-ucrt-x86_64-{gcc,clang,lld,llvm-tools,llvm-openmp,mlir,avr-gcc,avr-libc,avrdude,qemu,python,python-matplotlib}
   ```

3. Rodar os comandos abaixo nesse mesmo terminal (todas as ferramentas ficam
   no `PATH`).

### Linux / Docker

O `Dockerfile` (Debian 13, LLVM/MLIR 19, avr-gcc 14) reproduz o ambiente:

```sh
docker build -t macaronica .
docker run --rm -v "$PWD":/projeto macaronica make test
docker run --rm -v "$PWD":/projeto macaronica make bench
```

> O Dockerfile não foi testado na máquina de desenvolvimento (sem Docker).
> Ele usa os mesmos passes de MLIR, que existem desde a versão 18. Para usar
> ferramentas com sufixo de versão sem Docker:
> `make LLVM_BIN=/usr/lib/llvm-19/bin/`.

## Uso

```sh
make                                          # compila ./macaronica

./macaronica programa.mac                     # so valida (relatorio do PjBL 1)
./macaronica programa.mac -o programa.ll      # gera LLVM IR (+ programa.mlir se usar a DSL)
./macaronica programa.mac -o programa.ll --alvo=uno   # + verificacao de RAM do Arduino Uno

sh scripts/construir.sh programa.mac build/pc pc      # executavel do PC: build/pc/programa(.exe)
lli build/pc/programa.lli.bc                          # mesmo programa no lli
sh scripts/jit.sh programa.mac [C0|C2|C3]             # mesmo programa no JIT do MLIR

make placa PROG=programa.mac                  # placa/programa.hex (Arduino Uno)
make gravar PROG=programa.mac PORTA=COM3      # grava (Linux: PORTA=/dev/ttyACM0)
```

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

Opções do compilador: `-o saida.ll`, `--mlir=nucleos.mlir`, `--alvo=pc|uno`,
`-q` (silencioso se válido) e `--fastmath` (núcleos da DSL podem reassociar
operações reais; usado no estudo da Parte C). Para um programa inválido, o
relatório de erros é impresso e **nenhum arquivo é gerado**.

Códigos de saída do compilador: 0 válido, 1 inválido, 2 erro de uso, de
leitura/escrita ou falha interna. Código de saída de um programa compilado
no PC: o valor de `respost` da `principal`, ou 3 em erro de execução.

## Testes (`make test`)

| Etapa | O que verifica | Resultado |
| --- | --- | --- |
| `test-lexer`, `test-parser`, `test-semantic` | 3 suítes unitárias em C | 2 371 + 8 593 + 3 349 assertions, 0 falhas |
| `test-programs` | 8 programas válidos e 9 inválidos do PjBL 1 (códigos de saída) | 17 ok |
| `test-execucao` | 19 programas do PjBL 3: `.ll` passa no `llvm-as` e no `opt -passes=verify`; executa com **lli** e com **clang**; as duas saídas são iguais ao `.esperado` | 19 ok |
| `test-dsl` | 7 programas da DSL vetorial em **C0, C2 e C3** (mesma saída) + 5 programas rejeitados com o código de erro esperado | 12 ok × 3 |
| `test-jit` | os 19 programas do PjBL 3 (C0) e os da DSL (C2, C3) também pelo **executor JIT do MLIR** (`mlir-runner`, `scripts/jit.sh`) | 19 + 12 + 12 ok |
| `test-diferencial` | 60 programas **aleatórios** válidos, comparados com um interpretador de referência em Python escrito à parte (`tests/diferencial.py`), em clang -O0, clang -O2 e lli | 60 ok (a rodada longa, `make diferencial`, passou 299 de 299) |
| `test-memoria` | **vazamento de memória**: compilador instrumentado (`ld --wrap` de `malloc`/`free`, `tests/contador_alocacoes.c`) sobre todos os programas de teste, válidos e inválidos, nos alvos pc e uno: nenhum bloco alocado fica sem `free` | 167 execuções, 0 vazamentos |
| `test-placa` | programas da placa no PC + 1 rejeitado pela verificação de memória do Uno | 6 ok |
| `test-placa-uno` | no **Arduino Uno emulado** (QEMU), construídos a partir do mesmo `.ll`: programas da placa em C0 e em C2 (núcleos vetorizados) e os 19 de `tests/execucao`; saída idêntica à do PC | 5 + 5 + 19 ok |

Cada `.esperado` foi conferido à mão (valores calculados independentemente).
Os `.esperado` usam fim de linha LF (`.gitattributes`). No Windows, o `\r`
que o modo texto acrescenta é retirado antes da comparação, então os mesmos
arquivos valem em Linux, macOS e Windows.

## Benchmark (`make bench`)

Gera os núcleos com o compilador, mede C0, C1, C1-fastmath, C2, C2+C1 e C3
(1 a 20 threads) com N = 10⁶ e 10⁷ (mediana de 5), coleta `-Rpass` e o IR, e
escreve `docs/bench/resultados.md` e os gráficos. Análise em
[`docs/relatorio_otimizacao.md`](docs/relatorio_otimizacao.md).

## Documentação

| Documento | Conteúdo |
| --- | --- |
| [`docs/decisoes.md`](docs/decisoes.md) | Decisões D01–D47 (PjBL 1, correções, PjBL 3/4) |
| [`docs/decisoes_backend.md`](docs/decisoes_backend.md) | PjBL 3: tipos, construções, portabilidade, lacunas da Seção 4.3, análise de mem2reg/phi e -O2 |
| [`docs/dsl.md`](docs/dsl.md) | PjBL 4 A: especificação formal da DSL vetorial e lacunas da Seção 5.5 |
| [`docs/relatorio_otimizacao.md`](docs/relatorio_otimizacao.md) | PjBL 4 B e C: estratégia híbrida, pipeline, núcleo em três níveis, medições e pergunta obrigatória |
| [`docs/placa.md`](docs/placa.md) | PjBL 4 D: Arduino Uno, ferramentas, comandos, decisões do alvo, memória, tamanhos, vídeo |
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
| 4. Placa | Arduino Uno: `make placa`/`gravar`, runtime da placa, programas com saída idêntica no Uno emulado (5 da placa em C0 e C2, mais os 19 do PjBL 3), E/S física, verificação de memória, `duplocarpado` de 64 bits, núcleos MLIR na placa, textos da biblioteca na flash (`placa.md`) |
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
- No Uno, a profundidade de recursão não é verificada estaticamente.
- A linguagem não tem comentários.
