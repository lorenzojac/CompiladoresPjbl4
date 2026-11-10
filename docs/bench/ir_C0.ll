; ModuleID = 'build/bench/n1000000/nucleos_C0.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-w64-windows-gnu"

; Function Attrs: nounwind
define void @zl_nucleo_1(ptr %0, ptr %1, ptr %2) #0 {
  %4 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %2, 0
  %5 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %4, ptr %2, 1
  %6 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %5, i64 0, 2
  %7 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %6, i64 1000000, 3, 0
  %8 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %7, i64 1, 4, 0
  %9 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %1, 0
  %10 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %9, ptr %1, 1
  %11 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %10, i64 0, 2
  %12 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %11, i64 1000000, 3, 0
  %13 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %12, i64 1, 4, 0
  %14 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %0, 0
  %15 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %14, ptr %0, 1
  %16 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %15, i64 0, 2
  %17 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %16, i64 1000000, 3, 0
  %18 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %17, i64 1, 4, 0
  br label %19

19:                                               ; preds = %22, %3
  %20 = phi i64 [ %32, %22 ], [ 0, %3 ]
  %21 = icmp slt i64 %20, 1000000
  br i1 %21, label %22, label %33

22:                                               ; preds = %19
  %23 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %18, 1
  %24 = getelementptr inbounds nuw float, ptr %23, i64 %20
  %25 = load float, ptr %24, align 4
  %26 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %13, 1
  %27 = getelementptr inbounds nuw float, ptr %26, i64 %20
  %28 = load float, ptr %27, align 4
  %29 = fadd float %25, %28
  %30 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %8, 1
  %31 = getelementptr inbounds nuw float, ptr %30, i64 %20
  store float %29, ptr %31, align 4
  %32 = add i64 %20, 1
  br label %19

33:                                               ; preds = %19
  ret void
}

; Function Attrs: nounwind
define i32 @zl_nucleo_2(ptr %0) #0 {
  %2 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %0, 0
  %3 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %2, ptr %0, 1
  %4 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %3, i64 0, 2
  %5 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %4, i64 1000000, 3, 0
  %6 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %5, i64 1, 4, 0
  br label %7

7:                                                ; preds = %11, %1
  %8 = phi i64 [ %16, %11 ], [ 0, %1 ]
  %9 = phi i32 [ %15, %11 ], [ 0, %1 ]
  %10 = icmp slt i64 %8, 1000000
  br i1 %10, label %11, label %17

11:                                               ; preds = %7
  %12 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %6, 1
  %13 = getelementptr inbounds nuw i32, ptr %12, i64 %8
  %14 = load i32, ptr %13, align 4
  %15 = add i32 %9, %14
  %16 = add i64 %8, 1
  br label %7

17:                                               ; preds = %7
  ret i32 %9
}

; Function Attrs: nounwind
define float @zl_nucleo_3(ptr %0) #0 {
  %2 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %0, 0
  %3 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %2, ptr %0, 1
  %4 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %3, i64 0, 2
  %5 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %4, i64 1000000, 3, 0
  %6 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %5, i64 1, 4, 0
  br label %7

7:                                                ; preds = %11, %1
  %8 = phi i64 [ %16, %11 ], [ 0, %1 ]
  %9 = phi float [ %15, %11 ], [ 0.000000e+00, %1 ]
  %10 = icmp slt i64 %8, 1000000
  br i1 %10, label %11, label %17

11:                                               ; preds = %7
  %12 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %6, 1
  %13 = getelementptr inbounds nuw float, ptr %12, i64 %8
  %14 = load float, ptr %13, align 4
  %15 = fadd float %9, %14
  %16 = add i64 %8, 1
  br label %7

17:                                               ; preds = %7
  ret float %9
}

; Function Attrs: nounwind
define float @zl_nucleo_4(ptr %0, ptr %1) #0 {
  %3 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %1, 0
  %4 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %3, ptr %1, 1
  %5 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %4, i64 0, 2
  %6 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %5, i64 1000000, 3, 0
  %7 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %6, i64 1, 4, 0
  %8 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %0, 0
  %9 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %8, ptr %0, 1
  %10 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %9, i64 0, 2
  %11 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %10, i64 1000000, 3, 0
  %12 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %11, i64 1, 4, 0
  br label %13

13:                                               ; preds = %17, %2
  %14 = phi i64 [ %26, %17 ], [ 0, %2 ]
  %15 = phi float [ %25, %17 ], [ 0.000000e+00, %2 ]
  %16 = icmp slt i64 %14, 1000000
  br i1 %16, label %17, label %27

17:                                               ; preds = %13
  %18 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %12, 1
  %19 = getelementptr inbounds nuw float, ptr %18, i64 %14
  %20 = load float, ptr %19, align 4
  %21 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %7, 1
  %22 = getelementptr inbounds nuw float, ptr %21, i64 %14
  %23 = load float, ptr %22, align 4
  %24 = fmul float %20, %23
  %25 = fadd float %15, %24
  %26 = add i64 %14, 1
  br label %13

27:                                               ; preds = %13
  ret float %15
}

attributes #0 = { nounwind }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
