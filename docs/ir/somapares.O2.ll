; ModuleID = 'docs/ir/somapares.ll'
source_filename = "somapares.mac"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nounwind
declare void @zl_imprime_inteira(i32) local_unnamed_addr #0

; Function Attrs: nounwind
declare void @zl_imprime_nl() local_unnamed_addr #0

; Function Attrs: nounwind
define noundef i32 @zl_principal() local_unnamed_addr #0 {
entry:
  tail call void @zl_imprime_inteira(i32 20)
  tail call void @zl_imprime_nl()
  ret i32 0
}

attributes #0 = { nounwind }
