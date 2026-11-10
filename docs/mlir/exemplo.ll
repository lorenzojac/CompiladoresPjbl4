; ModuleID = 'exemplo.mac'
; Gerado pelo compilador Macaronica (PjBL 3).
; IR portavel: sem target triple nem datalayout; o alvo e
; escolhido na construcao (clang --target / llc -mtriple).
; Erros de execucao: 2 indice, 3 divisao por zero,
; 4 raiz de negativo, 5 expoente negativo (inteira).
source_filename = "exemplo.mac"

@a = internal global [1024 x float] zeroinitializer  ; linha 1
@b = internal global [1024 x float] zeroinitializer  ; linha 1
@c = internal global [1024 x float] zeroinitializer  ; linha 1

; Biblioteca de execucao (runtime/zl_runtime.h)
declare void @zl_imprime_flut(float) nounwind
declare void @zl_imprime_nl() nounwind
; Nucleos da DSL vetorial (gerados em MLIR)
declare void @zl_nucleo_1(ptr, ptr, ptr) nounwind
declare float @zl_nucleo_2(ptr) nounwind

; linha 3: principal
define i32 @zl_principal() nounwind {
entry:
  ; linha 4
  ; nucleo MLIR: zl_nucleo_1
  call void @zl_nucleo_1(ptr @a, ptr @b, ptr @c)
  ; linha 5
  ; nucleo MLIR: zl_nucleo_2
  %0 = call float @zl_nucleo_2(ptr @c)
  call void @zl_imprime_flut(float %0)
  call void @zl_imprime_nl()
  ; linha 6
  ret i32 0
}

