; ModuleID = 'docs/ir/somapares.ll'
source_filename = "somapares.mac"

; Function Attrs: nounwind
declare void @zl_imprime_inteira(i32) #0

; Function Attrs: nounwind
declare void @zl_imprime_nl() #0

; Function Attrs: nounwind
define internal i32 @somapares(i32 %n) #0 {
entry:
  br label %repete_teste_0

repete_teste_0:                                   ; preds = %repete_passo_0, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %5, %repete_passo_0 ]
  %soma.0 = phi i32 [ 0, %entry ], [ %soma.1, %repete_passo_0 ]
  %0 = icmp slt i32 %i.0, %n
  br i1 %0, label %repete_corpo_0, label %repete_fim_0

repete_corpo_0:                                   ; preds = %repete_teste_0
  %1 = sdiv i32 %i.0, 2
  %2 = mul i32 %1, 2
  %3 = icmp eq i32 %2, %i.0
  br i1 %3, label %entao_1_0, label %cond_fim_1

entao_1_0:                                        ; preds = %repete_corpo_0
  %4 = add i32 %soma.0, %i.0
  br label %cond_fim_1

cond_fim_1:                                       ; preds = %entao_1_0, %repete_corpo_0
  %soma.1 = phi i32 [ %4, %entao_1_0 ], [ %soma.0, %repete_corpo_0 ]
  br label %repete_passo_0

repete_passo_0:                                   ; preds = %cond_fim_1
  %5 = add i32 %i.0, 1
  br label %repete_teste_0

repete_fim_0:                                     ; preds = %repete_teste_0
  ret i32 %soma.0
}

; Function Attrs: nounwind
define i32 @zl_principal() #0 {
entry:
  %0 = call i32 @somapares(i32 10)
  call void @zl_imprime_inteira(i32 %0)
  call void @zl_imprime_nl()
  ret i32 0
}

attributes #0 = { nounwind }
