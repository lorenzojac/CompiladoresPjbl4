# Programas de teste do professor (defesa do PjBL 1/2)

Os 5 programas `nivel*.mcn` foram os usados na defesa do PjBL 1/2, copiados
sem alteração. Eles não imprimem nada: o resultado é o valor de `respost` da
`principal`, que vira o código de saída do processo (D42).

| Programa | `principal` devolve | Código de saída esperado | Conferência independente |
| --- | --- | --- | --- |
| `nivel1_soma.mcn` | `7 + 15` | 22 | à mão |
| `nivel2_primos_fatorial.mcn` | `fatorial(5)` | 120 | à mão |
| `nivel3_zoologico_ordenacao.mcn` | média inteira dos 6 pesos = 1203 // 6 | 200 | soma dos pesos do fonte com `awk` |
| `nivel4_zoologico_completo.mcn` | média inteira dos 12 pesos = 3632 // 12 = 302 | **46** (302 mod 256: o código de saída tem 8 bits) | idem |
| `nivel5_zoologico_avancado.mcn` | média inteira dos 18 pesos = 3956 // 18 | 219 | idem |

Rodar: `sh tests/run_execucao.sh tests/professor` (parte do `make test`, com
`clang` e `lli`) e, com a placa, `python3 tests/run_pico.py tests/professor`
(parte do `make test-pico`).

Estes programas mostraram duas diferenças entre o compilador e o que o
professor escreve, ambas corrigidas:

- **D51**: `Cuidador responsavel;` declara uma variável de registro **sem** a
  palavra `registro` (o `nivel3` era rejeitado já no PjBL 1). As duas formas
  são aceitas.
- **D50**: CPFs de 11 dígitos em `inteira` (`12345678901`) não cabem em i32.
  O PjBL 1 os aceitava e a D38 passou a recusar. Agora o compilador **aceita
  com aviso** e reduz o valor módulo 2³² (complemento de dois), a mesma
  política do estouro aritmético (D43).
