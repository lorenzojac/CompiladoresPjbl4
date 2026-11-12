# Extensão vetorial da Macaronica (PjBL 4, Parte A)

A extensão dá operações de primeira classe aos vetores que a linguagem já
tinha (`tipo vet{N} nome`), no mesmo estilo da linguagem: a atribuição
continua sendo `receba` e as operações usam os operadores aritméticos
comuns. Cada operação vetorial é compilada para um núcleo MLIR (Parte B, ver
[`relatorio_otimizacao.md`](./relatorio_otimizacao.md)).

```text
inteira vet{1024} a, b, c;
flut vet{1024} f;

c receba a + b;          // elemento a elemento
c receba 3 * a - b;      // escalar x vetor, expressao fundida
f receba a * 0.5;        // promocao inteira -> flut
imprime(somavet(c));     // reducoes
imprime(maxvet(a));
imprime(prodesc(a, b));  // produto escalar
```

## 1. Léxico

A extensão **não cria tokens novos**: usa `+`, `-`, `*`, `(`, `)`, `,`,
identificadores e literais já existentes. Os nomes das reduções são **funções
embutidas registradas na tabela de símbolos** (escopo global), e não palavras
reservadas:

| Nome | Aridade | Categoria |
| --- | ---: | --- |
| `somavet` | 1 | redução: soma |
| `maxvet` | 1 | redução: máximo |
| `minvet` | 1 | redução: mínimo (extra) |
| `prodesc` | 2 | produto escalar |

Como compartilham o namespace global (D25), o programa não pode declarar
outra função, variável ou registro com esses nomes
(`SEM_DUPLICATE_DECLARATION`). Isso mantém válido todo programa do PjBL 1 que
não use esses quatro nomes.

## 2. Gramática

As produções abaixo são um **refinamento tipado** da gramática de expressões
(`docs/README_gramatica.md`): o parser não muda; é a análise semântica que
classifica cada subexpressão como escalar ou vetorial. `NOME_VETOR` é um
identificador declarado com `vet{N}` e usado **sem índice**; `ESCALAR` é
qualquer expressão escalar numérica da linguagem.

```ebnf
ATRIBUICAO_VETORIAL -> NOME_VETOR 'receba' (EXPR_VET | ESCALAR) ';'

EXPR_VET            -> TERMO_VET (('+' | '-') TERMO_VET)*
TERMO_VET           -> FATOR_VET ('*' FATOR_VET)*
FATOR_VET           -> NOME_VETOR
                     | ESCALAR
                     | '(' EXPR_VET ')'
                     | ('+' | '-') FATOR_VET

REDUCAO             -> ('somavet' | 'maxvet' | 'minvet') '(' EXPR_VET ')'
                     | 'prodesc' '(' EXPR_VET ',' EXPR_VET ')'
```

Restrição: uma `EXPR_VET` contém ao menos um `NOME_VETOR`. `REDUCAO` é uma
expressão **escalar** e pode aparecer em qualquer lugar que aceite um valor
(`imprime(somavet(a) + 1)`). As precedências são as da D08.

Formas que a extensão aceita:

| Operação | Exemplo | Semântica (para todo i em [0, N)) |
| --- | --- | --- |
| elemento a elemento | `c receba a + b;` `a - b`, `a * b` | `c{i} = a{i} op b{i}` |
| escalar × vetor | `c receba 3 * a;` `a * k`, `k + a`, `a - 1` | `c{i} = k op a{i}` (k avaliado uma vez) |
| expressão fundida (extra) | `c receba a + k * b - c;` | uma única passada, sem vetores temporários |
| negação (extra) | `c receba -a;` | `c{i} = -a{i}` |
| preenchimento (extra) | `c receba 0;` | `c{i} = 0` |
| cópia | `c receba a;` | `c{i} = a{i}` |
| redução | `somavet(a)`, `maxvet(a)`, `minvet(a)` | Σ a{i}, max a{i}, min a{i} |
| redução de expressão (extra) | `somavet(a * b + c)` | Σ (a{i}·b{i} + c{i}) |
| produto escalar | `prodesc(a, b)` | Σ a{i}·b{i} |

## 3. Semântica e tipos

1. **Elementos numéricos.** Operações vetoriais aceitam vetores de
   `inteira`, `flut` e `duplocarpado`. Vetor de `bool`, `palavra` ou registro
   em operação vetorial é `SEM_INVALID_VECTOR_OPERATION`; como argumento de
   redução, `SEM_ARGUMENT_TYPE_MISMATCH`.
2. **Tamanhos.** Dois vetores na mesma operação (inclusive o destino e os dois
   argumentos de `prodesc`) devem ter o mesmo N; caso contrário,
   `SEM_VECTOR_SIZE_MISMATCH`.
3. **Promoção (D16/D17, como no PjBL 3).** O tipo do elemento do resultado é o
   mais largo dos operandos (`inteira < flut < duplocarpado`), e a conversão é
   feita elemento a elemento (`arith.sitofp`, `arith.extf` no núcleo). A
   atribuição ao destino segue a D16: `flut vet ← inteira vet` vale;
   `inteira vet ← flut vet` é `SEM_INVALID_ASSIGNMENT`. Um literal decimal
   promovido a `duplocarpado` é convertido a partir do texto (D38).
4. **Operadores.** Só `+`, `-` (binário e unário) e `*`. `/`, `//`, `^`,
   comparações e operadores lógicos com operando vetor são
   `SEM_INVALID_VECTOR_OPERATION`.
5. **Tipo das reduções.** `somavet`/`maxvet`/`minvet` devolvem o tipo do
   elemento da expressão; `prodesc` devolve o tipo mais largo dos dois
   argumentos.
6. **Contexto.** Vetor sem índice fora de uma operação vetorial continua erro
   (`SEM_NAME_KIND_MISMATCH`, como no PjBL 1); expressão vetorial em contexto
   escalar (`imprime(a + b)`) é `SEM_INVALID_VECTOR_OPERATION`.
7. **Compatibilidade.** Nenhum programa válido do PjBL 1 muda de significado: as
   formas acima eram todas erros semânticos antes (vetor sem índice). Os
   testes do PjBL 1 continuam passando (`make test`).

Testes: `tests/dsl/` (7 programas, cada um comparando a operação vetorial
com a mesma conta escrita à mão com laço escalar, executados em C0, C2 e C3
por `lli`, `clang` e pelo JIT do MLIR) e `tests/dsl/invalidos/`
(5 programas rejeitados, cada um com o código esperado em `.erro`).

## 4. Lacunas da Seção 5.5 e decisões

| Lacuna | Decisão |
| --- | --- |
| Vetores cujo tamanho só é conhecido em tempo de execução | **Não existem** na linguagem: o tamanho de um vetor é sempre um literal (D05/D28), e funções não recebem vetores. Todo tamanho é conhecido na compilação, então toda incompatibilidade é erro semântico e nenhuma verificação em tempo de execução é necessária |
| O destino pode ser um operando (`a receba a + b;`)? | **Pode.** Cada iteração lê `a{i}` e `b{i}` e escreve `a{i}`, sempre no mesmo índice: não há dependência entre iterações, então vetorizar e paralelizar continuam corretos. No núcleo MLIR o destino e o operando são o mesmo parâmetro `memref` (a folha é deduplicada), e não dois ponteiros que poderiam se sobrepor |
| Redução de vetor vazio ou de tamanho 1 | Vetor vazio não existe (`vet{0}` é `SEM_INVALID_VECTOR_SIZE`, D28). Com N = 1: `somavet` e `maxvet`/`minvet` devolvem o único elemento, e `prodesc` devolve `a{0} * b{0}`. `maxvet`/`minvet` começam do elemento 0 e o laço vai de 1 a N, que com N = 1 não executa nenhuma vez |
| Paridade (Zolang) | Não se aplica à Macaronica |
| Maior vetor na placa | Limitado pela RAM. Na **Pico 2 W** (placa principal, D48): globais + textos + o maior quadro de função ≤ 524 288 − 65 536 bytes, ou seja, até ~114 688 `inteira`/`flut` (`--alvo=pico`; o programa com 120 000 é rejeitado). No **Uno** (2ª arquitetura): ≤ 2048 − 512 bytes (D45), até ~380 `inteira`/`flut` ou ~190 `duplocarpado` (`--alvo=uno`) |
| Numeração dos pinos | A da própria placa: na Pico, GP0–GP22 e GP26–GP28, e 25 = LED embutido (`pinomodo(25, 1)`); no Uno, 2–19 (13 = LED). Ver [`placa.md`](./placa.md) |
| Núcleos vetoriais num alvo sem SIMD | O mesmo `.ll` serve: no AVR o `llc` "legaliza" os tipos `<8 x float>` da configuração C2 em operações escalares, e o resultado é igual. Não há ganho de vetorização no Uno; o ganho vem só no PC |

## 5. Relação com as decisões do projeto

- D41: DSL vetorial (esta especificação).
- D46: `tipo vet{N} a, b, c;` declara três vetores de N, o que torna natural
  declarar os operandos de uma operação vetorial.
- Funções de E/S física (`pinomodo`, `pinoescreve`, `pinole`, `espera`) são
  registradas na tabela de símbolos do mesmo jeito que as reduções (D44).
