; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"

; Function Attrs: nounwind
define void @zl_nucleo_1(ptr %0, ptr %1, ptr %2) #0 {
  %4 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %2, 0
  %5 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %4, ptr %2, 1
  %6 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %5, i64 0, 2
  %7 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %6, i64 1024, 3, 0
  %8 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %7, i64 1, 4, 0
  %9 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %1, 0
  %10 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %9, ptr %1, 1
  %11 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %10, i64 0, 2
  %12 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %11, i64 1024, 3, 0
  %13 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %12, i64 1, 4, 0
  %14 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %0, 0
  %15 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %14, ptr %0, 1
  %16 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %15, i64 0, 2
  %17 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %16, i64 1024, 3, 0
  %18 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %17, i64 1, 4, 0
  br label %19

19:                                               ; preds = %22, %3
  %20 = phi i64 [ %32, %22 ], [ 0, %3 ]
  %21 = icmp slt i64 %20, 1024
  br i1 %21, label %22, label %33

22:                                               ; preds = %19
  %23 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %18, 1
  %24 = getelementptr float, ptr %23, i64 %20
  %25 = load <8 x float>, ptr %24, align 4
  %26 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %13, 1
  %27 = getelementptr float, ptr %26, i64 %20
  %28 = load <8 x float>, ptr %27, align 4
  %29 = fadd <8 x float> %25, %28
  %30 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %8, 1
  %31 = getelementptr float, ptr %30, i64 %20
  store <8 x float> %29, ptr %31, align 4
  %32 = add i64 %20, 8
  br label %19

33:                                               ; preds = %19
  ret void
}

; Function Attrs: nounwind
define float @zl_nucleo_2(ptr %0) #0 {
  %2 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %0, 0
  %3 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %2, ptr %0, 1
  %4 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %3, i64 0, 2
  %5 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %4, i64 1024, 3, 0
  %6 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %5, i64 1, 4, 0
  br label %7

7:                                                ; preds = %11, %1
  %8 = phi i64 [ %16, %11 ], [ 0, %1 ]
  %9 = phi <8 x float> [ %15, %11 ], [ zeroinitializer, %1 ]
  %10 = icmp slt i64 %8, 1024
  br i1 %10, label %11, label %17

11:                                               ; preds = %7
  %12 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %6, 1
  %13 = getelementptr float, ptr %12, i64 %8
  %14 = load <8 x float>, ptr %13, align 4
  %15 = fadd <8 x float> %9, %14
  %16 = add i64 %8, 8
  br label %7

17:                                               ; preds = %7
  %18 = call float @llvm.vector.reduce.fadd.v8f32(float 0.000000e+00, <8 x float> %9)
  ret float %18
}

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.vector.reduce.fadd.v8f32(float, <8 x float>) #1

attributes #0 = { nounwind }
attributes #1 = { nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
