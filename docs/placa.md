# Execução em placa: Arduino Uno (PjBL 4, Parte D)

## 1. Placa e alvo

| Item | Valor |
| --- | --- |
| Placa | Arduino Uno R3 |
| Microcontrolador | ATmega328P, AVR de 8 bits, 16 MHz, sem sistema operacional |
| Memória | 32 KB de flash (0,5 KB do bootloader), **2 KB de RAM**, 1 KB de EEPROM |
| Tripla / CPU | `llc -mtriple=avr -mcpu=atmega328p` |
| Executável entregue | `placa/<programa>.hex` (Intel HEX), gerado de `build/placa/<programa>.elf` |
| Gravação | `avrdude` pelo bootloader do Arduino (USB, 115200 baud) |
| E/S | USART0 a 9600 baud pela mesma porta USB; LED do pino 13 |

## 2. Ferramentas e versões

| Ferramenta | Versão | Uso |
| --- | --- | --- |
| LLVM `llc` | 23.1.5 | IR do compilador → objeto AVR |
| avr-gcc | 16.2.0 | compila a biblioteca de execução e liga tudo |
| avr-libc | 2.3.2 | biblioteca C do AVR (com `double` de 64 bits) |
| avrdude | 8.3 | grava o `.hex` na placa |
| QEMU (`qemu-system-avr -machine uno`) | 11.1.3 | testes sem hardware (`make test`) |

No Windows todas vêm do MSYS2 UCRT64
(`pacman -S mingw-w64-ucrt-x86_64-{llvm,avr-gcc,avr-libc,avrdude,qemu}`); no
Ubuntu, de `apt install llvm gcc-avr avr-libc avrdude qemu-system-misc`.

## 3. Do programa ao executável

O executável é produzido **a partir do mesmo `.ll` do PC**; só a biblioteca de
execução da placa (`runtime/zl_runtime_uno.c`) é escrita à mão.

```sh
make placa PROG=tests/placa/01_laco_vetor.mac     # gera placa/01_laco_vetor.hex
make gravar PROG=tests/placa/01_laco_vetor.mac PORTA=COM3   # Windows
make gravar PROG=tests/placa/01_laco_vetor.mac PORTA=/dev/ttyACM0  # Linux
```

O que `make placa` faz (`scripts/construir.sh programa.mac build/placa uno`):

```sh
./macaronica programa.mac -o programa.ll --alvo=uno       # IR portavel + verificacao de RAM
llvm-as programa.ll && opt -passes=verify programa.ll      # IR valido
sh scripts/pipeline.sh programa.mlir programa.nucleos.ll   # so se usar a DSL vetorial
llvm-link -S programa.ll programa.nucleos.ll -o programa.final.ll
llc -mtriple=avr -mcpu=atmega328p -O2 -filetype=obj programa.final.ll -o programa.o
avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -mdouble=64 -Os \
    programa.o runtime/zl_runtime_uno.c -lm -Wl,--gc-sections -o programa.elf
avr-objcopy -O ihex -R .eeprom programa.elf programa.hex
```

Gravação manual (o que `make gravar` executa):

```sh
avrdude -c arduino -p atmega328p -P COM3 -b 115200 -U flash:w:placa/programa.hex:i
```

A porta aparece no Gerenciador de Dispositivos do Windows ("Portas (COM e
LPT)") ou em `ls /dev/ttyACM*` no Linux. Para ver a saída, abra um terminal
serial a **9600 baud** (Monitor Serial da IDE do Arduino, PuTTY ou
`python -m serial.tools.miniterm COM3 9600`). Abrir o monitor reinicia a placa,
então o programa roda de novo desde o começo.

## 4. Biblioteca de execução da placa

`runtime/zl_runtime_uno.c` implementa os ganchos de `zl_plataforma.h`. A
formatação e a leitura vêm de `zl_runtime_comum.c`, o mesmo arquivo do PC,
que ele inclui com `#include`. Por isso o `avr-gcc` recebe um único arquivo de
biblioteca.

| Função | Na placa |
| --- | --- |
| `main` | inicia a USART (9600 8N1), chama `zl_principal`, acende o LED e dorme |
| `imprime` | bytes pela serial (`\n` como fim de linha, igual ao PC) |
| `le` | lê da serial (bloqueia até chegar o valor) |
| erro de execução | mensagem `Erro de execucao (linha L): ...` pela serial e **LED do pino 13 piscando rápido** para sempre |
| `pinomodo(p, m)` | `m` = 0 entrada, 1 saída, 2 entrada com pull-up (registradores DDR/PORT) |
| `pinoescreve(p, v)` | nível 0 ou 1 no pino |
| `pinole(p)` | lê o nível (0 ou 1) |
| `espera(ms)` | espera ativa com `_delay_ms(1)` |

Fim normal do programa: o LED fica **aceso fixo**. Assim, sem terminal, dá
para saber se o programa terminou (aceso) ou parou com erro (piscando).

## 5. Decisões dependentes do alvo

| Decisão | Escolha e justificativa |
| --- | --- |
| `inteira` no processador de 8 bits | continua `i32` (D45): o resultado é o mesmo do PC, a um custo de mais instruções |
| `duplocarpado` | `double` IEEE de 64 bits também na placa, com `avr-gcc -mdouble=64` (a libgcc traz as rotinas de 64 bits). Não foi preciso rejeitar nem rebaixar o tipo. Custo: ~1,5 KB de flash |
| Numeração dos pinos | a da **própria placa**: 2-13 digitais, 14-19 = A0-A5. 0 e 1 são RX/TX da serial e são recusados (erro 7). Tabela pino → registrador em `zl_runtime_uno.c` |
| Formato da saída | igual ao PC: a formatação é a mesma e usa `double` de 64 bits nos dois |
| Textos | os do programa ficam na RAM (o AVR copia `.data` da flash na partida) e contam na verificação de memória; os da biblioteca ficam na flash (Seção 8) |
| `le` de `palavra` | 4 buffers rotativos de 32 bytes (sem `malloc` em 2 KB) |
| Profundidade de recursão | não é verificada estaticamente: a pilha divide os 512 bytes reservados com a biblioteca. `mdc`, `hanoi(16)` e potência recursiva rodam sem problema |
| Núcleos da DSL vetorial | rodam na placa (`tests/placa/04_dsl_e_duplo.mac`): o mesmo IR, com `<8 x float>` legalizado em operações escalares. Sem SIMD, não há ganho de desempenho |

## 6. Verificação de memória por alvo (`--alvo=uno`, D45)

Com `--alvo=uno`, a semântica soma o tamanho de cada variável **no AVR**
(`bool` 1 byte, `inteira`/`flut` 4, `duplocarpado` 8, `palavra` 2, registro =
soma dos campos, vetor = N × elemento):

- **globais e textos** ocupam a RAM o tempo todo;
- o **quadro de cada função** é a soma de **todas** as suas declarações locais
  (inclusive as de blocos internos, porque o gerador põe todos os `alloca` no
  bloco de entrada) mais os parâmetros.

Erro semântico `SEM_TARGET_MEMORY` quando

```text
globais + textos + quadro(f)  >  2048 - 512 bytes
```

para alguma função `f` (512 bytes ficam para a pilha de chamadas, a USART e
os buffers da biblioteca). O relatório mostra a estimativa:

```text
[ALVO UNO]
Memoria estimada: globais e textos 800 bytes; maior quadro de funcao 1208 bytes;
limite 1536 bytes (2048 de RAM - 512 reservados)
```

`tests/placa/invalidos/01_memoria_excedida.mac` (vetor global de 200 `inteira`
e local de 300 `flut`) é rejeitado com `--alvo=uno` e aceito para o PC.

## 7. Programas e testes

| Programa | O que mostra |
| --- | --- |
| `tests/placa/01_laco_vetor.mac` | laço e vetor: os 10 primeiros primos e a soma |
| `tests/placa/02_recursao.mac` | funções recursivas (`mdc`, Hanói) e `duplocarpado` de 64 bits |
| `tests/placa/03_erro_execucao.mac` | erro de execução da linguagem (divisão por zero): mensagem + LED piscando |
| `tests/placa/04_dsl_e_duplo.mac` | núcleos da DSL vetorial gerados via MLIR rodando no AVR |
| `tests/placa/05_pisca_led.mac` | **E/S física**: pisca o LED do pino 13 três vezes |
| `tests/placa/manual/06_botao_acende.mac` | **E/S física**: acende o LED enquanto um botão no pino 2 (ligado ao GND, pull-up interno) estiver pressionado. Laço infinito: só no hardware |
| `tests/placa/invalidos/01_memoria_excedida.mac` | rejeitado pela verificação de memória do alvo |

`make test` executa cada programa no PC (lli e clang) **e** no Uno emulado
pelo QEMU, construído a partir do mesmo `.ll`, e exige saída **idêntica**
(`tests/run_placa.py`). No Uno emulado rodam três baterias:

| Bateria | Resultado |
| --- | --- |
| `tests/placa` em C0 | 5 ok |
| `tests/placa` em **C2** (núcleos da DSL vetorizados pelo MLIR) | 5 ok |
| os 19 programas de `tests/execucao` do PjBL 3 (com `le`, erros de execução, registros, recursão, `duplocarpado`) | 19 ok |

A única diferença esperada é a do programa de pinos. No PC as operações de
pino são simuladas e aparecem como `[pino 13 <- 1]`, por isso ele tem um
`.esperado_placa`.

A USART do QEMU fica ligada a um **socket TCP** local
(`-serial tcp:127.0.0.1:PORTA,server=on,wait=on`). O script envia o arquivo
`.entrada` e lê a resposta até o byte `0x04`, que a biblioteca escreve no fim
quando é compilada com `-DZL_SENTINELA` (só nos testes). O TCP funciona igual
em Linux e Windows. No Windows o QEMU não lê a serial de um pipe nem de
arquivo.

O QEMU não simula o resistor de pull-up interno: num pino de entrada
desconectado ele lê 0, enquanto a placa real lê 1. Por isso o programa do
botão fica em `manual/` e é demonstrado só no hardware.

## 8. Tamanho dos executáveis

Medido com `avr-size -C --mcu=atmega328p` (gerado por `tests/run_placa.py` em
`build/testes/placa-uno-C0/tamanhos.txt`):

| Programa | Flash (.text + .data) de 32 256 | RAM estática (.data + .bss) de 2048 |
| --- | ---: | ---: |
| 01_laco_vetor | 10 030 bytes (31,1 %) | 203 bytes (9,9 %) |
| 02_recursao | 10 254 bytes (31,8 %) | 133 bytes (6,5 %) |
| 03_erro_execucao | 9 726 bytes (30,2 %) | 163 bytes (8,0 %) |
| 04_dsl_e_duplo | 11 682 bytes (36,2 %) | 467 bytes (22,8 %) |
| 05_pisca_led | 9 330 bytes (28,9 %) | 153 bytes (7,5 %) |

Porcentagens sobre os 32 256 bytes de flash que sobram ao programa com o
bootloader do Uno (o `avr-size` calcula sobre os 32 768 da memória inteira).

A maior parte da flash (~9 KB) é da biblioteca: formatação com `double` de 64
bits (libf7 da libgcc) e divisão de 64 bits. O código gerado para o programa
em si tem poucas centenas de bytes. Da RAM estática, ~130 bytes são da
biblioteca (4 buffers de 32 bytes para `le` de `palavra`); o resto são as
variáveis globais e os textos do programa.

**Otimização dependente do alvo: textos da biblioteca na flash.** No AVR,
constantes ficam na RAM por padrão (o `.data` é copiado da flash na partida).
Os textos fixos da biblioteca (mensagens de erro, `VER`/`FAL`, `nan`, ...) são
marcados com `PROGMEM` e lidos com `pgm_read_byte` (macro `FIXO` em
`zl_runtime_comum.c`; no PC ela não faz nada). Isso economizou **240 bytes de
RAM em todo programa** (por exemplo, `01_laco_vetor` caiu de 443 para 203
bytes), sem mudar o IR gerado nem a saída. Os textos do próprio programa
continuam na RAM, porque uma `palavra` precisa poder apontar tanto para um
literal quanto para um texto lido por `le`.

## 9. Vídeo

*(Gravar antes da entrega: até 1 minuto mostrando `make placa` + `make
gravar`, a saída no Monitor Serial e o LED piscando. Colocar aqui o link ou o
arquivo.)*

## 10. Demonstração com o programa inédito

```sh
make                                              # compilador
./macaronica inedito.mac -o build/inedito.ll      # erros? aparecem aqui
make placa PROG=inedito.mac                       # placa/inedito.hex
make gravar PROG=inedito.mac PORTA=COM3           # grava; abrir o monitor a 9600
```

Se o programa declarar vetores grandes demais para 2 KB, `make placa` para com
`SEM_TARGET_MEMORY` e mostra a estimativa de memória. No PC, o mesmo
programa roda com `sh scripts/construir.sh inedito.mac build/pc pc` e
`build/pc/inedito.exe` (Linux: `build/pc/inedito`). Antes de gravar, dá para
conferir no Uno emulado:
`python3 tests/run_placa.py pasta_com_o_programa`.
