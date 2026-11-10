# Gramática efetiva da Macaronica

Esta é a gramática efetiva da Macaronica, reconhecida pelo parser implementado em [`../src/parser.c`](../src/parser.c), depois das decisões registradas em [`decisoes.md`](./decisoes.md). Ela usa uma notação EBNF simplificada:

- aspas simples indicam terminais literais;
- palavras em maiúsculas indicam não-terminais ou classes de tokens;
- `*` significa zero ou mais ocorrências;
- `?` significa zero ou uma ocorrência;
- `|` separa alternativas.

## 1. Programa e itens de nível superior

```ebnf
PROGRAMA       -> ITEM* EOF

ITEM           -> REGISTRO
                | FUNCAO
                | DECLARACAO
```

## 2. Registros

```ebnf
REGISTRO       -> 'registro' ID '@' '[' CAMPO* ']'
CAMPO          -> TIPO_VALOR ID ';'
```

## 3. Funções

```ebnf
FUNCAO         -> FUNCAO_NAO_PRINCIPAL
                | FUNCAO_PRINCIPAL

FUNCAO_NAO_PRINCIPAL
                -> 'outrafuncao' TIPO_RETORNO ID
                   '(' PARAMETROS? ')' '@' BLOCO

FUNCAO_PRINCIPAL
                -> TIPO_RETORNO 'principal'
                   '(' PARAMETROS? ')' '@' BLOCO

PARAMETROS     -> PARAMETRO (',' PARAMETRO)*
PARAMETRO      -> TIPO_VALOR ID

BLOCO          -> '[' COMANDO* ']'
```

Uma função `vazio` pode terminar normalmente ao fim de seu bloco. Não existe produção para `respost ;`.

`RETORNO -> 'respost' EXPRESSAO ';'` é uma regra sintática geral. A análise semântica rejeita um retorno com valor dentro de uma função `vazio`.

## 4. Tipos e declarações

```ebnf
TIPO_VALOR     -> 'bool'
                | 'inteira'
                | 'flut'
                | 'palavra'
                | 'duplocarpado'

TIPO_RETORNO   -> TIPO_VALOR
                | 'vazio'

DECLARACAO     -> TIPO_VALOR DECLARADOR (',' DECLARADOR)* ';'
                | 'registro' ID DECLARADOR (',' DECLARADOR)* ';'   (D39)

DECLARADOR     -> ID
                | 'vet' '{' NUM_INTEIRO '}' ID
```

Um `DECLARADOR` simples que vem depois de um `vet{N}` na mesma declaração
herda o `vet{N}` (D46): `inteira vet{8} a, b;` declara dois vetores, na forma
`tipo vet{tamanho} nome, ... ;` da especificação.

A declaração de variável registro pode aparecer como item global e como
comando. No nível superior, `'registro' ID` seguido de `@` é a definição do
registro (`REGISTRO`); seguido de declarador, é uma variável.

## 5. Comandos

```ebnf
COMANDO        -> DECLARACAO
                | ATRIBUICAO
                | CHAMADA_CMD
                | IO
                | DECISAO
                | REPETICAO_FOR
                | REPETICAO_WHILE
                | RETORNO
                | BLOCO

IO             -> 'le' '(' LVALUE ')' ';'
                | 'imprime' '(' EXPRESSAO ')' ';'

ATRIBUICAO     -> LVALUE 'receba' EXPRESSAO ';'

CHAMADA_CMD    -> ID '(' ARGUMENTOS? ')' ';'        (D33)

RETORNO        -> 'respost' EXPRESSAO ';'

DECISAO        -> 'cond' '(' CONDICAO ')' 'edai' BLOCO
                   RAMO_ALTERNATIVO?

RAMO_ALTERNATIVO
                -> 'casocontrario' 'cond' '(' CONDICAO ')' 'edai' BLOCO
                   RAMO_ALTERNATIVO?
                | 'casocontrario' 'edai' BLOCO

REPETICAO_FOR  -> 'repete' '('
                   ATRIBUICAO_CABECALHO ','
                   CONDICAO ','
                   ITERACAO
                   ')' 'edai' BLOCO

ATRIBUICAO_CABECALHO
                -> LVALUE 'receba' EXPRESSAO

ITERACAO       -> LVALUE 'receba' EXPRESSAO

REPETICAO_WHILE
                -> 'durante' '(' CONDICAO ')' 'edai' BLOCO
```

`ATRIBUICAO` e `CHAMADA_CMD` começam ambas por `ID`; o token seguinte decide
(`(` é chamada; `receba` ou `{` é atribuição). Uma chamada isolada é o único
contexto em que uma função `vazio` pode ser chamada (D33).

Limites de aninhamento (D35): no máximo 256 níveis de parênteses, argumentos,
índices, blocos, unários, `NÃO` e `^`, e altura de expressão de no máximo 1024.
Acima disso o parser relata um erro sintático em vez de estourar a pilha.

`le` e `imprime` são statements próprios definidos posteriormente pela
decisão D31; não são chamadas de função. `le` reutiliza exatamente
`LVALUE`, portanto são sintaticamente válidos `le(x);` e `le(v{i});`, mas não
`le(1);`, `le(x + 1);`, `le(f());` ou `le();`. `imprime` reutiliza a produção
normal de `EXPRESSAO` e recebe exatamente uma expressão: `imprime(1);`,
`imprime("texto");`, `imprime(x + 1);` e `imprime(soma(1, 2));` são válidos,
enquanto `imprime();` e `imprime(x, y);` são rejeitados.

## 6. Condições e expressões booleanas

As três formas oficiais aparecem na forma `VALOR COMPARADOR VALOR`: uma variável ou um número pode ocupar cada lado do comparador. A gramática efetiva também permite expressões, booleanos e operadores lógicos. `CONDICAO` é um alias conceitual para `EXPRESSAO`; a análise semântica exige que o resultado usado em `cond`, `repete` e `durante` seja `bool`.

```ebnf
CONDICAO       -> EXPRESSAO

EXPRESSAO      -> COND_OU

COND_OU        -> COND_XOR ('OU' COND_XOR)*
COND_XOR       -> COND_E ('XOR' COND_E)*
COND_E         -> COND_NAO ('E' COND_NAO)*
COND_NAO       -> 'NÃO' COND_NAO
                | COND_ATOMO

COND_ATOMO     -> VALOR COMPARACAO_RESTO?

COMPARACAO_RESTO
                -> COMPARADOR VALOR

COMPARADOR     -> '<' | '>' | '==' | '!'
BOOLEANO       -> 'VER' | 'FAL'
```

Exemplos aceitos por esta regra:

```text
numero > 20
numeroA < numeroB
(numero > 0) E (numero < 10)
NÃO (resultado == 0)
VER
```

## 7. Valores e expressões aritméticas

```ebnf
VALOR          -> ARITMETICA
                | BOOLEANO
                | TEXTO

ARITMETICA     -> TERMO (('+' | '-') TERMO)*

TERMO          -> POTENCIA (('*' | '/' | '//') POTENCIA)*

POTENCIA       -> UNARIO ('^' POTENCIA)?

UNARIO         -> ('+' | '-') UNARIO
                | PRIMARIO

PRIMARIO       -> NUM
                | 'raiz' '(' EXPRESSAO ')'
                | '(' EXPRESSAO ')'
                | REFERENCIA

REFERENCIA     -> ID '(' ARGUMENTOS? ')'
                | ID INDICE? CAMPO?

ARGUMENTOS     -> EXPRESSAO (',' EXPRESSAO)*

LVALUE         -> ID INDICE? CAMPO?
INDICE         -> '{' EXPRESSAO '}'
CAMPO          -> '.' ID                                (D39)
```

`REFERENCIA` permite uma variável (`ID`), uma chamada (`ID(...)`), um acesso
a vetor (`ID{...}`) e um acesso a campo de registro (`p.x`, `v{i}.x`). Não há
encadeamento além disso (campos são sempre escalares, D39).

As operações da DSL vetorial (D41) usam estas mesmas produções; a análise
semântica distingue expressões escalares e vetoriais. Ver
[`dsl.md`](./dsl.md).

## 8. Tokens léxicos

```ebnf
ID             -> LETRA (LETRA | DIGITO)*
NUM_INTEIRO    -> DIGITO+
NUM            -> DIGITO+ ('.' DIGITO+)?
TEXTO          -> '"' CARACTERE_TEXTO* '"'
LETRA          -> 'A'..'Z' | 'a'..'z'
DIGITO         -> '0'..'9'
```

`CARACTERE_TEXTO` pode ser qualquer caractere da linha diferente de aspas duplas e quebra de linha. Palavras reservadas têm prioridade sobre `ID`, e `//` tem prioridade sobre `/` por ser o lexema mais longo.
