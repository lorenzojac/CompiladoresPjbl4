# Execução em placa: Raspberry Pi Pico 2 W (PjBL 4, Parte D)

A placa principal do grupo é a **Raspberry Pi Pico 2 W** (decisão D48). O
mesmo `.ll` gerado para o PC também roda no **Arduino Uno**, que fica como
segunda arquitetura e é testado no QEMU (Seção 12).

## 1. Placa e alvo

| Item | Valor |
| --- | --- |
| Placa | Raspberry Pi Pico 2 W |
| Microcontrolador | RP2350A: 2 núcleos ARM Cortex-M33 a 150 MHz (só o núcleo 0 é usado), FPU de precisão simples, sem sistema operacional |
| Memória | **520 KB de SRAM** (512 KB principais + 2 × 4 KB de rascunho), 4 MB de flash |
| Sem fio | CYW43439 (Wi-Fi e Bluetooth): não é usado, exceto pelo LED embutido, que é ligado a ele e não a um GPIO |
| Tripla / CPU | `llc -mtriple=thumbv8m.main-none-eabi -mcpu=cortex-m33 -float-abi=soft` |
| Executável entregue | `placa/<programa>.uf2`, gerado de `build/placa-pico/<programa>.elf` |
| Gravação | modo BOOTSEL: a placa aparece como a unidade USB `RP2350` e recebe o `.uf2`. Depois da primeira gravação, `make gravar` põe a placa nesse modo e grava sem o botão |
| E/S | USB da própria RP2350 (porta serial virtual, CDC); LED embutido = pino 25; GP0-GP22 e GP26-GP28 |

### 1.1 Por que a Pico 2 W

- Está na tabela de placas aceitas do enunciado (Raspberry Pi Pico / Pico 2:
  Cortex-M33, sem SO, `.uf2` pelo modo BOOTSEL).
- **USB nativa**: a própria RP2350 é o dispositivo USB, então a porta serial
  aparece sem instalar driver, no Windows e no Linux. O clone de Uno usado
  antes tem conversor CH340, que pede driver no Windows: um risco no PC da
  apresentação.
- **Gravação sem ferramenta**: no pior caso, basta copiar o `.uf2` para a
  unidade `RP2350`, como num pen drive.
- **Memória**: 520 KB de RAM contra 2 KB do Uno. O maior vetor passa de 384
  para ~114 mil `inteira` (Seção 6).
- **32 bits**: `inteira` (`i32`) tem o tamanho do registrador e `flut` usa a
  FPU. No AVR de 8 bits, cada operação de `inteira` vira várias instruções.
- O Uno continua como **segunda arquitetura** (AVR de 8 bits) executando o
  mesmo `.ll`, item opcional do nível Excelente.

## 2. Ferramentas e versões

| Ferramenta | Versão | Uso |
| --- | --- | --- |
| LLVM `llc` | 23.1.5 | IR do compilador → objeto ARM (Thumb-2) |
| Pico SDK | 2.3.1 | partida da placa, USB (TinyUSB), driver do CYW43439, `stdio`, script de ligação |
| arm-none-eabi-gcc | 16.2.0 | compila o SDK e a biblioteca de execução e liga tudo |
| arm-none-eabi-binutils | 2.47 | `ld`, `objcopy`, `size` |
| newlib (arm-none-eabi) | 4.6.0 | biblioteca C da placa |
| CMake / Ninja | 4.4.5 / 1.13.2 | sistema de construção do SDK |
| picotool | 2.3.1 | `.elf` → `.uf2` (chamado pelo SDK) |
| Python + pyserial | 3.14 / 3.5 | `scripts/pico.py` (gravar, monitor) e `tests/run_pico.py` |

No Windows todas vêm do MSYS2 UCRT64:

```sh
pacman -S mingw-w64-ucrt-x86_64-{arm-none-eabi-gcc,arm-none-eabi-binutils,arm-none-eabi-newlib,cmake,ninja,picotool,python-pyserial}
```

No Ubuntu/Debian:
`apt install gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib cmake ninja-build python3-serial`.
O picotool é compilado da fonte (como no `Dockerfile`); sem ele, o SDK baixa
e compila um na primeira configuração, o que exige internet.

Pico SDK (uma vez; ~75 MB, só os submódulos usados: USB e chip sem fio):

```sh
git clone --depth 1 --branch 2.3.1 https://github.com/raspberrypi/pico-sdk
cd pico-sdk && git submodule update --init --depth 1 lib/tinyusb lib/cyw43-driver
```

`scripts/pico_sdk.sh` procura o SDK, nesta ordem, em `$PICO_SDK_PATH`,
`../pico-sdk` (ao lado do projeto), `~/pico/pico-sdk`, `~/pico-sdk` e
`%USERPROFILE%\pico\pico-sdk`. No contêiner (`Dockerfile`), o SDK fica em
`/opt/pico-sdk` e o picotool em `/usr/local`.

As ferramentas do Uno (avr-gcc, avrdude, QEMU) estão na Seção 12.

## 3. Do programa ao executável

O executável é produzido **a partir do mesmo `.ll` do PC**; só a biblioteca de
execução da placa (`runtime/zl_runtime_pico.c`) é escrita à mão.

```text
programa.mac
  | ./macaronica --alvo=pico          analise + verificacao de RAM do alvo
  v
programa.ll  (+ programa.mlir se usar a DSL vetorial)
  | llvm-as, opt -passes=verify       IR valido
  | pipeline.sh + llvm-link           nucleos MLIR unidos ao programa
  v
programa.final.ll                     o mesmo IR do PC e do Uno
  | llc -mtriple=thumbv8m.main-none-eabi -mcpu=cortex-m33
  v
programa.o + zl_runtime_pico.c + Pico SDK    (CMake + Ninja + arm-none-eabi-gcc)
  v
programa.elf --picotool--> programa.uf2 --> unidade RP2350 (BOOTSEL) --> placa
```

Comandos:

```sh
make placa PROG=tests/placa/01_laco_vetor.mac     # gera placa/01_laco_vetor.uf2
make gravar PROG=tests/placa/01_laco_vetor.mac    # grava (PORTA=COM5 opcional)
make monitor                                      # abre o terminal: o programa comeca
```

`PLACA ?= pico` no Makefile; `make placa PLACA=uno ...` gera o `.hex` do Uno
(Seção 12).

O que `make placa` faz (`scripts/construir.sh programa.mac build/placa-pico pico`):

```sh
./macaronica programa.mac -o programa.ll --alvo=pico -q   # IR portavel + verificacao de RAM
llvm-as programa.ll && opt -passes=verify programa.ll     # IR valido
sh scripts/pipeline.sh programa.mlir programa.nucleos.ll  # so se usar a DSL vetorial
llvm-link -S programa.ll programa.nucleos.ll -o programa.final.ll
llc -mtriple=thumbv8m.main-none-eabi -mcpu=cortex-m33 -float-abi=soft -O2 \
    -function-sections -data-sections -filetype=obj programa.final.ll -o programa.o
cp programa.o build/pico-OFF/programa.o
cmake -S runtime/pico -B build/pico-OFF -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DPICO_SDK_PATH=... -DZL_PROGRAMA_OBJ=build/pico-OFF/programa.o  # so na 1a vez
cmake --build build/pico-OFF    # programa.o + zl_runtime_pico.c + SDK -> programa.elf, .uf2
```

- `runtime/pico/CMakeLists.txt` usa a placa `pico2_w` do SDK e liga
  `pico_stdlib`, `pico_cyw43_arch_none` (só o LED, sem rede) e
  `hardware_watchdog`; o `stdio` vai só pela USB (a UART fica desligada).
- A configuração do CMake é lenta (compila partes do SDK) e é feita **uma vez
  por pasta**: `build/pico-OFF` para `make placa` e `build/pico-ON` para os
  testes automáticos (com `-DZL_SENTINELA`). Depois, cada programa só troca o
  `programa.o` e religa. Tempo de `make placa` com a pasta já configurada:
  ~1,3 s; da primeira vez: ~1 min (compila o `pioasm` e ~110 arquivos do SDK).
- No Windows, o GNU ld 2.47 do MSYS2 recusa mais de um arquivo de script de
  ligação, e o SDK usa uns 20. `runtime/pico/achata_ld.py` gera um script
  único com o mesmo texto (opção `ZL_LINKER_UNICO`, ligada só no Windows).
- Logs: `build/placa-pico/<programa>.cmake.log` (configuração) e
  `build/placa-pico/<programa>.ld.log` (compilação e ligação).

### 3.1 Gravação

`make gravar PROG=x.mac [PORTA=COMx]` executa
`python scripts/pico.py gravar placa/x.uf2`.

- **Primeira vez** (a placa ainda não tem um programa nosso): segurar o botão
  **BOOTSEL** enquanto liga o cabo USB e soltar quando a unidade `RP2350`
  aparecer; depois rodar `make gravar`.
- **Depois**: o programa gravado tem a porta USB, e abri-la a 1200 baud
  reinicia a placa no modo BOOTSEL (recurso do SDK). `make gravar` faz isso e
  grava o novo `.uf2` sem apertar o botão.
- **Manual** (plano B, sempre funciona): com a placa em BOOTSEL, copiar
  `placa/x.uf2` para a unidade `RP2350`. A placa reinicia sozinha e roda o
  programa.

### 3.2 Terminal

`make monitor [PORTA=COMx]` executa `python scripts/pico.py monitor`.
Qualquer terminal serial também serve: Monitor Serial da IDE do Arduino,
PuTTY ou `python -m serial.tools.miniterm COM5`. Como a porta é USB, a
velocidade não importa, **exceto 1200 baud**, que reinicia a placa em
BOOTSEL.

A porta aparece no Gerenciador de Dispositivos do Windows ("Portas (COM e
LPT)", driver nativo, nada a instalar) ou como `/dev/ttyACM0` no Linux. O
programa **só começa quando um terminal abre a porta** (sinal DTR), para nada
da saída se perder. **Fechar o terminal reinicia a placa**, e abrir de novo
roda o programa desde o começo, como no Uno, que reinicia quando o Monitor
Serial abre. Para `le`, digite o valor e Enter.

## 4. Biblioteca de execução da placa

`runtime/zl_runtime_pico.c` implementa os ganchos de `zl_plataforma.h`. A
formatação e a leitura vêm de `zl_runtime_comum.c`, o mesmo arquivo do PC e
do Uno, que ele inclui com `#include`. Por isso o CMake recebe um único
arquivo de biblioteca.

| Função | Na placa |
| --- | --- |
| `main` | inicia a USB (`stdio_init_all`) e o CYW43439 (LED), espera o terminal, chama `zl_principal` |
| espera do terminal | LED pisca devagar até um terminal abrir a porta. Se nenhum PC reconhecer a placa em 3 s (carregador ou bateria), o programa roda sem terminal |
| `imprime` | bytes pela USB (`putchar` do SDK, que troca `\n` por `\r\n` para os terminais) |
| `le` | lê da USB (bloqueia até chegar o valor). Sem terminal, vê fim da entrada: erro `entrada invalida ou ausente` |
| texto lido por `le` | cópia com `malloc` (heap do SDK) |
| erro de execução | mensagem `Erro de execucao (linha L): ...` pela USB e **LED piscando rápido** para sempre |
| fim normal | LED **aceso fixo** |
| `pinomodo(p, m)` | `m` = 0 entrada, 1 saída, 2 entrada com pull-up (`gpio_init`, `gpio_set_dir`, `gpio_pull_up`). No pino 25 é aceito e ignorado: o LED é sempre saída |
| `pinoescreve(p, v)` | `gpio_put`; no pino 25, `cyw43_arch_gpio_put` no LED |
| `pinole(p)` | `gpio_get`; no pino 25, o último valor escrito |
| `espera(ms)` | `sleep_ms` em fatias de 20 ms, conferindo se o terminal foi fechado |
| pino inválido | erro de execução 7, `pino ou modo invalido` |
| `-DZL_SENTINELA` | só nos testes: o byte `0x04` (EOT) marca o fim do programa |

Sem terminal, o LED mostra o estado da placa:

| LED | Significado |
| --- | --- |
| pisca devagar (troca a cada 0,5 s) | programa gravado, **esperando um terminal** abrir a porta |
| apagado | programa rodando (se ele não usar o pino 25) ou placa em BOOTSEL (unidade `RP2350` aberta) |
| aceso fixo | o programa terminou normalmente |
| pisca rápido (troca a cada 0,1 s) | erro de execução; a mensagem saiu pela USB |

## 5. Decisões dependentes do alvo

| Decisão | Escolha e justificativa |
| --- | --- |
| `inteira` | `i32`, o tamanho do registrador do Cortex-M33 (no Uno também é `i32`, com mais instruções; D45) |
| `flut` | `float` IEEE de 32 bits, na FPU |
| `duplocarpado` | `double` IEEE de 64 bits, igual ao PC. A FPU só tem precisão simples, então as operações de 64 bits viram chamadas `__aeabi_d*`, implementadas pelo Pico SDK (`pico_double`). Não foi preciso rejeitar nem rebaixar o tipo |
| ABI de ponto flutuante | `-float-abi=soft` com FPU, ou seja *softfp*, a mesma que o SDK usa na RP2350: `float` e `double` passam em registradores inteiros entre funções, e a FPU é usada dentro delas. Com `hard`, o código gerado e a biblioteca discordariam sobre onde ficam os argumentos |
| Numeração dos pinos | a da **própria placa**: os números GP do SDK, não a posição no conector. 25 = LED embutido (Seção 5.1) |
| Pinos no PC | o PC simula a numeração da Pico (`zl_pino_valido` em `zl_runtime_posix.c`), então o mesmo programa roda no PC e na placa. O runtime do Uno mantém a numeração do Uno (2-19) |
| Formato da saída | igual ao PC: a formatação é a mesma e usa `double` de 64 bits nos dois |
| Textos | os literais do programa são constantes do IR e ficam na flash (`.rodata`, lida direto pela XIP). A verificação de memória os conta mesmo assim, uma estimativa conservadora. Textos lidos por `le` vão para o heap |
| `palavra` | ponteiro de 4 bytes (2 no Uno) |
| Profundidade de recursão | não é verificada estaticamente. A pilha do núcleo 0 começa no topo da SRAM e cresce para baixo, em direção ao heap, sem guarda; ela divide os 64 KB reservados com o heap e o SDK |
| Núcleos da DSL vetorial | o mesmo IR, com `<8 x float>` legalizado em operações escalares da FPU (`tests/placa/04_dsl_e_duplo.mac`). O Cortex-M33 não tem SIMD de ponto flutuante (a extensão DSP só tem SIMD de inteiros de 8 e 16 bits), então não há ganho de desempenho |
| Paralelismo (Parte C) | a configuração C3 (OpenMP) é só do PC: a placa não tem SO nem `libomp`, e o segundo núcleo não é usado. Na placa, os núcleos usam C0 ou C2 |
| Wi-Fi e Bluetooth | não usados: a linguagem não tem rede. O CYW43439 só é ligado por causa do LED |

Lacunas da Seção 5.5 do enunciado (a rubrica a chama de 5.6) para a placa:
o maior vetor está na Seção 6; os pinos usam a numeração da placa (Seção
5.1); sem SIMD, os núcleos vetoriais viram código escalar (tabela acima).

### 5.1 Numeração dos pinos

| Pino na linguagem | Na Pico 2 W | Uso |
| --- | --- | --- |
| 0-22 | GP0-GP22 | E/S digital. GP0 e GP1 ficam livres, porque a saída não usa a UART |
| 23, 24 | — | ligados ao CYW43439: **erro 7** |
| 25 | LED embutido | ligado ao CYW43439, não a um GPIO; só saída. É o número do LED na Pico sem fio |
| 26-28 | GP26-GP28 | E/S digital (também são ADC0-ADC2, mas a linguagem só tem E/S digital) |
| 29 | — | ligado ao CYW43439: **erro 7** |
| negativos ou maiores que 29 | — | **erro 7** |

Posições no conector, para a montagem dos programas de E/S: GP2 = pino
físico 4, GP13 = 17, GP15 = 20; GND = 3, 8, 13, 18, 23, 28, 33 e 38. Um LED
externo fica com um resistor (~330 Ω) entre o GP13 e o GND. Um botão fica
entre o GP15 e o GND, usando o pull-up interno (`pinomodo(15, 2)`): solto lê
1, pressionado lê 0.

## 6. Verificação de memória por alvo (`--alvo=pico`, D48)

Mesmo método do Uno (D45), com os tamanhos da Pico: `bool` 1 byte,
`inteira`/`flut` 4, `duplocarpado` 8, `palavra` 4, registro = soma dos campos,
vetor = N × elemento.

- **Globais e textos** ocupam a RAM o tempo todo.
- O **quadro de cada função** é a soma de **todas** as suas declarações
  locais mais os parâmetros. Entram também as de blocos internos, porque o
  gerador põe todos os `alloca` no bloco de entrada.

Erro semântico `SEM_TARGET_MEMORY` quando

```text
globais + textos + quadro(f)  >  524288 - 65536 = 458752 bytes
```

para alguma função `f`. 524 288 bytes são os 512 KB de SRAM principal (os
2 × 4 KB de rascunho não entram). Os 65 536 reservados ficam para o Pico SDK,
a pilha USB, o driver do CYW43439, o heap e a pilha de chamadas. O
relatório mostra a estimativa:

```text
[ALVO PICO 2 W]
Memoria estimada: globais e textos 800 bytes; maior quadro de funcao 1208 bytes; limite 458752 bytes (524288 de RAM - 65536 reservados)
```

Esse é `tests/placa/invalidos/01_memoria_excedida.mac` (vetor global de 200
`inteira` e local de 300 `flut`): **rejeitado com `--alvo=uno`** e **aceito
com `--alvo=pico`**. Para ser rejeitado na Pico, um programa precisa passar
de 448 KB. Um vetor global de 120 000 `inteira` (480 000 bytes), por exemplo,
dá:

```text
codigo: SEM_TARGET_MEMORY
mensagem: variaveis globais e textos excedem a RAM do alvo pico (524288 bytes, 65536 reservados para SDK, USB e pilha)
[ALVO PICO 2 W]
Memoria estimada: globais e textos 480000 bytes; maior quadro de funcao 0 bytes; limite 458752 bytes (524288 de RAM - 65536 reservados)
```

**Maior vetor** (lacuna da Seção 5.5): 458 752 / 4 = **114 688 `inteira`** ou
`flut`, ou 57 344 `duplocarpado`, se o resto do programa não ocupar nada. No
Uno, o limite é 384 `inteira`.

## 7. Programas e testes

| Programa | O que mostra |
| --- | --- |
| `tests/placa/01_laco_vetor.mac` | laço e vetor: os 10 primeiros primos e a soma |
| `tests/placa/02_recursao.mac` | funções recursivas (`mdc`, Hanói) e `duplocarpado` de 64 bits |
| `tests/placa/03_erro_execucao.mac` | erro de execução da linguagem (divisão por zero): mensagem + LED piscando rápido |
| `tests/placa/04_dsl_e_duplo.mac` | núcleos da DSL vetorial gerados via MLIR |
| `tests/placa/05_pisca_led.mac` | **E/S física**: pisca o pino 13 três vezes. Na Pico, o pino 13 é o GP13, sem LED na placa: precisa de um LED externo |
| `tests/placa/pico/05_pisca_led_pico.mac` | **E/S física**: o mesmo, no LED embutido (pino 25) |
| `tests/placa/manual/06_botao_acende.mac` | **E/S física**: acende o pino 13 enquanto um botão no pino 2 (ligado ao GND, pull-up interno) estiver pressionado. Na Pico: botão no GP2, LED externo no GP13. Laço infinito: só no hardware |
| `tests/placa/pico/manual/06_botao_pico.mac` | **E/S física**: acende o LED (pino 25) enquanto um botão entre o GP15 e o GND (pull-up interno) estiver pressionado. Laço infinito: só no hardware |
| `tests/placa/pico/manual/07_led_liga.mac` | **E/S física**, para ver a olho nu: LED da placa aceso 3 s, apagado 2 s, 5 piscadas de 1 s, aceso no fim. Visto na placa |
| `tests/placa/pico/manual/08_led_externo.mac` | **E/S física**: LED externo no GP13 (GP13 → resistor de 220–330 Ω → LED → GND): aceso 3 s, 5 piscadas e depois alternando com o LED da placa |
| `tests/placa/invalidos/01_memoria_excedida.mac` | rejeitado pela verificação de memória com `--alvo=uno`; aceito na Pico (Seção 6) |
| `tests/placa/pico/invalidos/01_memoria_excedida_pico.mac` | vetor de 120 000 `inteira` (480 000 bytes): rejeitado com `--alvo=pico` (`SEM_TARGET_MEMORY`), válido no PC |

No PC (`make test`), os programas rodam com os pinos da Pico simulados. A
única diferença esperada entre PC e placa é a dos programas de pinos: no PC
as operações aparecem como `[pino 13 <- 1]`, por isso eles têm um
`.esperado_placa`.

### 7.1 Testes automáticos na placa

```sh
make test-pico    # python tests/run_pico.py tests/placa (placa ligada na USB)
```

Precisa da placa de verdade, por isso não faz parte de `make test`. Os
programas são construídos com `-DZL_SENTINELA` (pasta `build/pico-ON`) e
gravados pela USB. A saída é lida até o byte `0x04` e comparada com a do PC.

| Bateria | Resultado |
| --- | --- |
| `tests/placa` (C0 e C2) e `tests/placa/pico` (C0) | 11 ok, 0 falhas |
| `tests/execucao` (C0, os 19 programas do PjBL 3, inclusive `le` pela USB) | 19 ok, 0 falhas |
| `tests/placa/pico/05_pisca_led_pico.mac` (LED embutido piscando, visto na placa) | ok: 3 piscadas e LED fixo no fim |
| `tests/placa/pico/manual/06_botao_pico.mac` (botão no GP15, manual) | pendente: precisa de um botão ligado ao GP15 |

A bateria do Uno emulado (`make test-placa-uno`) continua valendo para a
segunda arquitetura (Seção 12).

## 8. Tamanho dos executáveis

Medido com `arm-none-eabi-size` (C0; flash = `text` + `data`; RAM estática =
`data` + `bss`). A coluna "código do programa" é o objeto que o `llc` gera a
partir do nosso `.ll` (`build/placa-pico/<programa>.o`); o resto é o SDK, a
pilha USB, a biblioteca de execução e o firmware do chip sem fio:

| Programa | Código do programa (.o) | Flash de 4 194 304 | RAM estática de 524 288 |
| --- | ---: | ---: | ---: |
| 01_laco_vetor | 290 B (+40 B de globais) | 277 812 (6,6 %) | 5 108 (1,0 %) |
| 02_recursao | 350 B | 279 540 (6,7 %) | 5 068 (1,0 %) |
| 03_erro_execucao | 211 B (+16 B) | 277 732 (6,6 %) | 5 084 (1,0 %) |
| 04_dsl_e_duplo | 690 B (+320 B) | 280 796 (6,7 %) | 5 388 (1,0 %) |
| 05_pisca_led | 143 B | 277 900 (6,6 %) | 5 068 (1,0 %) |
| pico/05_pisca_led_pico | 143 B | 277 900 (6,6 %) | 5 068 (1,0 %) |

Quase toda a flash é fixa: só o firmware do CYW43439
(`w43439A0_7_95_49_00_combined`, que a placa carrega no chip para acender o
LED) ocupa 225 240 bytes, 81 % do executável. O código gerado pelo compilador
tem de 143 a 690 bytes, como no Uno. Em C2, o `04_dsl_e_duplo` tem 281 716
bytes, 920 a mais: o Cortex-M33 não tem SIMD de `float` (a extensão DSP só
tem SIMD de inteiros de 8 e 16 bits), então o `llc` quebra cada operação
vetorial do MLIR em operações escalares. Os textos do programa ficam na flash
(Seção 5), então não há equivalente à otimização `PROGMEM` do Uno. O `.uf2`
tem cerca do dobro do tamanho (548 KB), porque cada bloco de 512 bytes leva
256 bytes de dados.

## 9. Vídeo

*(Gravar antes da entrega: até 1 minuto mostrando `make placa` + `make
gravar`, a saída no terminal (`make monitor`) e o LED da Pico 2 W piscando.
Colocar aqui o link ou o arquivo.)*

## 10. Demonstração com o programa inédito

Antes da apresentação, no notebook que vai ser usado:

- [ ] `make` e `make test` passam.
- [ ] `sh scripts/pico_sdk.sh` imprime o caminho do SDK.
- [ ] `make placa PROG=tests/placa/01_laco_vetor.mac` rodou pelo menos uma
  vez, deixando `build/pico-OFF` configurada. No dia, só religa: não apagar
  essa pasta.
- [ ] A placa já tem um programa nosso gravado, para `make gravar` funcionar
  sem o botão BOOTSEL. `make gravar` e `make monitor` foram testados, e a
  porta COM foi anotada.
- [ ] Levar a Pico 2 W e um **cabo micro-USB de dados**, mais um reserva
  (cabo só de carga não mostra nem a unidade nem a porta). Para E/S, levar a
  protoboard com LED no GP13 e botão no GP15.
- [ ] Os `.uf2` dos programas de teste estão em `placa/` (plano B: copiar
  para a unidade `RP2350`).

Na hora:

```sh
make                                                    # compilador
./macaronica inedito.mac -o build/inedito.ll --alvo=pico  # erros e estimativa de memoria
sh scripts/construir.sh inedito.mac build/pc pc         # executavel do PC
build/pc/inedito.exe                                    # Linux: build/pc/inedito
make placa PROG=inedito.mac                             # placa/inedito.uf2
make gravar PROG=inedito.mac                            # PORTA=COMx se precisar
make monitor                                            # o programa comeca aqui
```

- A saída do terminal deve ser igual à do PC, fora as linhas `[pino ...]`,
  que só o PC mostra.
- Se o programa usa `le`, digite os valores no terminal, cada um seguido de
  Enter.
- Se usa pinos, valem 0-22, 25 (LED) e 26-28. Outro número dá erro 7 na placa
  **e** no PC.
- Se `make placa` parar com `SEM_TARGET_MEMORY`, o programa passa de 448 KB de
  variáveis. Esse é o comportamento pedido; mostrar a estimativa.
- Para rodar de novo, feche e abra o terminal (a placa reinicia).

## 11. Problemas comuns

| Sintoma | Causa provável e solução |
| --- | --- |
| A unidade `RP2350` não aparece | Segure **BOOTSEL** *antes* de ligar o cabo e só solte quando a unidade aparecer. Confira se o cabo é de dados e tente outra porta USB, sem hub |
| A porta COM não aparece | A porta só existe enquanto um programa nosso está rodando, porque a USB é do programa e não de um chip conversor. Se a placa está em BOOTSEL, aparece a unidade `RP2350` no lugar da porta: grave um `.uf2`. Logo depois de gravar, a porta leva alguns segundos para voltar. Windows: "Portas (COM e LPT)" no Gerenciador de Dispositivos. Linux: `ls /dev/ttyACM*`, com o usuário no grupo `dialout`. Se nada aparece, suspeite do cabo |
| LED piscando devagar, nada acontece | O programa está **esperando o terminal**: abra `make monitor` (ou o Monitor Serial/PuTTY na porta da placa) |
| LED piscando rápido | Erro de execução, e a mensagem saiu pela USB. Se o terminal não estava aberto, feche e abra de novo: o programa roda outra vez e repete o erro |
| LED aceso fixo, terminal vazio | O programa já terminou, por exemplo ligado sem PC. Feche e abra o terminal para rodar de novo |
| Ao abrir o terminal, a unidade `RP2350` aparece | O terminal abriu a porta a 1200 baud, que reinicia a placa em BOOTSEL. Use outra velocidade e grave de novo |
| `make gravar` falha com a porta ocupada | Outro programa (monitor, IDE do Arduino, PuTTY) está com a porta aberta: feche-o. Plano B: BOOTSEL e copiar o `.uf2` |
| `le` não termina | Falta Enter depois do valor. No Monitor Serial da IDE do Arduino, escolha o fim de linha "Nova linha" |
| `Pico SDK nao encontrado` | Defina `PICO_SDK_PATH` ou clone o SDK num dos lugares da Seção 2 |
| Erro do CMake ou de ligação | Veja `build/placa-pico/<programa>.cmake.log` ou `.ld.log`. Depois de trocar o SDK ou a toolchain, apague `build/pico-OFF` e `build/pico-ON` para reconfigurar |

## 12. Alvo secundário: Arduino Uno (QEMU)

O Arduino Uno foi a primeira placa do grupo. Ele continua como **segunda
arquitetura** (AVR de 8 bits, ATmega328P) que executa o mesmo `.ll`, testada
sem hardware no QEMU.

| Item | Valor |
| --- | --- |
| Placa | Arduino Uno R3 (clone com conversor USB-serial CH340) |
| Microcontrolador | ATmega328P, AVR de 8 bits, 16 MHz, sem sistema operacional |
| Memória | 32 KB de flash (0,5 KB do bootloader), **2 KB de RAM**, 1 KB de EEPROM |
| Tripla / CPU | `llc -mtriple=avr -mcpu=atmega328p` |
| Executável | `placa/<programa>.hex` (Intel HEX) |
| Gravação | `avrdude` pelo bootloader do Arduino (USB, 115200 baud) |
| E/S | USART0 a 9600 baud; LED no pino 13; pinos 2-13 e 14-19 (= A0-A5). 0 e 1 são da serial (erro 7) |
| Ferramentas | avr-gcc 16.2.0, avr-libc 2.3.2, avrdude 8.3, QEMU 11.1.3 (`qemu-system-avr -machine uno`). MSYS2: `mingw-w64-ucrt-x86_64-{avr-gcc,avr-libc,avrdude,qemu}`; Ubuntu: `gcc-avr avr-libc avrdude qemu-system-misc` |

```sh
make placa PLACA=uno PROG=tests/placa/01_laco_vetor.mac             # placa/01_laco_vetor.hex
make gravar PLACA=uno PROG=tests/placa/01_laco_vetor.mac PORTA=COM3
make test-placa-uno                                                 # bateria no Uno emulado
```

O que `scripts/construir.sh programa.mac build/placa uno` faz depois das
etapas comuns (`--alvo=uno`, verificação do IR, núcleos MLIR):

```sh
llc -mtriple=avr -mcpu=atmega328p -O2 -filetype=obj programa.final.ll -o programa.o
avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -mdouble=64 -Os \
    programa.o runtime/zl_runtime_uno.c -lm -Wl,--gc-sections -o programa.elf
avr-objcopy -O ihex -R .eeprom programa.elf programa.hex
avrdude -c arduino -p atmega328p -P COM3 -b 115200 -U flash:w:placa/programa.hex:i  # gravar
```

Para ver a saída, o terminal fica a **9600 baud**; abrir o monitor reinicia a
placa.

Decisões próprias do Uno:

- `inteira` continua `i32` (D45): mesmo resultado do PC, com mais instruções.
- `duplocarpado` é `double` IEEE de 64 bits com `avr-gcc -mdouble=64` (a
  libgcc traz as rotinas de 64 bits), sem rejeitar nem rebaixar o tipo. Custa
  ~1,5 KB de flash.
- `runtime/zl_runtime_uno.c`: USART 9600 8N1, `espera` com `_delay_ms(1)`,
  `le` de `palavra` em 4 buffers rotativos de 32 bytes (sem `malloc` em 2 KB),
  e os mesmos sinais de LED (fixo = fim, piscando rápido = erro).
- Como o PC agora simula os pinos da Pico, os pinos 0, 1, 20-22 e 25-28 valem
  no PC e dão erro 7 no Uno. Os programas de `tests/placa` usam o pino 13 e o
  2, que valem nas duas placas.
- Verificação de memória (`--alvo=uno`, D45): `globais + textos + quadro(f) >
  2048 - 512` dá `SEM_TARGET_MEMORY`, com `palavra` de 2 bytes. Os 512 bytes
  ficam para a pilha, a USART e os buffers. `01_memoria_excedida.mac` é
  rejeitado:

  ```text
  [ALVO UNO]
  Memoria estimada: globais e textos 800 bytes; maior quadro de funcao 1208 bytes;
  limite 1536 bytes (2048 de RAM - 512 reservados)
  ```

- A recursão divide os 512 bytes reservados com a biblioteca: `mdc`,
  `hanoi(16)` e a potência recursiva rodam sem problema.
- Núcleos da DSL vetorial rodam no AVR emulado, com `<8 x float>` legalizado
  em operações escalares, sem ganho de desempenho.
- **Otimização dependente do alvo: textos da biblioteca na flash.** No AVR,
  constantes ficam na RAM por padrão. Os textos fixos da biblioteca
  (mensagens de erro, `VER`/`FAL`, `nan`, ...) são marcados com `PROGMEM` e
  lidos com `pgm_read_byte` (macro `FIXO` em `zl_runtime_comum.c`; no PC e na
  Pico ela não faz nada). Isso economiza **240 bytes de RAM em todo programa**
  (`01_laco_vetor` caiu de 443 para 203 bytes). Os textos do próprio programa
  continuam na RAM, porque uma `palavra` precisa poder apontar tanto para um
  literal quanto para um texto lido por `le`.

**Testes no QEMU** (`tests/run_placa.py`): cada programa roda no PC (lli e
clang) e no Uno emulado, construído a partir do mesmo `.ll`, e a saída precisa
ser **idêntica** (ou igual ao `.esperado_placa`, no programa de pinos).

| Bateria no Uno emulado | Resultado |
| --- | --- |
| `tests/placa` em C0 | 5 ok |
| `tests/placa` em **C2** (núcleos da DSL vetorizados pelo MLIR) | 5 ok |
| os 19 programas de `tests/execucao` do PjBL 3 (com `le`, erros de execução, registros, recursão, `duplocarpado`) | 19 ok |

A USART do QEMU fica ligada a um **socket TCP** local
(`-serial tcp:127.0.0.1:PORTA,server=on,wait=on`). O script envia o arquivo
`.entrada` e lê a resposta até o byte `0x04` (`-DZL_SENTINELA`). O TCP
funciona igual em Linux e Windows; no Windows o QEMU não lê a serial de um
pipe nem de arquivo. O QEMU não simula o pull-up interno (um pino de entrada
desconectado lê 0, e a placa real lê 1), por isso o programa do botão fica em
`manual/`.

**Tamanhos** (`avr-size -C --mcu=atmega328p`, gerado por `tests/run_placa.py`
em `build/testes/placa-uno-C0/tamanhos.txt`):

| Programa | Flash (.text + .data) de 32 256 | RAM estática (.data + .bss) de 2048 |
| --- | ---: | ---: |
| 01_laco_vetor | 10 030 bytes (31,1 %) | 203 bytes (9,9 %) |
| 02_recursao | 10 254 bytes (31,8 %) | 133 bytes (6,5 %) |
| 03_erro_execucao | 9 726 bytes (30,2 %) | 163 bytes (8,0 %) |
| 04_dsl_e_duplo | 11 682 bytes (36,2 %) | 467 bytes (22,8 %) |
| 05_pisca_led | 9 330 bytes (28,9 %) | 153 bytes (7,5 %) |

As porcentagens são sobre os 32 256 bytes que sobram com o bootloader. A
maior parte da flash (~9 KB) é da biblioteca: formatação com `double` de 64
bits (libf7 da libgcc) e divisão de 64 bits. O código do programa em si tem
poucas centenas de bytes.
