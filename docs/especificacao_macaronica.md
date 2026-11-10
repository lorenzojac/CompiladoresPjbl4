# Especificação da linguagem Macaronica

Transcrição organizada da especificação oficial da Macaronica, conforme as Seções 4.2 e 4.3 do enunciado do trabalho prático da disciplina. Este documento registra as definições da fonte e não resolve as lacunas listadas na Seção 9.

## 1. Alfabeto

```
[ ] { } ( ) a..z A..Z 0..9 + - * / ; ^ < > = !
```

## 2. Palavras reservadas

```
receba, repete, durante, cond, casocontrario, func, bool, inteira,
flut, palavra, duplocarpado, principal, vazio, respost
```

## 3. Tipos

Os tipos de dado disponíveis são:

- `bool`
- `inteira` (`int`)
- `flut` (`float`)
- `palavra` (`string`)
- `duplocarpado` (`double`)
- `vazio` (`void`), apenas como tipo de retorno

## 4. Tokens

| Categoria | Símbolos / definição |
| --- | --- |
| Booleanos | `VER` (verdadeiro) · `FAL` (falso) |
| Comparadores | `<` · `>` · `==` · `!` (diferente) |
| Operadores lógicos | `OU` · `E` · `NÃO` · `XOR` |
| Operadores aritméticos (SINAL) | `+` · `-` · `*` · `/` · `//` (divisão inteira) · `^` (potência) · `raiz()` (raiz quadrada) |
| Atribuição | `receba` |
| Terminador de linha | `;` |

## 5. Operadores

### Comparadores

```text
<   >   ==   ! (diferente)
```

### Operadores lógicos

```text
OU   E   NÃO   XOR
```

### Operadores aritméticos

```text
+   -   *   /   // (divisão inteira)   ^ (potência)   raiz() (raiz quadrada)
```

### Atribuição

```text
receba
```

## 6. Regra de condição

A regra de condição é definida explicitamente na especificação original:

```text
CONDIÇÃO -> VAR COMPARADOR VAR
CONDIÇÃO -> VAR COMPARADOR NUM
CONDIÇÃO -> NUM COMPARADOR NUM
```

## 7. Estruturas da linguagem

- Registro (struct): `registro nome @ [ tipo nome; ... ]`
- Função (não-principal): `outrafuncao tipo nome(tipo nome, ...) @ [ ... respost valor; ]`
- Função principal: `tipo principal(tipo nome, ...) @ [ ... ]`
- Declaração de variável: `tipo nome, ... ;`
- Declaração de vetor: `tipo vet{tamanho} nome, ... ;`
- Atribuição: `var receba valor;`
- Repetição (for): `repete ( var , condição , iteração ) edai [ ... ]`
- Repetição (while): `durante ( condição ) edai [ ... ]`
- Decisão: `cond ( condição ) edai [ ... ] casocontrario cond ( condição ) edai [ ... ]`
- Alternativa da decisão: `casocontrario edai [ ... ]`
- Retorno: `respost valor ;`

## 8. Exemplo oficial

```text
outrafuncao inteira quadrado(inteira n) @
[
    respost n * n ;
]

inteira principal() @
[
    inteira numero;
    inteira resultado;

    numero receba 5;
    resultado receba quadrado(numero);

    cond ( resultado > 20 ) edai
    [
        respost 1;
    ]
    casocontrario edai
    [
        respost 0;
    ]
]
```

## 9. Ambiguidades/lacunas oficiais da Seção 4.3

As lacunas abaixo são as explicitamente listadas para a Macaronica na Seção 4.3. A especificação determina que cada grupo deve tomar e documentar uma decisão de projeto para elas.

1. As chaves `{ }` são usadas tanto para o tamanho de um vetor na declaração (`tipo vet{tamanho} nome`) quanto, presumivelmente, para o índice no acesso a um elemento do vetor. A especificação original não mostra um exemplo de acesso; o grupo deve definir e documentar essa sintaxe.

2. A especificação não define explicitamente a sintaxe de chamada de função (apenas a declaração). O uso natural `nome(argumentos)` deve ser assumido, mas isso deve constar na documentação do grupo.

3. Para uma função do tipo vazio (`void`), não fica claro se o comando `respost` deve ser omitido ou usado sem valor (`respost ;`). O grupo deve escolher e documentar uma convenção.
