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

## Decisões dos PjBL 3 e 4 (D38 a D47)

Detalhes e evidências em [`decisoes_backend.md`](./decisoes_backend.md),
[`dsl.md`](./dsl.md) e [`placa.md`](./placa.md).

## D38 — Divisão, raiz e literais na geração de código (revisa D18 e D20)

- `/` continua sendo sempre divisão real (`inteira / inteira` é `flut`), mas
  o resultado passa a ser `duplocarpado` quando algum operando for
  `duplocarpado`. O mesmo vale para `raiz`. Antes, `duplocarpado / duplocarpado`
  era `flut`, o que exigiria um `fptrunc` e perderia precisão sem aviso, contra
  o espírito da D16 (nenhum estreitamento implícito).
- Literal inteiro fora de `i32` é `SEM_LITERAL_OUT_OF_RANGE`.
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

## D44 — E/S física

Funções embutidas `pinomodo(pino, modo)`, `pinoescreve(pino, valor)`,
`pinole(pino)` e `espera(ms)`, registradas na tabela de símbolos. Pinos na
numeração da placa Arduino; no PC elas são simuladas.

## D45 — Alvo Arduino Uno (`--alvo=uno`)

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

## Estado final da lacuna de entrada e saída

A decisão D31 fecha essa lacuna. A especificação original permanece sem
sintaxe de I/O; a linguagem efetiva do projeto possui a extensão
`le(LVALUE);` e `imprime(EXPRESSAO);`, validada pelo front-end e demonstrada
em [`../tests/validos/06_io.mac`](../tests/validos/06_io.mac).
