# Decisões do back-end (PjBL 3) — geração de LLVM IR

Este documento registra como o compilador traduz Macaronica para LLVM IR, as
lacunas do enunciado (Seção 4.3) e as decisões adicionais tomadas pelo grupo.
As decisões numeradas (D38 a D49) também estão resumidas em
[`decisoes.md`](./decisoes.md).

## 1. Ferramentas e forma de emissão

| Item | Escolha |
| --- | --- |
| LLVM / MLIR | 23.1.5 (MSYS2 UCRT64 no Windows; ver README para Linux) |
| Emissão do IR | **Texto**, montado em memória (`src/strbuf.c`) e escrito só no fim |
| Módulo | `src/codegen_llvm.c` (IR) e `src/codegen_mlir.c` (núcleos da DSL) |
| Entrada do gerador | AST **anotada** pela semântica: tipo de cada expressão, `symbol_id` de cada variável, índice de campo, função chamada |

A emissão textual (em vez da API C do LLVM) foi escolhida porque deixa o
compilador sem dependência de bibliotecas do LLVM em tempo de compilação, o
IR gerado é exatamente o que se lê no arquivo (bom para a arguição) e o mesmo
mecanismo serve para o MLIR, que não tem API C estável para todos os
dialetos usados.

O gerador nunca reanalisa nomes ou tipos: ele só lê as anotações. Escopos e
*shadowing* são resolvidos pelo `symbol_id` único que a semântica atribui a
cada declaração; o gerador transforma cada id em um nome LLVM único dentro da
função (`%x`, `%x.2`, ...).

Um programa inválido **nunca** gera arquivo: `main.c` remove o `.ll`/`.mlir`
antigos antes de compilar e só escreve depois que a geração termina sem erro.

## 2. Mapeamento de tipos (largura fixa)

| Macaronica | LLVM IR | Observação |
| --- | --- | --- |
| `inteira` | `i32` | sempre 32 bits, inclusive no AVR de 8 bits |
| `flut` | `float` | IEEE-754 binário de 32 bits |
| `duplocarpado` | `double` | IEEE-754 binário de 64 bits (também no Uno, ver D45) |
| `bool` | `i1` | atravessa a biblioteca como `i32` (0/1), para a ABI ser igual em todo alvo |
| `palavra` | `ptr` | ponteiro para texto terminado em zero; `null` vale `""`. O tamanho vem do alvo: 8 bytes no PC (x86-64), 4 na Pico (ARM), 2 no Uno (AVR) |
| `tipo vet{N}` | `[N x T]` | acesso por `getelementptr inbounds [N x T], ptr %v, i32 0, i32 %i` |
| `registro R` | `%struct.R = type { ... }` | campo por `getelementptr inbounds %struct.R, ptr %p, i32 0, i32 k` |

Nenhum tipo depende do "int do C": o IR é o mesmo para x86-64, ARM (Pico 2 W)
e AVR (Uno).

## 3. Construções

| Construção | Tradução |
| --- | --- |
| Variáveis locais e parâmetros | `alloca` no bloco `entry` + `load`/`store` (forma em memória). Parâmetros: `%a` (argumento) e `%a.addr` (alloca) |
| Inicialização | toda variável recebe zero ao executar a declaração (D42); globais têm inicializador `zeroinitializer`/`0` |
| Globais | `@nome = internal global T zero` |
| Aritmética | `add/sub/mul` (sem `nsw`), `sdiv`, `fadd/fsub/fmul/fdiv` |
| Conversões | `sitofp` (inteira → real), `fpext` (flut → duplocarpado); constantes já convertidas no próprio compilador |
| Comparações | `icmp slt/sgt/eq/ne`, `fcmp olt/ogt/oeq/une`; texto por `zl_texto_igual` |
| `E`, `OU` | curto-circuito com `br` e `phi i1` (D42) |
| `XOR`, `NÃO` | `xor i1 a, b`; `xor i1 x, true` |
| `cond` / `casocontrario cond` / `casocontrario` | blocos `entao_N_k`, `senao_cond_N_k`, `senao_N`, `cond_fim_N` |
| `durante` | `durante_teste_N` (teste no cabeçalho), `durante_corpo_N`, `durante_fim_N` |
| `repete` | inicialização; `repete_teste_N`; `repete_corpo_N`; `repete_passo_N` (incremento); `repete_fim_N` |
| Funções | `define internal T @nome(...) nounwind`; `principal` vira `define i32 @zl_principal() nounwind` |
| `respost` em função `vazio` (Seção 4.2) | `vazio` vira `define internal void`. Não há `respost` em `vazio`: `respost expr;` é `SEM_INVALID_RETURN` e `respost;` sem valor é erro sintático. A função termina no fim do bloco, com `ret void` |
| Chamada como comando (D33) | `call` cujo valor é descartado |
| `^` | `inteira ^ inteira`: `zl_potencia_inteira`; real ^ inteira: `llvm.powi`; real ^ real: `llvm.pow.f64` (com `flut`, `fpext` antes e `fptrunc` depois, D45) |
| `raiz` | `llvm.sqrt.f32` / `llvm.sqrt.f64` |
| Textos | `@.str.N = private unnamed_addr constant [L x i8] c"...\00"` |
| `le` / `imprime` | funções `zl_le_*` / `zl_imprime_*` + `zl_imprime_nl` |
| DSL vetorial | chamada a um núcleo `@zl_nucleo_N` gerado em MLIR (PjBL 4) |

Rótulos usam `_` (`entao_3`), caractere proibido em identificadores (D03),
então nunca colidem com nomes de variáveis. Temporários usam a numeração
implícita do LLVM (`%0`, `%1`, ...), emitida na ordem do texto. Cada comando
tem um comentário `; linha N`.

## 4. Regras de portabilidade (Seção 4.5 do enunciado)

| Regra | Como o compilador cumpre |
| --- | --- |
| 1. Sem alvo no IR | nenhum `target triple`/`datalayout`; o alvo vem de `clang` (PC), `llc -mtriple=thumbv8m.main-none-eabi` (Pico 2 W) ou `llc -mtriple=avr` (Uno) |
| 2. Largura fixa | tabela da Seção 2 |
| 3. Sem variádicas | o IR só chama funções `zl_*` de assinatura fixa e intrínsecos `llvm.*` |
| 4. `getelementptr` | todo endereço de elemento/campo usa GEP com índices `i32` |
| 5. `nounwind` | todas as funções definidas e declaradas; núcleos MLIR via `llvm.passthrough = ["nounwind"]` |
| 6. Ponto de entrada | `@zl_principal`; `main()` está em `zl_runtime_posix.c`, `zl_runtime_pico.c` e `zl_runtime_uno.c` |

Evidência: o mesmo `.ll` roda no PC (clang, lli) e no Arduino Uno emulado
(`llc -mtriple=avr -mcpu=atmega328p`) com saída idêntica (`make test`), e é
compilado sem mudança para a Raspberry Pi Pico 2 W, um ARM de 32 bits
(`llc -mtriple=thumbv8m.main-none-eabi -mcpu=cortex-m33`, D48). Saída na
placa real (`make test-pico`, 10/11/2026): 30 execuções com saída
idêntica à do PC (5 programas da placa em C0 e em C2, 1 só da Pico e os
19 de `tests/execucao`).

Funções do programa têm ligação `internal`; uma função com nome de função da
libc (`sqrt`, `abs`, `exit`...) ganha o sufixo `.mac` no IR, para o otimizador
nunca aplicar a ela a semântica da função da biblioteca.

## 5. Biblioteca de execução

`runtime/zl_runtime.h` é a única ponte entre o código gerado e a plataforma:

- `zl_runtime_comum.c`: formatação de números, leitura de valores, potência
  inteira, igualdade de textos e montagem da mensagem de erro, **iguais em
  todos os alvos** (sem `printf`);
- `zl_runtime_posix.c`: PC (Linux, macOS e Windows), saída com `putchar`;
- `zl_runtime_pico.c`: Raspberry Pi Pico 2 W, a placa principal (PjBL 4,
  D48 e D49, ver [`placa.md`](./placa.md)), sobre o Pico SDK;
- `zl_runtime_uno.c`: Arduino Uno, alvo secundário (PjBL 4).

Cada plataforma implementa os ganchos de `runtime/zl_plataforma.h` (escrever
e ler caracteres, guardar um texto lido, terminar com erro e, desde a D48,
`zl_pino_valido`, que saiu da parte comum porque cada placa tem os seus
pinos).

Os três arquivos de plataforma fazem `#include "zl_runtime_comum.c"`. Assim
cada alvo tem **um único arquivo** de biblioteca, e o comando do enunciado
funciona como está. Comandos por alvo (`scripts/construir.sh` faz o mesmo,
depois do pipeline MLIR quando o programa usa a DSL):

```sh
# PC
clang saida.ll runtime/zl_runtime_posix.c -o prog        # Linux: acrescentar -lm
lli --extra-module=build/zl_runtime_posix.bc saida.ll     # make runtime gera o .bc

# Raspberry Pi Pico 2 W (D48): objeto ARM + Pico SDK (CMake) -> .uf2
mkdir -p build/pico
llc -mtriple=thumbv8m.main-none-eabi -mcpu=cortex-m33 -float-abi=soft -O2 \
    -function-sections -data-sections -filetype=obj saida.ll -o build/pico/programa.o
cmake -S runtime/pico -B build/pico -G Ninja -DPICO_SDK_PATH=<pasta do pico-sdk> \
      -DZL_PROGRAMA_OBJ=build/pico/programa.o            # so na primeira vez
cmake --build build/pico      # + zl_runtime_pico.c -> programa.elf e programa.uf2

# Arduino Uno (D45)
llc -mtriple=avr -mcpu=atmega328p -O2 -filetype=obj saida.ll -o programa.o
avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -mdouble=64 -Os programa.o \
    runtime/zl_runtime_uno.c -lm -Wl,--gc-sections -o programa.elf
avr-objcopy -O ihex -R .eeprom programa.elf programa.hex
```

Na Pico, o `llc` gera só o objeto do programa; a biblioteca e o SDK são
compilados pelo `arm-none-eabi-gcc` com `-mfloat-abi=softfp`. Por isso o
`llc` usa `-float-abi=soft` com a FPU ligada pelo `-mcpu`: `float` e
`double` passam nos mesmos registradores dos dois lados (D48).

| | PC | Pico 2 W | Uno |
| --- | --- | --- | --- |
| Compilador da biblioteca | clang | `arm-none-eabi-gcc` (Pico SDK) | `avr-gcc` |
| `imprime` / `le` | stdout / stdin | USB da placa (CDC); `\n` sai como `\r\n` (D49) | USART0 a 9600 baud |
| Início | imediato | espera um terminal abrir a porta; sem PC, roda após 3 s (D49) | ao ligar ou ao abrir o monitor (reinício) |
| Fim normal | código de saída = `respost` (§6.3) | LED aceso fixo | LED aceso fixo |
| Erro de execução | mensagem + código de saída 3 | mensagem + LED (pino 25) piscando rápido | mensagem + LED (pino 13) piscando rápido |
| `le` de `palavra` | `malloc` | `malloc` | 4 buffers rotativos de 32 bytes |
| Pinos | simulados, numeração da Pico | GP0–GP22, GP26–GP28, 25 = LED | 2–19 |

**Nomes.** O enunciado cita `zl_imprime_int` e `zl_le_int` só como exemplos.
Aqui os nomes seguem os tipos da linguagem: `zl_imprime_inteira`,
`zl_imprime_flut`, `zl_imprime_duplo`, `zl_imprime_bool`, `zl_imprime_texto`,
`zl_imprime_nl` e `zl_le_inteira`, `zl_le_flut`, `zl_le_duplo`, `zl_le_bool`,
`zl_le_texto`, todos declarados em `runtime/zl_runtime.h`. Nenhum código
gerado chama `printf` ou `scanf`.

Códigos de erro de execução (`zl_erro_execucao(codigo, linha)`):

| Código | Situação |
| ---: | --- |
| 2 | índice de vetor fora dos limites |
| 3 | divisão por zero (`/` ou `//`) |
| 4 | `raiz` de número negativo |
| 5 | expoente negativo em `inteira ^ inteira` |
| 6 | entrada inválida ou ausente em `le` |
| 7 | pino ou modo inválido |

A mensagem é `Erro de execucao (linha L): <motivo>`. No PC o processo termina
com código 3; na Pico 2 W o LED da placa (pino 25) pisca rápido; no Uno, o
LED do pino 13.

## 6. Lacunas da Seção 4.3 e decisões

### 6.1 Promoção e conversão; `/` e `//` (D16, D18, D19, D38)

- A promoção implícita só alarga: `inteira → flut → duplocarpado` (D16). Não
  existe conversão implícita para um tipo mais estreito, então `fptrunc` e
  `fptosi` nunca aparecem em expressões.
- Cada operação é feita no tipo do seu resultado: os operandos são
  convertidos explicitamente (`sitofp`, `fpext`) antes da instrução.
- **`/` é sempre divisão real.** `inteira / inteira` é `flut`
  (`7 / 2 = 3.5`). **D38** revisa a D18: se algum operando for
  `duplocarpado`, o resultado é `duplocarpado` (antes era sempre `flut`, o
  que exigiria `fptrunc` e perderia precisão em silêncio).
- **`//` é divisão inteira** e só aceita `inteira // inteira` (D19). Com
  operando real, é erro semântico. Arredonda para zero, como `sdiv`
  (`-7 // 2 = -3`).
- `raiz` produz `flut`, ou `duplocarpado` para argumento `duplocarpado` (D38
  revisa a D20 pelo mesmo motivo).
- **Literal decimal** tem tipo `flut` (D15). Quando é promovido a
  `duplocarpado`, o compilador converte a partir do **texto** do literal, e
  não do `float` estendido: `d receba 0.1` guarda o double mais próximo de
  0,1 (`0.10000000`), não `0.100000001490116` (D38). Ver
  `tests/execucao/08_tipos_mistos.mac`.

### 6.2 Curto-circuito em `E` e `OU`; `XOR` (D42)

`E` e `OU` usam curto-circuito: o operando da direita só é avaliado se for
necessário, e o valor vem de um `phi i1` no bloco de junção. Isso permite
escrever `(i < 3) E (v{i} > 0)` sem erro de índice
(`tests/execucao/13_curto_circuito.mac`). `XOR` não tem curto-circuito
possível (precisa dos dois lados) e vira `xor i1`.

### 6.3 `respost` dentro de `principal` (D42)

`@zl_principal` sempre devolve `i32`, usado como **código de saída** do
processo no PC:

| tipo de `principal` | código de saída |
| --- | --- |
| `inteira` | o valor de `respost` |
| `bool` | 1 para `VER`, 0 para `FAL` |
| `flut` / `duplocarpado` | valor truncado (`fptosi`) |
| `palavra` / `vazio`, ou fim sem `respost` | 0 (como o `return 0` implícito do C99, D34) |

Nas placas (Pico 2 W e Uno) não há processo: o valor é ignorado e o LED fica
aceso.

Parâmetros de `principal` (permitidos pela gramática desde o PjBL 1) são
**lidos da entrada** no início do programa, na ordem, como se fossem `le`
(`tests/execucao/17_principal_parametros.mac`).

### 6.4 Divisão por zero, estouro e índice fora dos limites (D40, D43, D47)

| Situação | Política |
| --- | --- |
| Divisor **constante** zero | erro semântico `SEM_DIVISION_BY_ZERO` |
| Divisor variável zero (`/` e `//`) | verificação em tempo de execução → erro 3 |
| `-2147483648 // -1` | resultado definido: `-2147483648` (o divisor vira 1 por `select`) |
| Estouro em `+ - *` e `^` inteira | aritmética em complemento de dois (dá a volta), sem `nsw` |
| Índice **constante** fora de `[0, N)` | erro semântico `SEM_INDEX_OUT_OF_BOUNDS` |
| Índice com faixa **provada** dentro de `[0, N)` (D47) | nenhuma verificação |
| Qualquer outro índice variável | verificação em tempo de execução → erro 2 |
| `raiz` de negativo | verificação em tempo de execução → erro 4 (omitida para literal ou constante ≥ 0) |

As verificações só são inseridas **quando não podem ser provadas
estaticamente**. A D47 descreve a análise de faixa: um `repete` com limites
constantes, cuja variável de controle é local e não muda no corpo, dá a
faixa da variável. A faixa de `i`, `9 - i`, `i * 4 + j` etc. é então
calculada por aritmética de intervalos.

Evidência no IR de `tests/execucao/05_vetor.mac`, gerado por `make test` em
`build/testes/execucao-C0/05_vetor.ll`. Na linha 25,
`invertido{9 - i} receba dados{i};` está dentro de
`repete (i receba 0, i < 10, i receba i + 1)`. Os dois índices têm faixa
`[0, 9]` e o acesso sai sem teste:

```llvm
  ; linha 25
  %18 = load i32, ptr %i
  %19 = sub i32 9, %18
  ; linha 25: indice em [0, 9] dentro de [0, 10), sem verificacao (D47)
  %20 = getelementptr inbounds [10 x i32], ptr %invertido, i32 0, i32 %19
  %21 = load i32, ptr %i
  ; linha 25: indice em [0, 9] dentro de [0, 10), sem verificacao (D47)
  %22 = getelementptr inbounds [10 x i32], ptr @dados, i32 0, i32 %21
```

Em `tests/execucao/19_faixa_indices.mac`, linha 33, o corpo do laço faz
`i receba 8`. A faixa deixa de valer e a verificação continua:

```llvm
  ; linha 33
  %57 = load i32, ptr %i
  %58 = icmp ult i32 %57, 10
  br i1 %58, label %indice_ok_8, label %indice_erro_8
indice_erro_8:
  call void @zl_erro_execucao(i32 2, i32 33)
  unreachable
indice_ok_8:
  %59 = getelementptr inbounds [10 x i32], ptr %v, i32 0, i32 %57
```

O mesmo programa termina com `repete (i receba 7, i < 11, ...)` sobre um
vetor de 10. A faixa `[7, 10]` não cabe, a verificação fica e o erro de
execução da linha 36 acontece como esperado.

`m receba dados{0};` (linha 6 de `05_vetor.mac`, índice constante) gera só o
acesso:

```llvm
  ; linha 6
  %0 = getelementptr inbounds [10 x i32], ptr @dados, i32 0, i32 0
  %1 = load i32, ptr %0
```

Da mesma forma, `a // 2` não gera teste de divisor zero. `icmp ult` cobre
índices negativos com uma única comparação, porque um negativo vira um número
sem sinal enorme.

**Efeito medido, sem exagero.** No `-O2`, o próprio LLVM já elimina essas
verificações nos laços simples: com e sem a D47 os laços de teste foram
vetorizados (`-Rpass=loop-vectorize`: largura 4, intercalação 2). O ganho da
D47 aparece onde não há otimização: no `lli`, que executa o IR sem os passes
de `opt`, e no `-O0`. Ali, cada acesso deixa de pagar uma comparação, um
desvio e um bloco de erro. Há também um ganho de clareza: o IR mostra quais
acessos foram provados seguros.

### 6.5 Formato de impressão (D42)

| Tipo | Formato | Exemplo |
| --- | --- | --- |
| `inteira` | decimal | `-27` |
| `flut` | 4 casas, metade para longe do zero | `2.5000` |
| `duplocarpado` | 8 casas | `0.10000000` |
| `bool` | `VER` / `FAL` | `VER` |
| `palavra` | o texto | `Bom dia` |

Cada `imprime` termina a linha. A formatação é feita pela biblioteca sem
`printf`, com aritmética de `double` IEEE-754, então PC e placa imprimem os
mesmos dígitos. Valores fora da faixa da forma fixa usam notação científica
(`1.2345e+20`); `nan`/`inf` só aparecem por estouro.

### 6.6 Largura dos tipos nas placas: AVR de 8 bits (D45) e ARM de 32 bits (D48)

- `inteira` é sempre `i32`, também no AVR. O `llc` divide as operações em
  registradores de 8 bits e chama as rotinas da libgcc (`__mulsi3`,
  `__divmodsi4`). É mais lento, mas o resultado é o mesmo do PC, que é o que
  a linguagem promete.
- `duplocarpado` é `double` de 64 bits **também no Uno**: o projeto liga o
  programa da placa com `avr-gcc -mdouble=64` (avr-gcc 16 + avr-libc 2.3), em
  que a libgcc traz as rotinas IEEE de 64 bits (`__adddf3`, `__divdf3`...).
  Em vez de rejeitar o tipo ou rebaixá-lo para `float`, o mesmo IR roda com
  o mesmo resultado (`tests/placa/02_recursao.mac` e
  `tests/placa/04_dsl_e_duplo.mac`). O custo é cerca de 1,5 KB de flash.
- Na **Pico 2 W** (Cortex-M33, D48), `inteira` tem o tamanho do registrador e
  `flut` usa a FPU de precisão simples. `duplocarpado` já é `double` de 64
  bits pela ABI do ARM; como a FPU não tem precisão dupla, as operações viram
  chamadas `__aeabi_d*`, atendidas pelo SDK com o coprocessador de precisão
  dupla (DCP) da RP2350. O IR é o mesmo dos outros alvos.

## 7. Lacunas adicionais identificadas pelo grupo

| # | Lacuna | Decisão |
| --- | --- | --- |
| D39 | O PjBL 3 exige registros, mas a linguagem não tinha variável de tipo registro nem acesso a campo | `registro R p;` declara (global ou local, inclusive `registro R vet{3} t;`); `p.campo` e `v{i}.campo` acessam; `p receba q` copia o registro inteiro. Registros não são parâmetros nem retorno |
| D40 | Erros que dá para provar na compilação | índice constante fora dos limites e divisor constante zero são erros semânticos |
| D42 | Valor inicial de variáveis | zero, sempre que a declaração é executada (também dentro de laços) |
| D42 | Igualdade de `palavra` | compara o conteúdo, não o endereço |
| D38 | Literal inteiro fora de `i32` | erro semântico `SEM_LITERAL_OUT_OF_RANGE` (`-2147483648` deve ser escrito `0 - 2147483647 - 1`) |
| D42 | Leitura de `palavra` | `le(p)` lê a linha inteira |
| D42 | Ordem de avaliação | da esquerda para a direita; em atribuição, o índice do destino é avaliado antes do valor |
| D46 | `tipo vet{N} a, b;` | segue a especificação oficial ("`tipo vet{tamanho} nome, ...`"): todos os nomes são vetores de N |
| D47 | Verificação de índice em todo acesso de laço | análise de faixa em `repete` com limites constantes; verificação omitida quando o índice cabe em `[0, N)` (§6.4) |
| D48 | A placa do grupo é a Raspberry Pi Pico 2 W (ARM Cortex-M33 de 32 bits), não o Uno | mesmo IR com `llc -mtriple=thumbv8m.main-none-eabi -mcpu=cortex-m33 -float-abi=soft` (softfp, a ABI do Pico SDK) e ligação pelo Pico SDK 2.3.1 (CMake); `--alvo=pico` verifica globais + textos + quadro ≤ 524 288 − 65 536 bytes, com `palavra` de 4 bytes; pinos GP0–GP22, GP26–GP28 e 25 = LED, simulados no PC; o Uno fica como alvo secundário (QEMU) |
| D49 | Sem conversor USB-serial, o que o programa imprime antes de o terminal abrir a porta se perderia | o programa espera o terminal (DTR) com o LED piscando devagar; sem PC, roda após 3 s; fechar o terminal reinicia a placa; 1200 baud reinicia em BOOTSEL para gravar |
| — | Nome de função igual ao de função da libc | sufixo `.mac` no IR (Seção 4) |
| — | Vetores locais grandes | ficam na pilha (`alloca`); vetores grandes devem ser globais (no PC a pilha padrão do Windows é 1 MB) |

## 8. SSA: análise do `mem2reg` e do `-O2`

Programa ([`ir/somapares.mac`](./ir/somapares.mac)), com laço e condicional:

```text
outrafuncao inteira somapares(inteira n) @ [
    inteira i, soma;

    soma receba 0;
    repete (i receba 0, i < n, i receba i + 1) edai [
        cond ((i // 2) * 2 == i) edai [
            soma receba soma + i;
        ]
    ]
    respost soma;
]
```

### 8.1 IR gerado (forma em memória)

```llvm
define internal i32 @somapares(i32 %n) nounwind {
entry:
  %n.addr = alloca i32
  %i = alloca i32
  %soma = alloca i32
  store i32 %n, ptr %n.addr
  ...
  store i32 0, ptr %i
  br label %repete_teste_0
repete_teste_0:
  %0 = load i32, ptr %i
  %1 = load i32, ptr %n.addr
  %2 = icmp slt i32 %0, %1
  br i1 %2, label %repete_corpo_0, label %repete_fim_0
repete_corpo_0:
  %3 = load i32, ptr %i
  %4 = sdiv i32 %3, 2
  %5 = mul i32 %4, 2
  %6 = load i32, ptr %i
  %7 = icmp eq i32 %5, %6
  br i1 %7, label %entao_1_0, label %cond_fim_1
entao_1_0:
  %8 = load i32, ptr %soma
  %9 = load i32, ptr %i
  %10 = add i32 %8, %9
  store i32 %10, ptr %soma
  br label %cond_fim_1
cond_fim_1:
  br label %repete_passo_0
repete_passo_0:
  %11 = load i32, ptr %i
  %12 = add i32 %11, 1
  store i32 %12, ptr %i
  br label %repete_teste_0
repete_fim_0:
  %13 = load i32, ptr %soma
  ret i32 %13
}
```

Arquivo completo: [`ir/somapares.ll`](./ir/somapares.ll).

### 8.2 Depois de `opt -S -passes=mem2reg`

```llvm
define internal i32 @somapares(i32 %n) #0 {
entry:
  br label %repete_teste_0

repete_teste_0:                         ; preds = %repete_passo_0, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %5, %repete_passo_0 ]
  %soma.0 = phi i32 [ 0, %entry ], [ %soma.1, %repete_passo_0 ]
  %0 = icmp slt i32 %i.0, %n
  br i1 %0, label %repete_corpo_0, label %repete_fim_0

repete_corpo_0:                         ; preds = %repete_teste_0
  %1 = sdiv i32 %i.0, 2
  %2 = mul i32 %1, 2
  %3 = icmp eq i32 %2, %i.0
  br i1 %3, label %entao_1_0, label %cond_fim_1

entao_1_0:                              ; preds = %repete_corpo_0
  %4 = add i32 %soma.0, %i.0
  br label %cond_fim_1

cond_fim_1:                             ; preds = %entao_1_0, %repete_corpo_0
  %soma.1 = phi i32 [ %4, %entao_1_0 ], [ %soma.0, %repete_corpo_0 ]
  br label %repete_passo_0

repete_passo_0:                         ; preds = %cond_fim_1
  %5 = add i32 %i.0, 1
  br label %repete_teste_0

repete_fim_0:                           ; preds = %repete_teste_0
  ret i32 %soma.0
}
```

O `mem2reg` promove a registradores SSA os três `alloca` cujo endereço só é
usado por `load`/`store`. Em SSA cada valor tem uma única definição, então,
onde o fluxo de controle se junta e a variável pode ter valores diferentes
conforme o caminho, aparece uma instrução `phi` que escolhe o valor pelo bloco
de onde se veio. Os pontos são calculados pela fronteira de dominância dos
blocos que fazem `store`:

- **`%i.0` no `repete_teste_0`**: o cabeçalho do laço tem dois predecessores,
  `entry` (antes da primeira iteração, `i = 0`) e `repete_passo_0` (a volta do
  laço, `i = i + 1` em `%5`).
- **`%soma.0` no `repete_teste_0`**: pelo mesmo motivo; vale 0 na entrada e
  `%soma.1` na volta.
- **`%soma.1` no `cond_fim_1`**: a junção do `cond` tem dois predecessores.
  Vindo do `entao_1_0`, `soma` foi atualizada (`%4`); vindo direto do
  `repete_corpo_0` (condição falsa), `soma` é a mesma do início da iteração
  (`%soma.0`).

O parâmetro `n` não precisa de `phi`: só tem um `store` (o do argumento), que
domina todos os usos, e seus `load` viram o próprio `%n`. Os `store i32 0`
repetidos (zero da declaração, D42, seguido de `soma receba 0`) desaparecem
sem deixar rastro, porque nenhum `load` lê o primeiro valor. O `mem2reg` só
reescreve memória: o bloco vazio `cond_fim_1 → repete_passo_0` continua lá, e
quem o elimina é o `simplifycfg` do `-O2`.

### 8.3 Depois de `-O2`

Com `n` constante ([`ir/somapares.O2.ll`](./ir/somapares.O2.ll)), o
`-O2` embute `somapares` em `zl_principal`, desenrola o laço e calcula tudo em
tempo de compilação:

```llvm
define noundef i32 @zl_principal() local_unnamed_addr #0 {
entry:
  tail call void @zl_imprime_inteira(i32 20)
  tail call void @zl_imprime_nl()
  ret i32 0
}
```

Com `n` lido da entrada ([`ir/somapares_entrada.O2.ll`](./ir/somapares_entrada.O2.ll),
`opt -O2 -mtriple=x86_64-pc-linux-gnu -mcpu=alderlake`), o laço não pode ser
eliminado e aparecem as transformações clássicas:

1. **Inlining**: `somapares` é embutida em `zl_principal` (é `internal`, tem um
   único chamador).
2. **Redução de força**: `(i // 2) * 2 == i` vira `(i & 1) == 0`. A `sdiv`, que
   é cara, some.
3. **If-conversion**: o `cond` vira `select` (soma `i` ou 0), sem desvio no
   corpo.
4. **Vetorização + intercalação**: o vetorizador de laços processa 32
   iterações por volta, em quatro acumuladores `<8 x i32>` (AVX2):

```llvm
vector.body:
  %index = phi i32 [ 0, %vector.ph ], [ %index.next, %vector.body ]
  %vec.phi = phi <8 x i32> [ zeroinitializer, %vector.ph ], [ %14, %vector.body ]
  ...
  %vec.ind = phi <8 x i32> [ <i32 0, i32 1, ..., i32 7>, %vector.ph ], ...
  %2 = and <8 x i32> %vec.ind, splat (i32 1)
  %6 = icmp eq <8 x i32> %2, zeroinitializer
  %10 = select <8 x i1> %6, <8 x i32> %vec.ind, <8 x i32> zeroinitializer
  %14 = add <8 x i32> %10, %vec.phi
```

Os `phi` que o `mem2reg` criou continuam no centro de tudo: `%soma.0` vira o
acumulador vetorial `%vec.phi` e `%i.0` vira o vetor de induções `%vec.ind`.
Como a soma de inteiros é associativa, o LLVM pode reordenar as somas sem
alterar o resultado, ao contrário do que acontece com `flut`
([`relatorio_otimizacao.md`](./relatorio_otimizacao.md), Seção 4).

Para reproduzir:

```sh
./macaronica docs/ir/somapares.mac -o docs/ir/somapares.ll
opt -S -passes=mem2reg docs/ir/somapares.ll -o docs/ir/somapares.mem2reg.ll
opt -S -O2 -mtriple=x86_64-pc-linux-gnu docs/ir/somapares.ll -o docs/ir/somapares.O2.ll
./macaronica docs/ir/somapares_entrada.mac -o docs/ir/somapares_entrada.ll
opt -S -O2 -mtriple=x86_64-pc-linux-gnu -mcpu=alderlake \
    docs/ir/somapares_entrada.ll -o docs/ir/somapares_entrada.O2.ll
```
