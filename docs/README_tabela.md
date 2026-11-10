# Tabela de definição da Macaronica

Este documento separa o que está definido oficialmente no enunciado do que é ambíguo ou ausente. A fonte principal é a Seção 4.2 de `especificacao_macaronica.md` e as lacunas explicitamente listadas na Seção 4.3.

| Status | Item | O que consta na fonte | Tratamento no projeto |
| --- | --- | --- | --- |
| Definido oficialmente | Alfabeto básico | `[ ] { } ( ) a..z A..Z 0..9 + - * / ; ^ < > = !` | Mantido como referência da especificação. |
| Definido oficialmente | Palavras reservadas e tipos | `receba`, `repete`, `durante`, `cond`, `casocontrario`, `func`, `bool`, `inteira`, `flut`, `palavra`, `duplocarpado`, `principal`, `vazio`, `respost` | Mantidos na forma e na grafia da fonte. |
| Definido oficialmente | Tipos | `bool`, `inteira`, `flut`, `palavra`, `duplocarpado` e `vazio`, sendo `vazio` apenas tipo de retorno | Mantidos. |
| Definido oficialmente | Booleanos | `VER` e `FAL` | Reconhecidos exatamente com essa grafia. |
| Definido oficialmente | Comparadores | `<`, `>`, `==` e `!` (diferente) | Reconhecidos exatamente com esses símbolos. |
| Definido oficialmente | Operadores lógicos | `OU`, `E`, `NÃO` e `XOR` | Reconhecidos como operadores de condição. |
| Definido oficialmente | Operadores aritméticos | `+`, `-`, `*`, `/`, `//`, `^` e `raiz()` | Mantidos; a precedência é definida em `README_gramatica.md`. |
| Definido oficialmente | Atribuição e terminador | `receba` e `;` | Mantidos. |
| Definido oficialmente | Regra mínima de condição | `CONDIÇÃO -> VAR COMPARADOR VAR`, `CONDIÇÃO -> VAR COMPARADOR NUM`, `CONDIÇÃO -> NUM COMPARADOR NUM` | Preservada como subconjunto da gramática efetiva. |
| Definido oficialmente | Estruturas | Registro, funções, principal, variáveis, vetores, atribuição, `repete`, `durante`, `cond`, `casocontrario` e `respost` | Estruturas incorporadas à gramática efetiva. |
| Definido oficialmente | Exemplo oficial | Função `quadrado` e função `principal` com atribuição, chamada, condição e retorno | Mantido em `especificacao_macaronica.md`. |
| Ambíguo | Acesso a vetor | `{ }` aparecem no tamanho da declaração e, presumivelmente, no índice de acesso; não há exemplo de acesso | Usar `nome{indice}`. |
| Ambíguo | Chamada de função | A declaração é mostrada, mas a chamada não é especificada | Usar `nome(argumentos)`. |
| Ambíguo | Retorno de função `vazio` | Não se sabe se `respost` deve ser omitido ou escrito como `respost ;` | Omitir `respost` em funções `vazio`; não reconhecer `respost ;`. |
| Ambíguo/conflitante | Uso de `vazio` | A lista de tipos diz que `vazio` é apenas retorno, mas a produção genérica `TIPO` poderia permiti-lo em declarações, parâmetros e campos | Separar `TIPO_VALOR` de `TIPO_RETORNO`. |
| Ambíguo/conflitante | Vocabulário estrutural | `registro`, `outrafuncao`, `vet` e `edai` aparecem nas estruturas, mas não na lista explícita de palavras reservadas | Tratá-los como palavras reservadas estruturais. |
| Ambíguo/conflitante | Função não-principal | `func` aparece na lista de reservadas, mas a estrutura usa `outrafuncao` | Usar apenas `outrafuncao` na produção de função; `func` permanece reservado, mas sem construção sintática. |
| Ambíguo/conflitante | Símbolos usados nas estruturas | `@` e `,` aparecem nas estruturas, mas não aparecem no alfabeto básico | Incluí-los no conjunto efetivo de tokens do parser. |
| Ambíguo/conflitante | Token `NÃO` | O token oficial contém `Ã`, que não aparece no alfabeto simplificado | Aceitar a grafia oficial `NÃO` em UTF-8; não criar a grafia alternativa `NAO`. |
| Ambíguo | Operadores lógicos e comparações | São listados na tabela de tokens, mas não aparecem nas três produções mínimas de condição nem há regra para produzir um valor `bool` | Permitir expressões booleanas gerais; a semântica exigirá tipo `bool` em condições. |
| Ambíguo | `raiz()` | O token é listado, mas não é apresentada sua forma dentro de uma expressão | Reconhecer `raiz(expressao)` como operador unário e reservar `raiz`. |
| Ausente | Chamada de função como comando | A fonte não diz se `nome(argumentos);` pode aparecer sozinho | Não permitir chamada isolada; chamadas só aparecem dentro de expressões. |
| Ausente | Regra lexical de identificadores | A fonte usa `VAR`, mas não define o formato de um identificador | Usar uma letra seguida de zero ou mais letras ou dígitos; sem `_`. |
| Ausente | Regra lexical de números | A fonte usa `NUM`, mas não define inteiros ou decimais | Usar inteiros e decimais com ponto: `DIGITO+` ou `DIGITO+ '.' DIGITO+`. |
| Ausente | Literais de `palavra` | O tipo existe, mas não há sintaxe de literal textual | Usar texto entre aspas duplas, sem quebra de linha. |
| Ausente | Gramática de expressões | Não há produções para valores, chamadas, precedência ou associatividade | Usar a gramática de expressões de `README_gramatica.md`. |
| Ausente | Sintaxe de inicialização e iteração do `repete` | A estrutura usa `var` e `iteração`, sem definir sua forma | Usar atribuições sem `;`, separadas por vírgulas no cabeçalho. |
| Ausente | Acesso a campos de registro | A declaração do registro existe, mas o acesso a campos não é descrito | Não reconhecer acesso a campos nesta versão. |
| Ausente | Comentários e regras de espaço em branco | Não há definição | Ignorar espaços, tabulações e quebras de linha; comentários não fazem parte da linguagem. |
| Ausente | Regras semânticas de tipos | A fonte lista tipos, mas não formaliza conversões e compatibilidades | Deixar essas verificações para o analisador semântico; não alteram a gramática. |
| Ambíguo | Declarações globais | `PROGRAMA -> ITEM*` não é definido pela fonte, mas declarações de variável são definidas | Permitir declarações globais no nível de programa. |
| Ausente na fonte / extensão do projeto | Entrada e saída | A especificação original não define sintaxe de I/O, embora o requisito de entrega peça cobertura | D31 resolve a lacuna com `le(LVALUE);` e `imprime(EXPRESSAO);` como statements próprios, sem runtime. |

As decisões concretas correspondentes às linhas ambíguas e ausentes estão em [`decisoes.md`](./decisoes.md). A gramática resultante está em [`README_gramatica.md`](./README_gramatica.md).
