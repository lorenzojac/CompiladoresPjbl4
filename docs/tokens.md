# Especificação léxica da Macaronica

Este documento define, exclusivamente, os tokens que o lexer da Macaronica reconhece. Ele foi elaborado a partir de:

- [`especificacao_macaronica.md`](./especificacao_macaronica.md);
- [`README_tabela.md`](./README_tabela.md);
- [`decisoes.md`](./decisoes.md);
- [`README_gramatica.md`](./README_gramatica.md).

Este arquivo é a referência léxica da linguagem; a implementação correspondente está em [`../src/lexer.c`](../src/lexer.c) e [`../src/token.h`](../src/token.h).

## 1. Convenções

- Os nomes `TokenType` abaixo correspondem aos definidos em `../src/token.h`.
- Palavras reservadas são reconhecidas antes de um `ID` com o mesmo texto.
- A linguagem é case-sensitive.
- `NÃO` deve ser escrito exatamente com o acento e em UTF-8. `NAO` não é equivalente; pela regra de identificadores, `NAO` pode ser um `ID` comum.
- `raiz()` não é um token único: o lexer produz `TOK_RAIZ`, seguido de `TOK_ABRE_PAREN` e, depois, `TOK_FECHA_PAREN`.
- `//` é divisão inteira e não inicia comentário.
- `=` isolado não possui token próprio. Embora apareça no alfabeto oficial, somente `==` é comparador; `=` isolado produz `TOK_INVALIDO`.

## 2. Lista definitiva de `TokenType`

### Identificadores e literais

```text
TOK_ID
TOK_NUM_INTEIRO
TOK_NUM_DECIMAL
TOK_TEXTO
TOK_VER
TOK_FAL
```

### Palavras reservadas oficiais

```text
TOK_RECEBA
TOK_REPETE
TOK_DURANTE
TOK_COND
TOK_CASOCONTRARIO
TOK_FUNC
TOK_BOOL
TOK_INTEIRA
TOK_FLUT
TOK_PALAVRA
TOK_DUPLOCARPADO
TOK_PRINCIPAL
TOK_VAZIO
TOK_RESPOST
```

`TOK_FUNC` existe no lexer porque `func` está na lista oficial de palavras reservadas. A gramática efetiva, porém, usa somente `outrafuncao` para iniciar uma função não-principal.

### Palavras e tokens adicionais definidos pelo projeto

```text
TOK_REGISTRO
TOK_OUTRAFUNCAO
TOK_VET
TOK_EDAI
TOK_RAIZ
TOK_LE
TOK_IMPRIME
```

### Operadores lógicos

```text
TOK_OU
TOK_E
TOK_NAO
TOK_XOR
```

`TOK_NAO` é o nome de implementação para o lexema oficial `NÃO`.

### Operadores aritméticos

```text
TOK_MAIS
TOK_MENOS
TOK_MULT
TOK_DIV
TOK_DIV_INT
TOK_POTENCIA
```

### Comparadores

```text
TOK_MENOR
TOK_MAIOR
TOK_IGUAL_IGUAL
TOK_DIFERENTE
```

### Símbolos estruturais

```text
TOK_ABRE_COLCHETE
TOK_FECHA_COLCHETE
TOK_ABRE_CHAVE
TOK_FECHA_CHAVE
TOK_ABRE_PAREN
TOK_FECHA_PAREN
TOK_PONTO_VIRGULA
TOK_VIRGULA
TOK_ARROBA
```

### Tokens internos

```text
TOK_EOF
TOK_INVALIDO
```

## 3. Tabela definitiva de tokens

| TokenType | Lexema/padrão | Categoria | Exemplo | Origem |
| --- | --- | --- | --- | --- |
| `TOK_ID` | `LETRA (LETRA \| DIGITO)*` | identificador | `contador10` | decisão de projeto (D03) |
| `TOK_NUM_INTEIRO` | `DIGITO+` | literal numérico inteiro | `42` | decisão de projeto (D03) |
| `TOK_NUM_DECIMAL` | `DIGITO+ '.' DIGITO+` | literal numérico decimal | `3.14` | decisão de projeto (D02/D03) |
| `TOK_TEXTO` | `"` texto `"`, sem quebra de linha | literal textual | `"mensagem"` | decisão de projeto (D02/D03) |
| `TOK_VER` | `VER` | literal booleano verdadeiro | `VER` | especificação oficial |
| `TOK_FAL` | `FAL` | literal booleano falso | `FAL` | especificação oficial |
| `TOK_RECEBA` | `receba` | palavra reservada / atribuição | `x receba 10;` | especificação oficial |
| `TOK_REPETE` | `repete` | palavra reservada / repetição `for` | `repete (...) edai [...]` | especificação oficial |
| `TOK_DURANTE` | `durante` | palavra reservada / repetição `while` | `durante (...) edai [...]` | especificação oficial |
| `TOK_COND` | `cond` | palavra reservada / decisão | `cond (...) edai [...]` | especificação oficial |
| `TOK_CASOCONTRARIO` | `casocontrario` | palavra reservada / ramo alternativo | `casocontrario edai [...]` | especificação oficial |
| `TOK_FUNC` | `func` | palavra reservada oficial | fragmento lexical: `func` | especificação oficial; sem produção sintática na decisão D01 |
| `TOK_BOOL` | `bool` | palavra reservada / tipo | `bool ativo;` | especificação oficial |
| `TOK_INTEIRA` | `inteira` | palavra reservada / tipo | `inteira contador;` | especificação oficial |
| `TOK_FLUT` | `flut` | palavra reservada / tipo | `flut medida;` | especificação oficial |
| `TOK_PALAVRA` | `palavra` | palavra reservada / tipo | `palavra nome;` | especificação oficial |
| `TOK_DUPLOCARPADO` | `duplocarpado` | palavra reservada / tipo | `duplocarpado valor;` | especificação oficial |
| `TOK_PRINCIPAL` | `principal` | palavra reservada / função principal | `inteira principal() @ [...]` | especificação oficial |
| `TOK_VAZIO` | `vazio` | palavra reservada / tipo de retorno | `vazio principal() @ [...]` | especificação oficial |
| `TOK_RESPOST` | `respost` | palavra reservada / retorno | `respost resultado;` | especificação oficial |
| `TOK_REGISTRO` | `registro` | palavra estrutural reservada | `registro Pessoa @ [...]` | derivado de estrutura oficial; decisão D01 |
| `TOK_OUTRAFUNCAO` | `outrafuncao` | palavra estrutural reservada / função | `outrafuncao inteira f() @ [...]` | derivado de estrutura oficial; decisão D01 |
| `TOK_VET` | `vet` | palavra estrutural reservada / vetor | `inteira vet{10} valores;` | derivado de estrutura oficial; decisão D01 |
| `TOK_EDAI` | `edai` | palavra estrutural reservada / introdução de bloco | `cond (x > 0) edai [...]` | derivado de estrutura oficial; decisão D01 |
| `TOK_RAIZ` | `raiz` | palavra reservada / operador aritmético | `raiz(9)` | especificação oficial (`raiz()`); reservado por decisão D01 |
| `TOK_LE` | `le` | palavra reservada / statement de entrada | `le(x);` | extensão do grupo definida em D31; não consta da especificação original |
| `TOK_IMPRIME` | `imprime` | palavra reservada / statement de saída | `imprime(x);` | extensão do grupo definida em D31; não consta da especificação original |
| `TOK_OU` | `OU` | operador lógico | `a > 0 OU b > 0` | especificação oficial |
| `TOK_E` | `E` | operador lógico | `a > 0 E b > 0` | especificação oficial |
| `TOK_NAO` | `NÃO` | operador lógico de negação | `NÃO (x == 0)` | especificação oficial; grafia UTF-8 confirmada em D03 |
| `TOK_XOR` | `XOR` | operador lógico | `a > 0 XOR b > 0` | especificação oficial |
| `TOK_MAIS` | `+` | operador aritmético | `a + b` | especificação oficial |
| `TOK_MENOS` | `-` | operador aritmético / sinal unário | `a - b` | especificação oficial |
| `TOK_MULT` | `*` | operador aritmético | `a * b` | especificação oficial |
| `TOK_DIV` | `/` | operador aritmético | `a / b` | especificação oficial |
| `TOK_DIV_INT` | `//` | operador aritmético / divisão inteira | `a // b` | especificação oficial; prioridade de maior casamento em D02 |
| `TOK_POTENCIA` | `^` | operador aritmético / potência | `a ^ b` | especificação oficial |
| `TOK_MENOR` | `<` | comparador | `a < b` | especificação oficial |
| `TOK_MAIOR` | `>` | comparador | `a > b` | especificação oficial |
| `TOK_IGUAL_IGUAL` | `==` | comparador de igualdade | `a == b` | especificação oficial; prioridade de maior casamento |
| `TOK_DIFERENTE` | `!` | comparador de diferença | `a ! b` | especificação oficial |
| `TOK_ABRE_COLCHETE` | `[` | símbolo estrutural | `[...]` | especificação oficial |
| `TOK_FECHA_COLCHETE` | `]` | símbolo estrutural | `[...]` | especificação oficial |
| `TOK_ABRE_CHAVE` | `{` | símbolo estrutural / vetor | `vet{10}` ou `v{1}` | especificação oficial |
| `TOK_FECHA_CHAVE` | `}` | símbolo estrutural / vetor | `vet{10}` ou `v{1}` | especificação oficial |
| `TOK_ABRE_PAREN` | `(` | símbolo estrutural | `principal()` | especificação oficial |
| `TOK_FECHA_PAREN` | `)` | símbolo estrutural | `principal()` | especificação oficial |
| `TOK_PONTO_VIRGULA` | `;` | terminador de comando | `inteira x;` | especificação oficial |
| `TOK_VIRGULA` | `,` | separador | `inteira x, y;` | derivado de estrutura oficial; decisão D02 |
| `TOK_ARROBA` | `@` | marcador estrutural de declaração | `principal() @ [...]` | derivado de estrutura oficial; decisão D02 |
| `TOK_PONTO` | `.` seguido de letra | acesso a campo de registro | `p.x`, `v{i}.nota` | PjBL 3, decisão D39; `.5` continua inválido |
| `TOK_EOF` | nenhum lexema | fim da entrada | após o último token | decisão de implementação necessária ao lexer |
| `TOK_INVALIDO` | qualquer lexema não reconhecido | erro léxico | `=` ou `#` | decisão de implementação; regra de erro léxico |

## 4. Regras léxicas

### 4.1 Identificador

```ebnf
ID -> LETRA (LETRA | DIGITO)*
LETRA -> 'A'..'Z' | 'a'..'z'
DIGITO -> '0'..'9'
```

Não é permitido `_`. Um texto que coincida exatamente com uma palavra reservada ou token nomeado é classificado como a palavra reservada correspondente, não como `TOK_ID`.

`NÃO` é uma exceção lexical reservada: como contém `Ã`, deve ser reconhecido diretamente pelo lexer em sua sequência UTF-8 oficial, antes ou em paralelo ao reconhecimento de identificadores ASCII. Ele não é produzido pelo reconhecimento genérico de `ID`.

`le` e `imprime` são palavras reservadas adicionadas pela decisão D31. A
comparação continua case-sensitive e exige coincidência do lexema inteiro:

```text
le        -> TOK_LE
imprime   -> TOK_IMPRIME
Le        -> TOK_ID
IMPRIME   -> TOK_ID
leia      -> TOK_ID
imprimir  -> TOK_ID
le1       -> TOK_ID
imprime1  -> TOK_ID
```

As grafias `leia` e `escreva` não pertencem à linguagem: foram alternativas
consideradas e não adotadas (ver D31).

### 4.2 Números

```ebnf
NUM_INTEIRO -> DIGITO+
NUM_DECIMAL -> DIGITO+ '.' DIGITO+
```

O ponto só pertence a um `TOK_NUM_DECIMAL` completo. Não existe `TOK_PONTO`. Exemplos como `3.` e `.5` não formam `TOK_NUM_DECIMAL` e resultam em erro léxico conforme a estratégia de recuperação do lexer.

Os sinais `+` e `-` são tokens próprios (`TOK_MAIS` e `TOK_MENOS`); não fazem parte do lexema numérico.

Não foram definidos expoentes, separadores de milhares ou outras formas numéricas.

### 4.3 Texto

```ebnf
TEXTO -> '"' CARACTERE_TEXTO* '"'
```

O texto começa e termina com aspas duplas e deve permanecer na mesma linha. Não são definidos escapes, interpolação ou continuação de texto; o lexer não reconhece essas formas.

### 4.4 Espaços e posição

Espaços, tabulações e quebras de linha são ignorados como tokens. Mesmo ignorados, devem atualizar corretamente a posição da entrada:

- a linha deve avançar ao encontrar uma quebra de linha;
- a coluna deve avançar ao consumir cada caractere ignorado ou pertencente a um token;
- a posição associada a cada token deve permitir diagnósticos posteriores de linha e coluna.

A convenção recomendada para a implementação é iniciar linha e coluna em `1`, sem transformar tabulação em múltiplas colunas.

### 4.5 Comentários

A Macaronica não possui comentários. Portanto, `//` sempre é tokenizado como `TOK_DIV_INT`; ele nunca inicia comentário de linha.

## 5. Prioridade de reconhecimento

O lexer aplica as seguintes prioridades:

1. Ignorar espaços, tabulações e quebras de linha, atualizando linha e coluna.
2. Reconhecer operadores compostos antes dos operadores unitários: `//` antes de `/` e `==` antes de qualquer tentativa de tratar `=` isolado.
3. Reconhecer um número decimal completo antes de classificá-lo como número inteiro; o ponto deve ser consumido somente quando fizer parte de `DIGITO+ '.' DIGITO+`.
4. Reconhecer `NÃO` diretamente nessa grafia UTF-8, antes ou em paralelo ao reconhecimento genérico de identificadores ASCII.
5. Reconhecer uma sequência ASCII de letras e dígitos como uma palavra inteira antes de classificá-la.
6. Consultar a tabela de palavras reservadas e tokens nomeados antes de produzir `TOK_ID`. `NAO` não deve ser convertido para `TOK_NAO`.
7. Reconhecer `raiz` como `TOK_RAIZ`, nunca como `TOK_ID`.
8. Se nenhum token se aplicar, produzir `TOK_INVALIDO` e registrar a posição do erro.
9. Produzir `TOK_EOF` somente ao alcançar o fim da entrada.

**Caracteres UTF-8 fora do alfabeto (D37).** Uma palavra que contém letras
ASCII e caracteres UTF-8 multibyte (por exemplo `função`, `número`, `éxito`) é
lida inteira e produz **um** `TOK_ID` marcado com erro léxico: o erro é
relatado uma vez e a análise sintática continua normalmente. Um caractere
multibyte isolado (`€`, emoji) produz um único `TOK_INVALIDO`. Bytes que não
formam UTF-8 válido continuam produzindo um `TOK_INVALIDO` por byte. A coluna
conta caracteres, não bytes.

O princípio geral para os operadores compostos é o maior casamento possível. Em `a // b`, o lexer deve produzir `TOK_DIV_INT`, e não `TOK_DIV` seguido de `TOK_DIV`.

## 6. Conflitos e diferenças entre léxico e sintaxe

### 6.1 `func`

`func` é um token lexical válido porque está na lista oficial de palavras reservadas. A decisão D01 determina que a gramática efetiva reconhece somente `outrafuncao` como início de função não-principal. Portanto, `func` pode ser produzido pelo lexer e ainda assim provocar erro sintático no parser. Validade léxica não implica validade sintática.

### 6.2 `vazio`

`vazio` é sempre reconhecido como `TOK_VAZIO`. A posição em que ele pode aparecer é uma regra sintática: a gramática usa `TIPO_RETORNO` para funções e `TIPO_VALOR` para declarações, parâmetros e campos.

### 6.3 `raiz()`

O operador oficial aparece escrito como `raiz()`, mas o lexer não cria um token composto para a chamada. A sequência é:

```text
TOK_RAIZ TOK_ABRE_PAREN TOK_NUM_INTEIRO TOK_FECHA_PAREN
```

O parser decide se essa sequência é uma aplicação válida de `raiz`.

### 6.4 `=` isolado

`=` aparece no alfabeto oficial, mas não está definido como atribuição nem como comparador isolado. A atribuição é a palavra `receba` e a igualdade é `==`. Por isso, `=` isolado será classificado como `TOK_INVALIDO`; não será criado `TOK_IGUAL`.

### 6.5 `@` e `,`

`@` e `,` são usados pelas estruturas oficiais, embora não apareçam no alfabeto básico. A menor correção adotada pelo projeto foi incluí-los como tokens léxicos, sem alterar as estruturas:

- `@` -> `TOK_ARROBA`;
- `,` -> `TOK_VIRGULA`.

### 6.6 `.` e aspas duplas

`.` e `"` não aparecem no alfabeto básico. Eles foram introduzidos pelas decisões D02/D03 para permitir, respectivamente, números decimais e literais do tipo `palavra`. O ponto não existe isoladamente como token e as aspas delimitam `TOK_TEXTO`.

### 6.7 Chamada isolada de função

`nome(argumentos)` é uma forma lexical composta por `TOK_ID`, `TOK_ABRE_PAREN`, argumentos e `TOK_FECHA_PAREN`. O `;` posterior também é lexicalmente válido. A decisão de não permitir `nome(argumentos);` como comando é sintática, não lexical.

## 7. Tokens especiais em relação à gramática

Todos os terminais literais usados em [`README_gramatica.md`](./README_gramatica.md) possuem classificação nesta especificação:

- nomes e valores: `TOK_ID`, `TOK_NUM_INTEIRO`, `TOK_NUM_DECIMAL`, `TOK_TEXTO`, `TOK_VER`, `TOK_FAL`;
- palavras de tipos e comandos: tokens oficiais e tokens estruturais adicionais;
- operadores: tokens lógicos, aritméticos e comparadores;
- delimitadores: colchetes, chaves, parênteses, ponto e vírgula, vírgula e `@`;
- controle da entrada: `TOK_EOF`.

- `TOK_FUNC`: lexema válido reconhecido pelo lexer, mas sem construção aceita pela gramática efetiva, por decisão D01.

- `TOK_INVALIDO`: token interno de diagnóstico. Não representa um lexema válido da linguagem e nunca deve ser aceito pelo parser.

- `TOK_EOF`: token interno de fim de entrada e corresponde ao `EOF` usado em `PROGRAMA -> ITEM* EOF`.

Isso é esperado e não representa conflito entre as fases.
