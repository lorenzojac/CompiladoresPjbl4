; ModuleID = 'somapares.mac'
; Gerado pelo compilador Macaronica (PjBL 3).
; IR portavel: sem target triple nem datalayout; o alvo e
; escolhido na construcao (clang --target / llc -mtriple).
; Erros de execucao: 2 indice, 3 divisao por zero,
; 4 raiz de negativo, 5 expoente negativo (inteira).
source_filename = "somapares.mac"

; Biblioteca de execucao (runtime/zl_runtime.h)
declare void @zl_imprime_inteira(i32) nounwind
declare void @zl_imprime_nl() nounwind

; linha 1: outrafuncao somapares
define internal i32 @somapares(i32 %n) nounwind {
entry:
  %n.addr = alloca i32
  %i = alloca i32
  %soma = alloca i32
  store i32 %n, ptr %n.addr
  ; linha 2
  store i32 0, ptr %i
  store i32 0, ptr %soma
  ; linha 4
  store i32 0, ptr %soma
  ; linha 5
  store i32 0, ptr %i
  br label %repete_teste_0
repete_teste_0:
  %0 = load i32, ptr %i
  %1 = load i32, ptr %n.addr
  %2 = icmp slt i32 %0, %1
  br i1 %2, label %repete_corpo_0, label %repete_fim_0
repete_corpo_0:
  ; linha 6
  %3 = load i32, ptr %i
  %4 = sdiv i32 %3, 2
  %5 = mul i32 %4, 2
  %6 = load i32, ptr %i
  %7 = icmp eq i32 %5, %6
  br i1 %7, label %entao_1_0, label %cond_fim_1
entao_1_0:
  ; linha 7
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
  ; linha 10
  %13 = load i32, ptr %soma
  ret i32 %13
}

; linha 13: principal
define i32 @zl_principal() nounwind {
entry:
  ; linha 14
  %0 = call i32 @somapares(i32 10)
  call void @zl_imprime_inteira(i32 %0)
  call void @zl_imprime_nl()
  ; linha 15
  ret i32 0
}

