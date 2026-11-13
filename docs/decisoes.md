# Decisões de projeto da Macaronica

Este arquivo registra as decisões adotadas para transformar a especificação
incompleta em uma linguagem implementável. As decisões não alteram a
transcrição oficial; definem o comportamento do projeto quando a fonte não é
suficiente.

## Documentos relacionados

- **Gramática usada pelo projeto:** [`README_gramatica.md`](./README_gramatica.md)
  apresenta, em EBNF, a gramática efetiva reconhecida pelo parser.
- **Especificação léxica:** [`tokens.md`](./tokens.md) lista os tokens, as
  regras léxicas e as prioridades de reconhecimento.
- **Origem das decisões:** [`README_tabela.md`](./README_tabela.md) separa o
  que a fonte define do que é ambíguo ou ausente.
- **Fonte original:** [`especificacao_macaronica.md`](./especificacao_macaronica.md)
  transcreve a especificação recebida, sem resolver lacunas.

## D01 — Vocabulário estrutural

Além das palavras oficiais, serão tokens reservados estruturais:

    registro outrafuncao vet edai raiz

raiz é reservada para não ser reconhecida como identificador. func permanece
lexicalmente reservado, mas não introduz função; somente outrafuncao inicia
função não-principal.

## D02 — Tokens necessários às estruturas

@ e , são aceitos porque aparecem nas estruturas oficiais. O ponto existe
somente dentro de números decimais e aspas duplas delimitam textos. // é
reconhecido antes de / e representa divisão inteira, não comentário.

## D03 — Identificadores, números e textos

- Identificador: letra ASCII seguida de letras ou dígitos.
- Inteiro: uma ou mais casas decimais.
- Decimal: casas decimais, ponto e uma ou mais casas decimais.
- Texto: entre aspas duplas, na mesma linha.
- _ não pertence a identificadores.
- Reservadas são case-sensitive.
- O operador lógico é NÃO, com acento.

## D04 — Espaços e comentários

Espaços, tabulações e quebras de linha são ignorados fora de textos. Não há
comentários nesta versão, pois a especificação não define sua sintaxe.

## D05 — Vetores

Declaração:

    tipo vet{tamanho} nome;

Acesso:

    nome{indice}

tamanho é inteiro positivo. indice é expressão; sua verificação semântica é
definida nas decisões posteriores.

## D06 — Chamadas de função

A sintaxe de chamada é:

    nome(argumento1, argumento2)

A lista pode ser vazia e os argumentos são expressões. Quantidade e tipos são
verificados semanticamente.

## D07 — Funções vazio

Função vazio não possui retorno sem valor. O parser reconhece
respost EXPRESSAO; em qualquer função; a semântica rejeita esse comando em
função vazio e verifica compatibilidade nas demais. respost ; não pertence à
gramática.

vazio não é aceito em variável, parâmetro ou campo, pois esses contextos usam
TIPO_VALOR.

## D08 — Expressões e precedência

Atribuições, argumentos e índices usam EXPRESSAO. Comparações e lógica são
expressões e produzem bool. A precedência, da maior para a menor, é:

1. parênteses, chamada, vetor e raiz;
2. unários + e -;
3. potência, associativa à direita;
4. *, / e //;
5. + e -;
6. <, >, == e !;
7. NÃO;
8. E;
9. XOR;
10. OU.

! é diferente; NÃO é a negação lógica.

## D09 — Condições

Condição é EXPRESSAO, mas a semântica exige resultado bool em cond, durante e
na expressão central de repete. Não há truthiness numérico.

## D10 — repete

O cabeçalho é:

    repete ( i receba 0, i < 10, i receba i + 1 ) edai [ ... ]

Os três elementos são atribuições/expressão/atribuição e são separados por
vírgula, sem ponto e vírgula interno.

## D11 — Registros (REVISADA pela D39)

> **Revisada pela D39 (PjBL 3):** agora há variáveis de tipo registro, acesso
> a campo (`p.x`, `v{i}.x`) e cópia de registro inteiro. O texto abaixo é a
> decisão original do PjBL 1.

Registro contém campos simples tipo nome;. Não há acesso a campos,
inicialização de registros ou vetor de campo nesta versão.

## D12 — Escopo da gramática

A gramática verifica forma. Declaração antes do uso, tipos, regras de tamanho
e índice de vetor, assinaturas e validade de retorno pertencem à semântica.

## D13 — Chamada como comando (SUBSTITUÍDA pela D33)

Versão original (PjBL 1): chamada isolada não era comando; o parser não
aceitava quadrado(5);.

A avaliação do PjBL 1 mostrou que D13 combinada com D16 ("vazio nunca é
valor") tornava **impossível chamar uma função vazio**: ela não podia ser
comando (D13) nem valor (D16). A D33 substitui esta decisão.

## D14 — Declarações globais

Declarações de variável são permitidas como itens do programa, antes, entre ou
depois de registros e funções.

## Decisões semânticas (D15 a D32)

O enunciado do trabalho prático define os requisitos gerais e as lacunas da
Seção 4.3, mas não formaliza as regras numéricas, de tipos e de escopo
necessárias à análise semântica. As decisões abaixo são, portanto, decisões
do projeto — não transcrição oficial — e completam a linguagem efetiva no
ponto em que a fonte é omissa.

## D15 — Tipos numéricos e literal decimal

inteira, flut e duplocarpado são numéricos. A ordem de promoção é:

    inteira < flut < duplocarpado

AST_LITERAL_INTEGER tem tipo inteira; AST_LITERAL_DECIMAL tem tipo flut;
AST_LITERAL_BOOLEAN tem tipo bool; AST_LITERAL_TEXT tem tipo palavra.

A promoção só ocorre de tipo mais estreito para mais largo. Não há narrowing
implícito.

## D16 — Compatibilidade

A mesma regra vale para atribuição, argumentos e retornos:

- tipos iguais são compatíveis;
- inteira promove para flut ou duplocarpado;
- flut promove para duplocarpado;
- tipo mais largo não é atribuído a tipo mais estreito;
- bool só é compatível com bool;
- palavra só é compatível com palavra;
- vazio nunca é valor;
- SEM_TYPE_ERROR é sentinela interno.

## D17 — Aritmética

Os operadores +, -, * e ^ exigem operandos numéricos e produzem o tipo
numérico mais largo.
Não existe concatenação de palavra com + e bool não é numérico.

## D18 — Divisão normal (REVISADA pela D38)

> **Revisada pela D38 (PjBL 3):** `/` produz `duplocarpado` quando algum
> operando é `duplocarpado` (antes era sempre `flut`). O texto abaixo é a
> decisão original do PjBL 1.

/ exige operandos numéricos e produz flut, inclusive
duplocarpado / duplocarpado. Esse é resultado específico do operador, não
uma aplicação geral da promoção.

## D19 — Divisão inteira

// aceita somente inteira com inteira e produz inteira. Outros pares são erro.

## D20 — Raiz (REVISADA pela D38)

> **Revisada pela D38 (PjBL 3):** `raiz` de `duplocarpado` produz
> `duplocarpado`. O texto abaixo é a decisão original do PjBL 1.

raiz(expressao) aceita expressão numérica e produz flut. bool, palavra e
vazio são inválidos. A regra vale também para argumento duplocarpado.

## D21 — Comparações de ordem

< e > aceitam operandos numéricos, inclusive tipos numéricos diferentes após
promoção conceitual para o mais largo. O resultado é bool. palavra e bool não
participam de comparação ordenada.

## D22 — Igualdade e diferença

== e ! exigem operandos do mesmo tipo e produzem bool. Igualdade não aplica
promoção numérica. Logo inteira == flut é inválido nesta versão.

## D23 — Lógica

E, OU, XOR e NÃO exigem bool e produzem bool. Não existe truthiness.

## D24 — Escopos aninhados

A hierarquia é:

    GLOBAL -> FUNCTION -> BLOCK -> BLOCK -> ...

O corpo diretamente associado à função usa o escopo de função e não cria um
escopo adicional. AstBlock aninhado cria escopo filho. Blocos de cond,
durante, repete, casocontrario e blocos explícitos criam escopos próprios.

## D25 — Shadowing e redeclaração

Redeclaração significa mesmo nome no mesmo escopo e é erro. Shadowing
significa mesmo nome em escopo filho e é permitido. Lookup percorre o escopo
atual e os pais; o primeiro símbolo vence.

Variáveis, funções e registros compartilham um namespace único dentro de cada
escopo. Portanto, uma variável e uma função com o mesmo nome no mesmo escopo
também são duplicata. A categoria continua armazenada para que um uso
incompatível produza o diagnóstico correto.

Parâmetros entram no escopo de função. Parâmetro e local com mesmo nome são
redeclaração; parâmetro que oculta global é permitido.

## D26 — Duas passadas

Passada 1 coleta registros, declaradores globais, assinaturas de funções e
funções principais.

Passada 2 analisa corpos: abre escopo de função, insere parâmetros, visita
corpo em ordem, abre/fecha blocos e verifica nomes, tipos, chamadas, retornos,
condições e vetores.

Todos os globais e assinaturas ficam visíveis na segunda passada. Assim,
global posterior e função posterior podem ser referenciados. Locais continuam
sujeitos a declaração antes do uso. A assinatura da própria função permite
recursão direta e mútua.

## D27 — Função principal

O programa deve possuir exatamente uma função principal. A principal aceita
qualquer tipo de retorno e parâmetros já aceitos pela gramática. Ela não é
chamável: principal é token próprio, não ID, e a AST atual não representa
principal() como chamada.

## D28 — Vetores na semântica (REVISADA pelas D40 e D47)

> **Revisada pela D40 (PjBL 3):** índice constante fora de `[0, N)` é erro
> semântico (com avaliação de expressões constantes); índices variáveis são
> verificados em tempo de execução, exceto quando a análise de faixa da D47
> prova que estão nos limites. O texto abaixo é a decisão original do PjBL 1.

O tamanho deve ser inteiro positivo e sua conversão deve detectar overflow.
O índice deve ser inteira e o acesso resulta no tipo do elemento.

Vetor sem índice em contexto de valor escalar é erro. Escalar indexado é
SEM_NOT_A_VECTOR. Não há verificação estática de limites nem constant folding;
isso é limitação documentada de escopo.

## D29 — Registros e duplicidade local (REVISADA pela D39)

> **Revisada pela D39 (PjBL 3):** registro declara variável
> (`registro Ponto p;`) e o acesso por ponto faz parte da linguagem. As regras
> de duplicidade continuam valendo. O texto abaixo é a decisão original do
> PjBL 1.

Registro não é tipo de expressão e não pode declarar variável. Registro
duplicado no conjunto global é erro. Campo duplicado no mesmo registro é erro.
Acesso por ponto permanece fora da linguagem.

## D30 — Retorno sem análise de fluxo (SUBSTITUÍDA pela D34)

Versão original (PjBL 1): retornos presentes eram verificados, mas não se
exigia retorno em todos os caminhos. Para gerar código (PjBL 3) isso deixaria
indefinido o valor devolvido por uma função que "cai" no fim do corpo; a
D34 passa a exigir o retorno.

## D31 — Entrada e saída — RESOLVIDA (complementada pelas D39 e D42)

> **Complementada no PjBL 3:** `le`/`imprime` não aceitam registro (D39);
> o formato de impressão e a leitura de `palavra` (linha inteira) estão na
> D42.

A especificação original da Macaronica não define construções de entrada e
saída, embora o requisito geral de entrega exija que o conjunto de programas
válidos cubra entrada e saída.

Como decisão explícita posterior do grupo, a linguagem efetiva é estendida
com dois statements próprios:

    le(LVALUE);
    imprime(EXPRESSAO);

`le` recebe um lvalue válido, que pode ser uma variável, um parâmetro ou um
elemento de vetor. `imprime` recebe uma expressão válida que produza valor.
As construções não são funções: são comandos próprios (chamadas comuns como
comando passaram a ser permitidas pela D33).

O front-end apenas reconhece, representa e analisa esses comandos. Nenhuma
leitura ou impressão é executada, pois runtime e geração de código estão fora
do escopo do projeto.

Histórico: as grafias `leia`/`escreva` foram consideradas e não adotadas. A
extensão acima resolve a lacuna sem alterar a especificação original.

## D32 — Operador unário +

O operador unário + aceita qualquer tipo numérico e produz o mesmo tipo do
operando. bool, palavra e vazio são inválidos. A decisão é simétrica ao
unário - e não introduz conversão.

## Decisões posteriores à avaliação do PjBL 1 (D33 a D37)

As decisões abaixo corrigem os pontos apontados na avaliação do PjBL 1 e são
pré-requisito para a geração de código (PjBL 3).

## D33 — Chamada de função como comando

**Problema.** D13 + D16 impediam qualquer chamada a função vazio.

**Decisão.** Uma chamada isolada terminada por `;` é um comando:

    CHAMADA_CMD -> ID '(' ARGUMENTOS? ')' ';'

- É o **único** contexto em que uma função vazio pode ser chamada; em
  expressão, `a receba mostra();` continua sendo `SEM_VOID_VALUE` (D16).
- Funções com valor também podem ser chamadas como comando; o valor é
  descartado (como em C).
- Quantidade e tipos dos argumentos são verificados como em qualquer chamada.
- `le` e `imprime` continuam sendo comandos próprios (D31), não funções.

**Justificativa.** É a solução usual (C, Java, Pascal) e não cria
ambiguidade: após `ID`, o token `(` indica chamada e `receba`/`{` indica
atribuição (LL(1) após ler o identificador).

**Consequência.** `tests/validos/07_chamada_vazio.mac`; novo nó
`AST_STMT_CALL`.

## D34 — Retorno em todos os caminhos

**Problema.** D30 aceitava função não-vazio que podia terminar sem `respost`.

**Decisão.** Toda função não-vazio, exceto `principal`, deve terminar com
`respost` em todos os caminhos; caso contrário, `SEM_MISSING_RETURN`. Um
comando "sempre retorna" quando é:

- `respost`;
- bloco com algum comando que sempre retorna;
- `cond` com `casocontrario edai` final, em que todos os ramos sempre
  retornam;
- `durante (VER)` ou `repete (..., VER, ...)`: como a linguagem não tem
  comando de interrupção, esses laços só terminam por `respost`.

Qualquer outro laço é considerado capaz de terminar (análise conservadora,
sem avaliar condições). A `principal` pode terminar sem `respost`: o valor
devolvido é o zero do tipo (como o `return 0` implícito do C99).

**Consequência.** `tests/validos/08_retorno_caminhos.mac` e
`tests/invalidos/06_sem_retorno.mac`.

## D35 — Limites de aninhamento

**Problema.** Parser, semântica e destrutores da AST são recursivos; 100.000
operadores encadeados, 50.000 parênteses ou 100.000 blocos estouravam a
pilha (segmentation fault).

**Decisão.** Dois limites, verificados no parser, viram erro sintático
normal ("aninhamento profundo demais" / "expressão longa ou profunda demais"):

| Limite | Valor | O que conta |
| --- | ---: | --- |
| `PARSER_MAX_NESTING` | 256 | parênteses, argumentos, índices, blocos `[ ]`, unários, `NÃO` e `^` |
| `PARSER_MAX_EXPRESSION_HEIGHT` | 1024 | altura da árvore de uma expressão (inclui cadeias `a + a + ...`) |

Quando o limite é atingido, o grupo `( ... )` / `[ ... ]` inteiro é pulado sem
recursão e o modo pânico (D36) suprime a cascata nos níveis externos.

**Justificativa.** A altura da AST passa a ser limitada, então todas as
travessias recursivas (semântica, destrutores, gerador de código) são
seguras sem precisar reescrevê-las de forma iterativa. Os valores ficam muito
acima de qualquer programa real e abaixo do que cabe na pilha padrão de 1 MB
do Windows.

**Consequência.** `tests/invalidos/09_aninhamento_extremo.mac` (3.000
parênteses) e o teste unitário `extreme_nesting_is_one_error` (9 formas
patológicas com 50.000 a 100.000 níveis, cada uma com exatamente 1 erro).

## D36 — Recuperação de erros sem cascata

**Problema.** `;` ausente gerava um segundo erro falso na linha seguinte; um
`$` no meio de uma expressão gerava erro léxico e sintático; o relatório de
`01_erro_lexico` dizia "[SINTATICO] Status: OK".

**Decisão.**

1. **`;` ausente no fim da linha.** Se o token seguinte está em linha
   posterior, o erro é relatado onde o `;` deveria estar e o parser age como
   se ele tivesse sido inserido.
2. **Modo pânico.** Depois de um token inválido (descartado pelo léxico) ou
   de um limite de aninhamento, erros sintáticos são contados como
   "suprimidos" até a próxima fronteira de comando (`;`, `[` ou `]`). Erros
   genuínos em outros comandos continuam sendo relatados.
3. **`vazio x;` em comando** gera um único erro e a declaração é consumida.
4. **Relatório.** A fase sintática mostra `PARCIAL` quando houve erro léxico
   (o parser trabalhou sem os tokens inválidos), informa quantos erros em
   cascata foram suprimidos, e a fase semântica mostra `NAO EXECUTADO`.

**Consequência.** `tests/invalidos/07_ponto_virgula.mac` e os testes
unitários `missing_semicolon_recovery`, `lexical_error_without_cascade` e
`void_variable_single_error`.

## D37 — Identificadores com caracteres fora do alfabeto

**Problema.** Em `inteira função;` cada caractere acentuado virava um erro
léxico e o identificador partido gerava ainda um erro sintático.

**Decisão.** Uma palavra que mistura letras ASCII com caracteres UTF-8
multibyte é lida inteira como **um** identificador inválido: um único erro
léxico ("identificador com caractere fora do alfabeto") e o token segue para o
parser como `TOK_ID`, sem cascata. Um caractere multibyte isolado (`€`) gera um
único erro; bytes que não formam UTF-8 válido continuam sendo reportados um a
um, pois não há como delimitar o "caractere". Colunas contam caracteres.

**Consequência.** `tests/invalidos/08_identificador_acento.mac` e o teste
`accented_identifier_single_error`.

## Decisões dos PjBL 3 e 4 (D38 a D51)

Detalhes e evidências em [`decisoes_backend.md`](./decisoes_backend.md),
[`dsl.md`](./dsl.md) e [`placa.md`](./placa.md).

## D38 — Divisão, raiz e literais na geração de código (revisa D18 e D20)

- `/` continua sendo sempre divisão real (`inteira / inteira` é `flut`), mas
  o resultado passa a ser `duplocarpado` quando algum operando for
  `duplocarpado`. O mesmo vale para `raiz`. Antes, `duplocarpado / duplocarpado`
  era `flut`, o que exigiria um `fptrunc` e perderia precisão sem aviso, contra
  o espírito da D16 (nenhum estreitamento implícito).
- Literal inteiro fora de `i32` é `SEM_LITERAL_OUT_OF_RANGE`. (REVISADA pela
  D50: virou aviso, com o valor reduzido módulo 2³².)
- Literal decimal (tipo `flut`, D15) promovido a `duplocarpado` é convertido a
  partir do texto: `0.1` vira o double mais próximo de 0,1.

## D39 — Variáveis de tipo registro e acesso a campo

A linguagem tinha registros, mas não havia como usá-los (D11, D29). Agora:

    registro Ponto p, q;            // global ou local
    registro Ponto vet{10} pontos;  // vetor de registros
    p.x receba 3;  pontos{i}.y receba p.x;
    q receba p;                     // copia o registro inteiro

O token novo `TOK_PONTO` (`.` seguido de letra) separa a variável do campo;
`.5` continua inválido. O acesso a campo inexistente, ou em valor que não é
registro, é `SEM_INVALID_FIELD`. Registros não podem ser parâmetros, retorno,
operandos de expressão, `le` ou `imprime`, só alvo/origem de atribuição.

## D40 — Erros provados na compilação

Índice constante fora de `[0, N)` (`SEM_INDEX_OUT_OF_BOUNDS`) e divisor
constante zero (`SEM_DIVISION_BY_ZERO`) passam a ser erros semânticos. Revisa
a D28, que não fazia verificação de limites. Para índices e divisores não
constantes, o gerador insere a verificação em tempo de execução.

## D41 — DSL vetorial

Operações `+ - *` entre vetores (e entre vetor e escalar) em atribuição a um
vetor, e as funções embutidas `somavet`, `maxvet`, `minvet` e `prodesc`.
Especificação formal em [`dsl.md`](./dsl.md).

## D42 — Semântica de execução

Curto-circuito em `E`/`OU`; zero como valor inicial; `respost` da principal
como código de saída; parâmetros da principal lidos da entrada; igualdade de
`palavra` pelo conteúdo; avaliação da esquerda para a direita; formato de
impressão fixo (4 casas para `flut`, 8 para `duplocarpado`, `VER`/`FAL`).

## D43 — Erros de execução e estouro

Divisão por zero, índice fora dos limites, raiz de negativo e expoente
negativo em `inteira ^ inteira` interrompem o programa com
`Erro de execucao (linha L): ...`. Estouro inteiro dá a volta (complemento de
dois); `-2147483648 // -1` é `-2147483648`.

## D44 — E/S física (REVISADA pela D48)

> **Revisada pela D48 (placa Raspberry Pi Pico 2 W):** a placa principal
> passou a ser a Pico 2 W, e cada plataforma define quais pinos existem
> (`zl_pino_valido`): GP0–GP22, GP26–GP28 e 25 = LED na Pico; 2–19 no Uno.
> O PC simula a numeração da Pico. O texto abaixo é a decisão original.

Funções embutidas `pinomodo(pino, modo)`, `pinoescreve(pino, valor)`,
`pinole(pino)` e `espera(ms)`, registradas na tabela de símbolos. Pinos na
numeração da placa Arduino; no PC elas são simuladas.

## D45 — Alvo Arduino Uno (`--alvo=uno`) (REVISADA pela D48)

> **Revisada pela D48:** o Uno passou a ser o alvo secundário, testado no
> QEMU; a placa principal é a Raspberry Pi Pico 2 W (`--alvo=pico`). A
> verificação de memória abaixo foi generalizada para os dois alvos
> (`semantic_target_info`), cada um com a sua RAM e a sua reserva, e com
> `palavra` de 2 bytes no Uno e 4 na Pico. O texto abaixo é a decisão
> original e continua valendo para `--alvo=uno`.

`inteira` continua `i32` e `duplocarpado` continua `double` de 64 bits
(`avr-gcc -mdouble=64`). O alvo só muda **verificações**, nunca o IR:
`SEM_TARGET_MEMORY` quando globais + textos + o quadro de alguma função
passam de 2048 − 512 bytes.

Uma consequência no gerador, igual para todos os alvos: `flut ^ flut` (e
qualquer `^` com expoente real) é calculado em `double` (`fpext`,
`llvm.pow.f64`, `fptrunc`). Com `-mdouble=64`, o `powf` da avr-libc 2.3.2
entra em laço infinito no Uno. Em `double` o resultado arredondado para
`float` é o mesmo do PC. Expoente `inteira` continua usando `llvm.powi`.

## D46 — `tipo vet{N} a, b, c;`

A especificação oficial escreve a declaração de vetor como
`tipo vet{tamanho} nome, ... ;`. Um nome simples depois de um declarador
`vet{N}` na mesma declaração herda o mesmo `vet{N}`. Assim
`inteira vet{8} a, b;` declara dois vetores, como se lê na especificação
(antes, só `a` era vetor). `inteira x, vet{3} v;` continua com `x` escalar.

## D47 — Verificação de índice omitida quando provada (análise de faixa)

A D40 só dispensava a verificação de índice constante. No laço típico
`repete (i receba 0, i < N, i receba i + 1) edai [ ... v{i} ... ]`, o teste
`icmp ult` + desvio para `zl_erro_execucao` aparecia em toda iteração, e
impedia o vetorizador do LLVM de tratar o laço.

O gerador agora calcula a faixa da variável de controle quando o `repete` tem a
forma `i receba A; i < B (ou B > i); i receba i + K` e todas as condições abaixo
valem:

- `A`, `B` e `K` são constantes inteiras (D40), com `K ≥ 1`;
- `i` é variável **local** `inteira` (nenhuma chamada pode alterá-la);
- o corpo não atribui a `i` nem faz `le(i)`, nem dentro de comandos aninhados;
- `B − 1 + K` cabe em `i32`. Com a volta do complemento de dois (D43), o último
  incremento poderia deixar `i` negativo, e o laço continuaria.

Dentro do corpo, `A ≤ i ≤ B − 1`. A faixa de um índice formado por essas
variáveis, constantes, `+`, `-` e `*` é calculada por aritmética de
intervalos. Se a conta sair de `i32`, a faixa é desconhecida. Quando a faixa
cabe em `[0, N)`, a verificação não é emitida e o IR recebe um comentário
`; linha L: indice em [a, b] dentro de [0, N), sem verificacao (D47)`.

Em qualquer outro caso a verificação continua: faixa maior que o vetor, `i`
alterada no corpo, variável global, passo não constante.
`tests/execucao/19_faixa_indices.mac` cobre os casos com e sem verificação,
inclusive o erro de execução de um laço que passa do fim do vetor. Evidência
no IR em [`decisoes_backend.md`](./decisoes_backend.md), §6.4.

## D48 — Placa principal: Raspberry Pi Pico 2 W (`--alvo=pico`)

**Problema.** A Parte D foi feita para o Arduino Uno (D44, D45), mas a placa
que o grupo tem para a demonstração é a Raspberry Pi Pico 2 W. Além disso, o
clone do Uno usado até aqui fala com o PC por um conversor USB-serial CH340,
que precisou de driver.

**Decisão.** A placa principal passa a ser a Pico 2 W. O Uno continua como
alvo secundário (`--alvo=uno`, `make placa PLACA=uno`), testado sem hardware
no QEMU (`make test-placa-uno`).

| Item | Raspberry Pi Pico 2 W |
| --- | --- |
| Microcontrolador | RP2350A: ARM Cortex-M33 a 150 MHz, sem sistema operacional (o programa usa só o núcleo 0) |
| Memória | 520 KB de SRAM (512 KB principais + dois bancos de 4 KB, SCRATCH_X e SCRATCH_Y), 4 MB de flash |
| Chip sem fio | CYW43439 (Wi-Fi e Bluetooth não são usados); o LED da placa é ligado a ele |
| Tripla / CPU | `llc -mtriple=thumbv8m.main-none-eabi -mcpu=cortex-m33 -float-abi=soft` |
| Ligação | Pico SDK 2.3.1 (CMake + Ninja) com `arm-none-eabi-gcc` 16.2 |
| Executável | `placa/<programa>.uf2`, gerado pelo `picotool` 2.3.1 |
| E/S | USB da própria placa (porta COM virtual), D49 |

Motivos da troca:

- é a placa que o grupo tem;
- USB nativa: a própria RP2350 implementa a USB e aparece como porta COM sem
  instalar driver (o CH340 do clone do Uno precisou de driver);
- 256 vezes a RAM do Uno (512 KB contra 2 KB);
- FPU de precisão simples em hardware: as contas de `flut` viram instruções
  (`vadd.f32`, `vmul.f32`, ...), e não rotinas de software;
- é um segundo back-end do LLVM bem diferente para o **mesmo IR portável**:
  ARM de 32 bits (Thumb-2) em vez de AVR de 8 bits. O `.ll` do compilador
  não muda, o que reforça a evidência da Seção 4.5 do enunciado
  ([`decisoes_backend.md`](./decisoes_backend.md), §4).

**Tripla, CPU e ABI.** O IR continua sem `target triple` e sem `datalayout`;
o alvo só aparece no `llc` (`scripts/construir.sh`):

```sh
llc -mtriple=thumbv8m.main-none-eabi -mcpu=cortex-m33 -float-abi=soft -O2 \
    -function-sections -data-sections -filetype=obj programa.final.ll -o programa.o
```

- `thumbv8m.main`: o Cortex-M33 implementa a arquitetura ARMv8-M Mainline e
  só executa instruções Thumb. `none-eabi`: sem sistema operacional, com a
  ABI padrão do ARM (EABI).
- `-mcpu=cortex-m33` liga a FPU de precisão simples e as instruções DSP do
  M33.
- `-float-abi=soft` muda só a **convenção de chamada**: `float` e `double`
  passam em registradores inteiros (`r0`–`r3`), e as contas continuam usando
  a FPU. FPU ligada com ABI soft é o que o GCC chama de `softfp`, a ABI
  padrão do Pico SDK 2.x para a RP2350 (`-mfloat-abi=softfp`). É com ela que
  o `arm-none-eabi-gcc` compila a biblioteca de execução e o SDK. Assim,
  `zl_imprime_flut(float)`, chamada pelo objeto do `llc`, recebe o valor onde
  a função compilada pelo GCC o procura. Com `-float-abi=hard` o valor iria em
  `s0` e a biblioteca o leria em `r0`; o ligador do ARM recusa misturar
  objetos com ABIs de float diferentes.
- `duplocarpado` é `double` de 64 bits pela própria ABI do ARM, sem opção
  especial (no Uno foi preciso `-mdouble=64`, D45). A FPU do M33 não tem
  precisão dupla: as operações viram chamadas `__aeabi_dadd`,
  `__aeabi_dmul`, ..., que o SDK (`pico_double`) redireciona para rotinas que
  usam o coprocessador de precisão dupla (DCP) da RP2350.
- `-function-sections -data-sections`: cada função e cada global em sua
  própria seção, para a ligação do SDK (`--gc-sections`) descartar o que não
  é usado.

**Integração com o Pico SDK.** `runtime/pico/CMakeLists.txt`
(`PICO_BOARD=pico2_w`, `PICO_PLATFORM=rp2350-arm-s`) cria o executável a
partir de `runtime/zl_runtime_pico.c` e do objeto do programa
(`-DZL_PROGRAMA_OBJ=...`, um objeto externo). Liga `pico_stdlib`,
`pico_cyw43_arch_none` (driver do chip sem fio só para o LED, sem pilha de
rede) e `hardware_watchdog` (reinício, D49), com o `stdio` na USB e não na
UART. `pico_add_uf2_output` chama o `picotool` para gerar o `.uf2`. O
`construir.sh` configura o CMake uma vez por pasta (`build/pico-OFF` e, com o
sentinela dos testes, `build/pico-ON`); nas próximas vezes só troca o objeto
do programa e religa. `scripts/pico_sdk.sh` encontra o SDK.

**Por que não "bare metal"** (só `arm-none-eabi-gcc` e um script de ligação
próprio, como o `avr-gcc` no Uno):

- a E/S pela USB precisa de uma pilha USB de dispositivo (enumeração,
  descritores, classe CDC). O SDK traz a TinyUSB já ligada ao `stdio`;
- o LED da Pico 2 W não é um GPIO da RP2350: é um pino do CYW43439, que só
  responde depois que o driver carrega o firmware do chip por um barramento
  SPI feito com PIO. O SDK traz o `cyw43-driver`;
- a partida da RP2350 (o bloco `IMAGE_DEF` que a ROM de boot exige, relógios,
  execução a partir da flash) também vem pronta.

Escrever isso à mão seria a maior parte da Parte D e não tem relação com o
compilador. O código do programa continua vindo só do `llc`; o SDK entra como
biblioteca, como a avr-libc no Uno. O CMake é a forma suportada de usar o SDK:
ele gera o script de ligação, a configuração da placa e o `.uf2`.

**Verificação de memória (`--alvo=pico`).** A D45 foi generalizada:
`semantic_target_info()` devolve a RAM e a reserva de cada alvo, e
`check_target_memory()` aplica a mesma regra ao Uno e à Pico
(`SEM_TARGET_MEMORY`):

```text
globais + textos               <= 524288 - 65536 = 458752 bytes
globais + textos + quadro(f)   <= 458752 bytes, para toda função f
```

- 524 288 bytes são os 512 KB de SRAM principal. 64 KB ficam para o SDK, a
  pilha USB, o driver do CYW43439 e o heap (na Pico, `le` de `palavra` usa
  `malloc`).
- Os tamanhos são os do alvo: `palavra` (ponteiro) tem 4 bytes no ARM e 2 no
  AVR (`target_pointer_bytes()`); os outros tipos são iguais nos dois.
- O relatório mostra `[ALVO PICO 2 W]` e a estimativa, no mesmo formato do
  Uno.
- **Pilha.** A pilha do núcleo 0 começa no topo do banco SCRATCH_Y (4 KB; o
  SDK reserva ali no mínimo 2 KB, `PICO_STACK_SIZE`) e cresce para baixo:
  atravessa a SCRATCH_X, livre porque o núcleo 1 não é iniciado, e continua
  no topo da RAM principal. Por isso um quadro grande cabe sem aumentar a
  pilha, desde que o total caiba no limite acima. Como no Uno, a
  profundidade de recursão não é verificada.

**Pinos (D44 estendida).** `zl_pino_valido` saiu de `zl_runtime_comum.c`:
cada plataforma define os seus pinos (declaração em `zl_plataforma.h`).

| Plataforma | Pinos válidos |
| --- | --- |
| Pico 2 W | GP0–GP22 e GP26–GP28 (o número `n` é o GPn da placa) e **25 = LED da placa** |
| PC | a mesma numeração da Pico, simulada |
| Uno | 2–19 (0 e 1 são a serial), como antes |

- Na Pico 2 W, GP23, GP24, GP25 e GP29 são usados pelo chip sem fio. GP23,
  GP24 e GP29 dão erro de execução 7 (pino inválido), como qualquer número
  fora da tabela; o 25 foi reaproveitado para o LED.
- **Por que o LED é o 25.** Na Pico e na Pico 2 sem fio, o LED fica no GP25.
  Na versão W, o GP25 é o *chip select* do CYW43439, e o LED passou para um
  pino do próprio chip sem fio. Usar o 25 mantém o número que a documentação
  e os exemplos da Pico dão para o LED, sem criar função nova na linguagem. A
  biblioteca encaminha o pino 25 para
  `cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, ...)`; `pinomodo(25, m)` só
  valida os argumentos (o LED é sempre saída) e `pinole(25)` devolve o último
  valor escrito.
- **O PC simula a numeração da Pico**, para que o mesmo programa rode no PC e
  na placa com a mesma validação: um pino recusado na placa também é recusado
  no PC. O Uno mantém a numeração dele, então um programa que usa o LED 25
  não roda no Uno (erro 7). Os programas próprios da Pico ficam em
  `tests/placa/pico/` (LED no 25; botão no GP15, só manual).

**Consequência.** `--alvo=pico` (`src/main.c`, `src/semantic.c`),
`runtime/zl_runtime_pico.c`, `runtime/pico/` (`CMakeLists.txt`,
`pico_sdk_import.cmake`), ramo `pico` de `scripts/construir.sh`,
`scripts/pico_sdk.sh`; `make placa` gera `.uf2` (`PLACA ?= pico`;
`PLACA=uno` gera o `.hex`). Detalhes em [`placa.md`](./placa.md).

## D49 — E/S pela USB e ciclo de vida do programa na placa

**Problema.** Na Pico 2 W, `imprime` e `le` passam pela USB da própria placa
(D48). Não há conversor USB-serial: a porta COM só existe enquanto o
programa, com a pilha USB do SDK, está rodando, e o terminal sempre abre a
porta depois de o programa começar. Se o programa rodasse logo ao ligar, o
que ele imprimisse antes de o terminal abrir se perderia (num programa curto,
a saída inteira). No Uno isso não acontece: abrir o Monitor Serial reinicia a
placa (sinal DTR), e o programa roda de novo desde o começo.

**Decisão.**

| Situação | Comportamento (`runtime/zl_runtime_pico.c`) |
| --- | --- |
| Placa ligada ao PC | o programa espera um terminal abrir a porta (sinal DTR, `stdio_usb_connected()`); enquanto espera, o LED pisca devagar (troca a cada 0,5 s) |
| Sem PC (carregador ou bateria) | se nenhum PC enumerar a placa em 3 s, o programa roda direto; `le` encontra o fim da entrada (erro 6, como no PC com entrada vazia) |
| Terminal fechado | a placa reinicia pelo watchdog e volta a esperar; abrir o terminal de novo roda o programa de novo, como o Uno ao abrir o Monitor Serial. A verificação é feita em `le`, em `espera` e depois do fim do programa |
| Erro de execução | mensagem pela USB e LED piscando rápido (troca a cada 100 ms) |
| Fim normal | LED aceso fixo |
| Porta aberta a 1200 baud | o SDK reinicia a placa em modo BOOTSEL; `make gravar` usa isso para gravar sem apertar o botão. Só a primeira gravação exige segurar BOOTSEL ao ligar o cabo |
| Compilado com `-DZL_SENTINELA` | byte EOT (0x04) no fim, para os testes automáticos saberem onde a saída termina (como no QEMU) |
| Fim de linha | o `stdio` do SDK converte `\n` em `\r\n`; os scripts de teste retiram o `\r` antes de comparar |

**Alternativas consideradas.**

1. **Começar o programa logo ao ligar**, como um programa comum do SDK: a
   saída impressa antes de o terminal abrir se perde, e depois de cada
   gravação a porta some e volta, então o terminal sempre chega atrasado.
   Rejeitada.
2. **Saída pela UART (GP0/GP1) com um adaptador USB-serial**: funcionaria
   como no Uno, mas exige um adaptador e, em geral, driver (o que a troca de
   placa quis evitar) e ocupa dois pinos. Rejeitada.

**Justificativa.** Para quem usa, o comportamento fica igual ao do Uno:
abrir o terminal mostra a saída completa desde o início. Sem terminal, o LED
informa o estado (piscando devagar: esperando; fixo: terminou; rápido: erro),
e a gravação não exige apertar o botão.

**Consequência.** `scripts/pico.py` (`gravar`, `monitor`, `executa`,
`portas`), `make gravar`, `make monitor` e `make test-pico`
(`tests/run_pico.py`, na placa real, fora do `make test`). Um terminal que
não liga o DTR não é reconhecido; os terminais comuns (o `miniterm` do
pyserial, o Monitor Serial da IDE do Arduino) ligam. Para ver a saída, não
abra a porta a 1200 baud: isso reinicia a placa em BOOTSEL.

## D50 — Literal inteiro maior que `i32`: aviso, não erro (revisa D38)

**Problema.** Os programas de teste do professor (defesa do PjBL 1/2,
`tests/professor/`) guardam CPFs de 11 dígitos em `inteira`
(`documentos{0} receba 12345678901;`). O PjBL 1 aceitava esses programas; a
D38 passou a recusá-los (`SEM_LITERAL_OUT_OF_RANGE`), uma regressão em relação
ao que o professor já tinha testado.

**Decisão.** Literal inteiro que não cabe em `i32` é aceito com **aviso**
(`[AVISOS SEMANTICOS]`, também com `-q`, na saída de erro), e o valor é
**reduzido módulo 2³²** e lido em complemento de dois:
`12345678901` vira `-539222987`. O programa continua válido e o `.ll` é gerado.
Os dois geradores (LLVM IR e MLIR) usam o mesmo valor
(`ast_integer_literal_i32`). Literal **decimal** maior que o máximo de `flut`
continua erro.

**Justificativa.** É a mesma política do estouro aritmético (D43): `inteira`
é `i32` em todo alvo (D16, D45), e o que não cabe dá a volta, como no
hardware. O aviso deixa claro que o valor mudou, sem recusar um programa que a
especificação não proíbe (ela não fixa a largura de `inteira`).

**Consequência.** `nivel4` e `nivel5` voltam a compilar e a rodar. O valor
guardado não é o CPF original (para isso a linguagem precisaria de um inteiro
de 64 bits). `-2147483648` também passa a ser aceito (com aviso) e dá o valor
certo. Testes: `literal_wrap_warning` (unitário) e `tests/professor/`.

## D51 — Variável de registro sem a palavra `registro`

**Problema.** O `nivel3` do professor declara `Cuidador responsavel;`, só com
o nome do registro. A D39 exigia `registro Cuidador responsavel;`, e o
programa era rejeitado (já no PjBL 1, com "token inesperado").

**Decisão.** As duas formas são aceitas, no nível superior e em blocos, para
variável e vetor: `Cuidador x;`, `Cuidador vet{3} equipe;` e as formas com
`registro`. No parser, um identificador seguido de outro identificador (ou de
`vet`) só pode ser declaração: atribuição exige `receba` e chamada exige `(`.
Por isso não há ambiguidade, e a gramática continua LL(2) nesse ponto. A
semântica confere se o nome é mesmo um registro (`SEM_UNDECLARED_IDENTIFIER`
ou `SEM_NAME_KIND_MISMATCH`, como na D39).

**Justificativa.** É a forma que o professor usa, e é a forma de C/Java
(`Tipo nome;`). A especificação só define a sintaxe da definição do registro,
não a da variável.

**Consequência.** O `nivel3` compila e roda. Testes: `records_as_types`
(unitário) e `tests/professor/`.

## Registro resumido das decisões consolidadas

Cada decisão abaixo registra problema, decisão, justificativa e consequência.

| ID | Problema | Decisão | Justificativa | Consequência |
|---|---|---|---|---|
| D15 | Decimal e três tipos numéricos | Decimal é flut; ordem inteira < flut < duplocarpado | Fecha lacuna compatível com os tipos existentes | Define ranks e promoção |
| D16 | Compatibilidade entre tipos | Promoção ascendente; sem narrowing; bool/palavra exatos | Evita conversões implícitas não documentadas | Uma matriz única serve a atribuição, argumentos e retornos |
| D17 | Resultado de +, -, * e ^ | Operandos numéricos; resultado mais largo | Regra numérica uniforme | Palavra e bool não participam |
| D18 | Resultado de / | Todo par numérico produz flut | Divisão é conceitualmente fracionária; evita truncamento silencioso | Mesmo duplo/duplo resulta flut |
| D19 | Divisão inteira | Somente inteira/inteira; resultado inteira | Operador tem finalidade própria | Outros pares são erros |
| D20 | Resultado de raiz | Numérico para flut | Regra específica do operador | Não aceita bool, palavra ou vazio |
| D21 | Ordem | < e > só numéricos; resultado bool | Compatível com comparações numéricas | Mistura numérica usa promoção conceitual |
| D22 | Igualdade | == e ! exigem mesmo tipo | Evita promoção implícita não indicada | Inteira e flut não são comparadas por igualdade |
| D23 | Lógica | E, OU, XOR e NÃO usam bool | Exclui truthiness | Resultado sempre bool |
| D24 | Escopos | Global, função e blocos aninhados | AstBlock representa blocos | Body direto da função não duplica escopo |
| D25 | Nomes em escopos | Namespace único; redeclaração no mesmo; shadowing em filho | Permite ocultação local controlada e resolve colisões | Lookup percorre pais e a categoria é validada depois |
| D26 | Ordem de resolução | Duas passadas; globals e assinaturas antes dos corpos | Permite globals posteriores, forward calls e recursão | Locais ainda respeitam ordem sequencial |
| D27 | Programa sem principal ou múltiplas principais | Exatamente uma principal; assinatura segue a gramática; não é chamável | Fecha a entrada do programa sem alterar o parser | Passada 1 conta principais e diagnostica ausência/duplicidade |
| D28 | Vetores | Índice inteira; sem verificação de limites | Fecha tipo do índice e evita análise não exigida | Bounds é limitação documentada |
| D29 | Registros | Duplicata de registro/campo é erro | São nomes declarados sem operações de campo | Não cria tipo de registro |
| D30 | Retorno incompleto | *(substituída pela D34)* | — | — |
| D31 | Lacuna de entrada e saída | Adotar `le(LVALUE);` e `imprime(EXPRESSAO);` como statements próprios | O requisito de entrega exige cobertura, mas a especificação original não define sintaxe | Extensão do grupo; front-end valida, sem runtime |
| D32 | Tipo do unário + | Numérico para o mesmo tipo | Simetria com unário - sem conversão | bool, palavra e vazio são inválidos |
| D33 | Função vazio não podia ser chamada (D13 + D16) | Chamada isolada `f(x);` é comando; único contexto que aceita vazio | Solução usual, sem ambiguidade LL(1) | Substitui D13; nó `AST_STMT_CALL` |
| D34 | Função sem retorno em algum caminho | `SEM_MISSING_RETURN`, exceto em `principal` (zero implícito) | Necessário para gerar código com valor definido | Substitui D30; análise conservadora de fluxo |
| D35 | Estouro de pilha com aninhamento extremo | Limites 256 (aninhamento) e 1024 (altura de expressão) | Limita a altura da AST e torna seguras todas as travessias | Um erro sintático controlado |
| D36 | Erros em cascata e relatório ambíguo | `;` inserido virtualmente; modo pânico; status `PARCIAL`/`NAO EXECUTADO` | Cada erro real relatado uma vez | Contagem de erros suprimidos no relatório |
| D37 | Acento em identificador gerava um erro por caractere | Palavra inteira = um identificador inválido | Erro único e sem cascata sintática | Colunas contam caracteres |
| D38 | `duplo / duplo` era `flut` (perda de precisão) | `/` e `raiz` produzem `duplocarpado` se houver operando `duplocarpado`; literal promovido convertido do texto | Nenhum estreitamento implícito (D16) | Revisa D18/D20 |
| D39 | Registros declarados mas inutilizáveis | `registro R p;`, `p.campo`, `v{i}.campo`, cópia `p receba q` | O PjBL 3 exige `%struct` + GEP | Token `TOK_PONTO` |
| D40 | Erros prováveis só em execução | Índice constante fora dos limites e divisor constante zero são erros semânticos | Detectar o que é provável antes | Revisa D28 |
| D41 | Sem operações vetoriais | DSL `+ - *`, `somavet`, `maxvet`, `minvet`, `prodesc` | PjBL 4, Parte A | Núcleos MLIR |
| D42 | Semântica de execução indefinida | Curto-circuito, zero inicial, código de saída, formatos | Programas determinísticos e iguais em todo alvo | `decisoes_backend.md` |
| D43 | Divisão por zero, índice, estouro | Erro de execução com linha; estouro dá a volta | Comportamento definido, nunca UB | Códigos 2 a 7 |
| D44 | E/S física | `pinomodo`, `pinoescreve`, `pinole`, `espera` | PjBL 4, Parte D | Simuladas no PC |
| D45 | Alvo de 8 bits com 2 KB | `i32` e `double` de 64 bits mantidos; verificação de memória | Mesmo resultado do PC | `--alvo=uno` |
| D46 | `tipo vet{N} a, b;` | Todos os nomes são vetores de N | Forma da especificação oficial | Revisa a leitura da D05 |
| D47 | Verificação de índice em todo acesso de laço | Análise de faixa em `repete` com limites constantes; verificação omitida quando o índice cabe em `[0, N)` | Custo zero quando provado, sem perder segurança | Complementa D40 |
| D48 | A placa do grupo é a Raspberry Pi Pico 2 W, não o Uno | Pico 2 W como placa principal (`--alvo=pico`): `llc -mtriple=thumbv8m.main-none-eabi -mcpu=cortex-m33 -float-abi=soft` (softfp, a ABI do SDK) + Pico SDK (CMake); memória ≤ 512 KB − 64 KB; LED = pino 25; PC simula os pinos da Pico | USB nativa sem driver, 256× a RAM, FPU; segundo back-end para o mesmo IR | Revisa D44 e D45; Uno vira alvo secundário (QEMU) |
| D50 | CPF de 11 dígitos em `inteira` nos testes do professor era recusado (D38) | Literal inteiro maior que `i32` é aceito com aviso e reduzido módulo 2³² | Mesma política do estouro (D43); o PjBL 1 aceitava | Revisa D38; `tests/professor/` |
| D51 | `Cuidador x;` (sem `registro`) era erro de sintaxe | Aceitar `Tipo nome;` além de `registro Tipo nome;` | Forma usada pelo professor; ID ID só pode ser declaração | Complementa D39 |
| D49 | Sem conversor USB-serial, a saída anterior à abertura do terminal se perderia | Programa espera o terminal (DTR); sem PC, roda após 3 s; fechar o terminal reinicia a placa; LED mostra o estado; 1200 baud = BOOTSEL | Mesmo modelo do Uno (abrir o monitor roda do começo), sem adaptador | `scripts/pico.py`, `make gravar`/`monitor`/`test-pico` |

## Estado final da lacuna de entrada e saída

A decisão D31 fecha essa lacuna. A especificação original permanece sem
sintaxe de I/O; a linguagem efetiva do projeto possui a extensão
`le(LVALUE);` e `imprime(EXPRESSAO);`, validada pelo front-end e demonstrada
em [`../tests/validos/06_io.mac`](../tests/validos/06_io.mac).
