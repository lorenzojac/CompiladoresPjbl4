; ModuleID = 'docs/ir/somapares_entrada.ll'
source_filename = "somapares_entrada.mac"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nounwind
declare void @zl_imprime_inteira(i32) local_unnamed_addr #0

; Function Attrs: nounwind
declare void @zl_imprime_nl() local_unnamed_addr #0

; Function Attrs: nounwind
declare i32 @zl_le_inteira(i32) local_unnamed_addr #0

; Function Attrs: nounwind
define noundef i32 @zl_principal() local_unnamed_addr #0 {
entry:
  %0 = tail call i32 @zl_le_inteira(i32 13)
  %1 = icmp sgt i32 %0, 0
  br i1 %1, label %iter.check, label %somapares.exit

iter.check:                                       ; preds = %entry
  %min.iters.check = icmp ult i32 %0, 8
  br i1 %min.iters.check, label %repete_corpo_0.i.preheader, label %vector.main.loop.iter.check

vector.main.loop.iter.check:                      ; preds = %iter.check
  %min.iters.check1 = icmp ult i32 %0, 32
  br i1 %min.iters.check1, label %vec.epilog.ph, label %vector.ph

vector.ph:                                        ; preds = %vector.main.loop.iter.check
  %n.mod.vf = and i32 %0, 24
  %n.vec = and i32 %0, 2147483616
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i32 [ 0, %vector.ph ], [ %index.next, %vector.body ]
  %vec.phi = phi <8 x i32> [ zeroinitializer, %vector.ph ], [ %14, %vector.body ]
  %vec.phi2 = phi <8 x i32> [ zeroinitializer, %vector.ph ], [ %15, %vector.body ]
  %vec.phi3 = phi <8 x i32> [ zeroinitializer, %vector.ph ], [ %16, %vector.body ]
  %vec.phi4 = phi <8 x i32> [ zeroinitializer, %vector.ph ], [ %17, %vector.body ]
  %vec.ind = phi <8 x i32> [ <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>, %vector.ph ], [ %vec.ind.next, %vector.body ]
  %step.add = add nuw <8 x i32> %vec.ind, splat (i32 8)
  %step.add.2 = add nuw <8 x i32> %vec.ind, splat (i32 16)
  %step.add.3 = add nuw <8 x i32> %vec.ind, splat (i32 24)
  %2 = and <8 x i32> %vec.ind, splat (i32 1)
  %3 = and <8 x i32> %vec.ind, splat (i32 1)
  %4 = and <8 x i32> %vec.ind, splat (i32 1)
  %5 = and <8 x i32> %vec.ind, splat (i32 1)
  %6 = icmp eq <8 x i32> %2, zeroinitializer
  %7 = icmp eq <8 x i32> %3, zeroinitializer
  %8 = icmp eq <8 x i32> %4, zeroinitializer
  %9 = icmp eq <8 x i32> %5, zeroinitializer
  %10 = select <8 x i1> %6, <8 x i32> %vec.ind, <8 x i32> zeroinitializer
  %11 = select <8 x i1> %7, <8 x i32> %step.add, <8 x i32> zeroinitializer
  %12 = select <8 x i1> %8, <8 x i32> %step.add.2, <8 x i32> zeroinitializer
  %13 = select <8 x i1> %9, <8 x i32> %step.add.3, <8 x i32> zeroinitializer
  %14 = add <8 x i32> %10, %vec.phi
  %15 = add <8 x i32> %11, %vec.phi2
  %16 = add <8 x i32> %12, %vec.phi3
  %17 = add <8 x i32> %13, %vec.phi4
  %index.next = add nuw i32 %index, 32
  %vec.ind.next = add nuw <8 x i32> %vec.ind, splat (i32 32)
  %18 = icmp eq i32 %index.next, %n.vec
  br i1 %18, label %middle.block, label %vector.body, !llvm.loop !0

middle.block:                                     ; preds = %vector.body
  %bin.rdx = add <8 x i32> %15, %14
  %bin.rdx5 = add <8 x i32> %16, %bin.rdx
  %bin.rdx6 = add <8 x i32> %17, %bin.rdx5
  %19 = tail call i32 @llvm.vector.reduce.add.v8i32(<8 x i32> %bin.rdx6)
  %cmp.n = icmp eq i32 %0, %n.vec
  br i1 %cmp.n, label %somapares.exit, label %vec.epilog.iter.check

vec.epilog.iter.check:                            ; preds = %middle.block
  %min.epilog.iters.check = icmp eq i32 %n.mod.vf, 0
  br i1 %min.epilog.iters.check, label %repete_corpo_0.i.preheader, label %vec.epilog.ph, !prof !3

vec.epilog.ph:                                    ; preds = %vector.main.loop.iter.check, %vec.epilog.iter.check
  %vec.epilog.resume.val = phi i32 [ %n.vec, %vec.epilog.iter.check ], [ 0, %vector.main.loop.iter.check ]
  %bc.merge.rdx = phi i32 [ %19, %vec.epilog.iter.check ], [ 0, %vector.main.loop.iter.check ]
  %n.vec8 = and i32 %0, 2147483640
  %20 = insertelement <8 x i32> <i32 poison, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0>, i32 %bc.merge.rdx, i64 0
  %broadcast.splatinsert = insertelement <8 x i32> poison, i32 %vec.epilog.resume.val, i64 0
  %broadcast.splat = shufflevector <8 x i32> %broadcast.splatinsert, <8 x i32> poison, <8 x i32> zeroinitializer
  %induction = or disjoint <8 x i32> %broadcast.splat, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  br label %vec.epilog.vector.body

vec.epilog.vector.body:                           ; preds = %vec.epilog.vector.body, %vec.epilog.ph
  %index9 = phi i32 [ %vec.epilog.resume.val, %vec.epilog.ph ], [ %index.next12, %vec.epilog.vector.body ]
  %vec.phi10 = phi <8 x i32> [ %20, %vec.epilog.ph ], [ %24, %vec.epilog.vector.body ]
  %vec.ind11 = phi <8 x i32> [ %induction, %vec.epilog.ph ], [ %vec.ind.next13, %vec.epilog.vector.body ]
  %21 = and <8 x i32> %vec.ind11, splat (i32 1)
  %22 = icmp eq <8 x i32> %21, zeroinitializer
  %23 = select <8 x i1> %22, <8 x i32> %vec.ind11, <8 x i32> zeroinitializer
  %24 = add <8 x i32> %23, %vec.phi10
  %index.next12 = add nuw i32 %index9, 8
  %vec.ind.next13 = add nuw nsw <8 x i32> %vec.ind11, splat (i32 8)
  %25 = icmp eq i32 %index.next12, %n.vec8
  br i1 %25, label %vec.epilog.middle.block, label %vec.epilog.vector.body, !llvm.loop !4

vec.epilog.middle.block:                          ; preds = %vec.epilog.vector.body
  %26 = tail call i32 @llvm.vector.reduce.add.v8i32(<8 x i32> %24)
  %cmp.n14 = icmp eq i32 %0, %n.vec8
  br i1 %cmp.n14, label %somapares.exit, label %repete_corpo_0.i.preheader

repete_corpo_0.i.preheader:                       ; preds = %iter.check, %vec.epilog.iter.check, %vec.epilog.middle.block
  %soma.07.i.ph = phi i32 [ 0, %iter.check ], [ %19, %vec.epilog.iter.check ], [ %26, %vec.epilog.middle.block ]
  %i.06.i.ph = phi i32 [ 0, %iter.check ], [ %n.vec, %vec.epilog.iter.check ], [ %n.vec8, %vec.epilog.middle.block ]
  br label %repete_corpo_0.i

repete_corpo_0.i:                                 ; preds = %repete_corpo_0.i.preheader, %repete_corpo_0.i
  %soma.07.i = phi i32 [ %spec.select.i, %repete_corpo_0.i ], [ %soma.07.i.ph, %repete_corpo_0.i.preheader ]
  %i.06.i = phi i32 [ %30, %repete_corpo_0.i ], [ %i.06.i.ph, %repete_corpo_0.i.preheader ]
  %27 = and i32 %i.06.i, 1
  %28 = icmp eq i32 %27, 0
  %29 = select i1 %28, i32 %i.06.i, i32 0
  %spec.select.i = add i32 %29, %soma.07.i
  %30 = add nuw nsw i32 %i.06.i, 1
  %exitcond.not.i = icmp eq i32 %30, %0
  br i1 %exitcond.not.i, label %somapares.exit, label %repete_corpo_0.i, !llvm.loop !5

somapares.exit:                                   ; preds = %repete_corpo_0.i, %middle.block, %vec.epilog.middle.block, %entry
  %soma.0.lcssa.i = phi i32 [ 0, %entry ], [ %26, %vec.epilog.middle.block ], [ %19, %middle.block ], [ %spec.select.i, %repete_corpo_0.i ]
  tail call void @zl_imprime_inteira(i32 %soma.0.lcssa.i)
  tail call void @zl_imprime_nl()
  ret i32 0
}

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.vector.reduce.add.v8i32(<8 x i32>) #1

attributes #0 = { nounwind "target-cpu"="alderlake" }
attributes #1 = { nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none) }

!0 = distinct !{!0, !1, !2}
!1 = !{!"llvm.loop.isvectorized", i32 1}
!2 = !{!"llvm.loop.unroll.runtime.disable"}
!3 = !{!"branch_weights", i32 8, i32 24}
!4 = distinct !{!4, !1, !2}
!5 = distinct !{!5, !2, !1}
