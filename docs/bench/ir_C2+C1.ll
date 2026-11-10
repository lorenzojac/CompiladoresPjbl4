; ModuleID = 'build/bench/n1000000/nucleos_C2+C1.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-w64-windows-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite)
define void @zl_nucleo_1(ptr nofree readonly captures(none) %0, ptr nofree readonly captures(none) %1, ptr nofree writeonly captures(none) %2) local_unnamed_addr #0 {
  br label %4

4:                                                ; preds = %4, %3
  %5 = phi i64 [ 0, %3 ], [ %75, %4 ]
  %6 = getelementptr [4 x i8], ptr %0, i64 %5
  %7 = load <8 x float>, ptr %6, align 4
  %8 = getelementptr [4 x i8], ptr %1, i64 %5
  %9 = load <8 x float>, ptr %8, align 4
  %10 = fadd <8 x float> %7, %9
  %11 = getelementptr [4 x i8], ptr %2, i64 %5
  store <8 x float> %10, ptr %11, align 4
  %12 = or disjoint i64 %5, 8
  %13 = getelementptr [4 x i8], ptr %0, i64 %12
  %14 = load <8 x float>, ptr %13, align 4
  %15 = getelementptr [4 x i8], ptr %1, i64 %12
  %16 = load <8 x float>, ptr %15, align 4
  %17 = fadd <8 x float> %14, %16
  %18 = getelementptr [4 x i8], ptr %2, i64 %12
  store <8 x float> %17, ptr %18, align 4
  %19 = add nuw nsw i64 %5, 16
  %20 = getelementptr [4 x i8], ptr %0, i64 %19
  %21 = load <8 x float>, ptr %20, align 4
  %22 = getelementptr [4 x i8], ptr %1, i64 %19
  %23 = load <8 x float>, ptr %22, align 4
  %24 = fadd <8 x float> %21, %23
  %25 = getelementptr [4 x i8], ptr %2, i64 %19
  store <8 x float> %24, ptr %25, align 4
  %26 = add nuw nsw i64 %5, 24
  %27 = getelementptr [4 x i8], ptr %0, i64 %26
  %28 = load <8 x float>, ptr %27, align 4
  %29 = getelementptr [4 x i8], ptr %1, i64 %26
  %30 = load <8 x float>, ptr %29, align 4
  %31 = fadd <8 x float> %28, %30
  %32 = getelementptr [4 x i8], ptr %2, i64 %26
  store <8 x float> %31, ptr %32, align 4
  %33 = add nuw nsw i64 %5, 32
  %34 = getelementptr [4 x i8], ptr %0, i64 %33
  %35 = load <8 x float>, ptr %34, align 4
  %36 = getelementptr [4 x i8], ptr %1, i64 %33
  %37 = load <8 x float>, ptr %36, align 4
  %38 = fadd <8 x float> %35, %37
  %39 = getelementptr [4 x i8], ptr %2, i64 %33
  store <8 x float> %38, ptr %39, align 4
  %40 = add nuw nsw i64 %5, 40
  %41 = getelementptr [4 x i8], ptr %0, i64 %40
  %42 = load <8 x float>, ptr %41, align 4
  %43 = getelementptr [4 x i8], ptr %1, i64 %40
  %44 = load <8 x float>, ptr %43, align 4
  %45 = fadd <8 x float> %42, %44
  %46 = getelementptr [4 x i8], ptr %2, i64 %40
  store <8 x float> %45, ptr %46, align 4
  %47 = add nuw nsw i64 %5, 48
  %48 = getelementptr [4 x i8], ptr %0, i64 %47
  %49 = load <8 x float>, ptr %48, align 4
  %50 = getelementptr [4 x i8], ptr %1, i64 %47
  %51 = load <8 x float>, ptr %50, align 4
  %52 = fadd <8 x float> %49, %51
  %53 = getelementptr [4 x i8], ptr %2, i64 %47
  store <8 x float> %52, ptr %53, align 4
  %54 = add nuw nsw i64 %5, 56
  %55 = getelementptr [4 x i8], ptr %0, i64 %54
  %56 = load <8 x float>, ptr %55, align 4
  %57 = getelementptr [4 x i8], ptr %1, i64 %54
  %58 = load <8 x float>, ptr %57, align 4
  %59 = fadd <8 x float> %56, %58
  %60 = getelementptr [4 x i8], ptr %2, i64 %54
  store <8 x float> %59, ptr %60, align 4
  %61 = add nuw nsw i64 %5, 64
  %62 = getelementptr [4 x i8], ptr %0, i64 %61
  %63 = load <8 x float>, ptr %62, align 4
  %64 = getelementptr [4 x i8], ptr %1, i64 %61
  %65 = load <8 x float>, ptr %64, align 4
  %66 = fadd <8 x float> %63, %65
  %67 = getelementptr [4 x i8], ptr %2, i64 %61
  store <8 x float> %66, ptr %67, align 4
  %68 = add nuw nsw i64 %5, 72
  %69 = getelementptr [4 x i8], ptr %0, i64 %68
  %70 = load <8 x float>, ptr %69, align 4
  %71 = getelementptr [4 x i8], ptr %1, i64 %68
  %72 = load <8 x float>, ptr %71, align 4
  %73 = fadd <8 x float> %70, %72
  %74 = getelementptr [4 x i8], ptr %2, i64 %68
  store <8 x float> %73, ptr %74, align 4
  %75 = add nuw nsw i64 %5, 80
  %76 = icmp samesign ult i64 %5, 999920
  br i1 %76, label %4, label %77

77:                                               ; preds = %4
  ret void
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define i32 @zl_nucleo_2(ptr nofree readonly captures(none) %0) local_unnamed_addr #1 {
  br label %2

2:                                                ; preds = %2, %1
  %3 = phi <8 x i32> [ zeroinitializer, %1 ], [ %83, %2 ]
  %4 = phi i64 [ 0, %1 ], [ %84, %2 ]
  %5 = getelementptr [4 x i8], ptr %0, i64 %4
  %6 = load <8 x i32>, ptr %5, align 4
  %7 = add <8 x i32> %6, %3
  %8 = getelementptr [4 x i8], ptr %0, i64 %4
  %9 = getelementptr i8, ptr %8, i64 32
  %10 = load <8 x i32>, ptr %9, align 4
  %11 = add <8 x i32> %10, %7
  %12 = getelementptr [4 x i8], ptr %0, i64 %4
  %13 = getelementptr i8, ptr %12, i64 64
  %14 = load <8 x i32>, ptr %13, align 4
  %15 = add <8 x i32> %14, %11
  %16 = getelementptr [4 x i8], ptr %0, i64 %4
  %17 = getelementptr i8, ptr %16, i64 96
  %18 = load <8 x i32>, ptr %17, align 4
  %19 = add <8 x i32> %18, %15
  %20 = getelementptr [4 x i8], ptr %0, i64 %4
  %21 = getelementptr i8, ptr %20, i64 128
  %22 = load <8 x i32>, ptr %21, align 4
  %23 = add <8 x i32> %22, %19
  %24 = getelementptr [4 x i8], ptr %0, i64 %4
  %25 = getelementptr i8, ptr %24, i64 160
  %26 = load <8 x i32>, ptr %25, align 4
  %27 = add <8 x i32> %26, %23
  %28 = getelementptr [4 x i8], ptr %0, i64 %4
  %29 = getelementptr i8, ptr %28, i64 192
  %30 = load <8 x i32>, ptr %29, align 4
  %31 = add <8 x i32> %30, %27
  %32 = getelementptr [4 x i8], ptr %0, i64 %4
  %33 = getelementptr i8, ptr %32, i64 224
  %34 = load <8 x i32>, ptr %33, align 4
  %35 = add <8 x i32> %34, %31
  %36 = getelementptr [4 x i8], ptr %0, i64 %4
  %37 = getelementptr i8, ptr %36, i64 256
  %38 = load <8 x i32>, ptr %37, align 4
  %39 = add <8 x i32> %38, %35
  %40 = getelementptr [4 x i8], ptr %0, i64 %4
  %41 = getelementptr i8, ptr %40, i64 288
  %42 = load <8 x i32>, ptr %41, align 4
  %43 = add <8 x i32> %42, %39
  %44 = getelementptr [4 x i8], ptr %0, i64 %4
  %45 = getelementptr i8, ptr %44, i64 320
  %46 = load <8 x i32>, ptr %45, align 4
  %47 = add <8 x i32> %46, %43
  %48 = getelementptr [4 x i8], ptr %0, i64 %4
  %49 = getelementptr i8, ptr %48, i64 352
  %50 = load <8 x i32>, ptr %49, align 4
  %51 = add <8 x i32> %50, %47
  %52 = getelementptr [4 x i8], ptr %0, i64 %4
  %53 = getelementptr i8, ptr %52, i64 384
  %54 = load <8 x i32>, ptr %53, align 4
  %55 = add <8 x i32> %54, %51
  %56 = getelementptr [4 x i8], ptr %0, i64 %4
  %57 = getelementptr i8, ptr %56, i64 416
  %58 = load <8 x i32>, ptr %57, align 4
  %59 = add <8 x i32> %58, %55
  %60 = getelementptr [4 x i8], ptr %0, i64 %4
  %61 = getelementptr i8, ptr %60, i64 448
  %62 = load <8 x i32>, ptr %61, align 4
  %63 = add <8 x i32> %62, %59
  %64 = getelementptr [4 x i8], ptr %0, i64 %4
  %65 = getelementptr i8, ptr %64, i64 480
  %66 = load <8 x i32>, ptr %65, align 4
  %67 = add <8 x i32> %66, %63
  %68 = getelementptr [4 x i8], ptr %0, i64 %4
  %69 = getelementptr i8, ptr %68, i64 512
  %70 = load <8 x i32>, ptr %69, align 4
  %71 = add <8 x i32> %70, %67
  %72 = getelementptr [4 x i8], ptr %0, i64 %4
  %73 = getelementptr i8, ptr %72, i64 544
  %74 = load <8 x i32>, ptr %73, align 4
  %75 = add <8 x i32> %74, %71
  %76 = getelementptr [4 x i8], ptr %0, i64 %4
  %77 = getelementptr i8, ptr %76, i64 576
  %78 = load <8 x i32>, ptr %77, align 4
  %79 = add <8 x i32> %78, %75
  %80 = getelementptr [4 x i8], ptr %0, i64 %4
  %81 = getelementptr i8, ptr %80, i64 608
  %82 = load <8 x i32>, ptr %81, align 4
  %83 = add <8 x i32> %82, %79
  %84 = add nuw nsw i64 %4, 160
  %85 = icmp samesign ult i64 %4, 999840
  br i1 %85, label %2, label %86

86:                                               ; preds = %2
  %87 = tail call i32 @llvm.vector.reduce.add.v8i32(<8 x i32> %83)
  ret i32 %87
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define float @zl_nucleo_3(ptr nofree readonly captures(none) %0) local_unnamed_addr #1 {
  br label %2

2:                                                ; preds = %2, %1
  %3 = phi <8 x float> [ zeroinitializer, %1 ], [ %83, %2 ]
  %4 = phi i64 [ 0, %1 ], [ %84, %2 ]
  %5 = getelementptr [4 x i8], ptr %0, i64 %4
  %6 = load <8 x float>, ptr %5, align 4
  %7 = fadd <8 x float> %3, %6
  %8 = getelementptr [4 x i8], ptr %0, i64 %4
  %9 = getelementptr i8, ptr %8, i64 32
  %10 = load <8 x float>, ptr %9, align 4
  %11 = fadd <8 x float> %7, %10
  %12 = getelementptr [4 x i8], ptr %0, i64 %4
  %13 = getelementptr i8, ptr %12, i64 64
  %14 = load <8 x float>, ptr %13, align 4
  %15 = fadd <8 x float> %11, %14
  %16 = getelementptr [4 x i8], ptr %0, i64 %4
  %17 = getelementptr i8, ptr %16, i64 96
  %18 = load <8 x float>, ptr %17, align 4
  %19 = fadd <8 x float> %15, %18
  %20 = getelementptr [4 x i8], ptr %0, i64 %4
  %21 = getelementptr i8, ptr %20, i64 128
  %22 = load <8 x float>, ptr %21, align 4
  %23 = fadd <8 x float> %19, %22
  %24 = getelementptr [4 x i8], ptr %0, i64 %4
  %25 = getelementptr i8, ptr %24, i64 160
  %26 = load <8 x float>, ptr %25, align 4
  %27 = fadd <8 x float> %23, %26
  %28 = getelementptr [4 x i8], ptr %0, i64 %4
  %29 = getelementptr i8, ptr %28, i64 192
  %30 = load <8 x float>, ptr %29, align 4
  %31 = fadd <8 x float> %27, %30
  %32 = getelementptr [4 x i8], ptr %0, i64 %4
  %33 = getelementptr i8, ptr %32, i64 224
  %34 = load <8 x float>, ptr %33, align 4
  %35 = fadd <8 x float> %31, %34
  %36 = getelementptr [4 x i8], ptr %0, i64 %4
  %37 = getelementptr i8, ptr %36, i64 256
  %38 = load <8 x float>, ptr %37, align 4
  %39 = fadd <8 x float> %35, %38
  %40 = getelementptr [4 x i8], ptr %0, i64 %4
  %41 = getelementptr i8, ptr %40, i64 288
  %42 = load <8 x float>, ptr %41, align 4
  %43 = fadd <8 x float> %39, %42
  %44 = getelementptr [4 x i8], ptr %0, i64 %4
  %45 = getelementptr i8, ptr %44, i64 320
  %46 = load <8 x float>, ptr %45, align 4
  %47 = fadd <8 x float> %43, %46
  %48 = getelementptr [4 x i8], ptr %0, i64 %4
  %49 = getelementptr i8, ptr %48, i64 352
  %50 = load <8 x float>, ptr %49, align 4
  %51 = fadd <8 x float> %47, %50
  %52 = getelementptr [4 x i8], ptr %0, i64 %4
  %53 = getelementptr i8, ptr %52, i64 384
  %54 = load <8 x float>, ptr %53, align 4
  %55 = fadd <8 x float> %51, %54
  %56 = getelementptr [4 x i8], ptr %0, i64 %4
  %57 = getelementptr i8, ptr %56, i64 416
  %58 = load <8 x float>, ptr %57, align 4
  %59 = fadd <8 x float> %55, %58
  %60 = getelementptr [4 x i8], ptr %0, i64 %4
  %61 = getelementptr i8, ptr %60, i64 448
  %62 = load <8 x float>, ptr %61, align 4
  %63 = fadd <8 x float> %59, %62
  %64 = getelementptr [4 x i8], ptr %0, i64 %4
  %65 = getelementptr i8, ptr %64, i64 480
  %66 = load <8 x float>, ptr %65, align 4
  %67 = fadd <8 x float> %63, %66
  %68 = getelementptr [4 x i8], ptr %0, i64 %4
  %69 = getelementptr i8, ptr %68, i64 512
  %70 = load <8 x float>, ptr %69, align 4
  %71 = fadd <8 x float> %67, %70
  %72 = getelementptr [4 x i8], ptr %0, i64 %4
  %73 = getelementptr i8, ptr %72, i64 544
  %74 = load <8 x float>, ptr %73, align 4
  %75 = fadd <8 x float> %71, %74
  %76 = getelementptr [4 x i8], ptr %0, i64 %4
  %77 = getelementptr i8, ptr %76, i64 576
  %78 = load <8 x float>, ptr %77, align 4
  %79 = fadd <8 x float> %75, %78
  %80 = getelementptr [4 x i8], ptr %0, i64 %4
  %81 = getelementptr i8, ptr %80, i64 608
  %82 = load <8 x float>, ptr %81, align 4
  %83 = fadd <8 x float> %79, %82
  %84 = add nuw nsw i64 %4, 160
  %85 = icmp samesign ult i64 %4, 999840
  br i1 %85, label %2, label %86

86:                                               ; preds = %2
  %87 = tail call float @llvm.vector.reduce.fadd.v8f32(float 0.000000e+00, <8 x float> %83)
  ret float %87
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define float @zl_nucleo_4(ptr nofree readonly captures(none) %0, ptr nofree readonly captures(none) %1) local_unnamed_addr #1 {
  br label %3

3:                                                ; preds = %3, %2
  %4 = phi <8 x float> [ zeroinitializer, %2 ], [ %74, %3 ]
  %5 = phi i64 [ 0, %2 ], [ %75, %3 ]
  %6 = getelementptr [4 x i8], ptr %0, i64 %5
  %7 = load <8 x float>, ptr %6, align 4
  %8 = getelementptr [4 x i8], ptr %1, i64 %5
  %9 = load <8 x float>, ptr %8, align 4
  %10 = fmul <8 x float> %7, %9
  %11 = fadd <8 x float> %4, %10
  %12 = or disjoint i64 %5, 8
  %13 = getelementptr [4 x i8], ptr %0, i64 %12
  %14 = load <8 x float>, ptr %13, align 4
  %15 = getelementptr [4 x i8], ptr %1, i64 %12
  %16 = load <8 x float>, ptr %15, align 4
  %17 = fmul <8 x float> %14, %16
  %18 = fadd <8 x float> %11, %17
  %19 = add nuw nsw i64 %5, 16
  %20 = getelementptr [4 x i8], ptr %0, i64 %19
  %21 = load <8 x float>, ptr %20, align 4
  %22 = getelementptr [4 x i8], ptr %1, i64 %19
  %23 = load <8 x float>, ptr %22, align 4
  %24 = fmul <8 x float> %21, %23
  %25 = fadd <8 x float> %18, %24
  %26 = add nuw nsw i64 %5, 24
  %27 = getelementptr [4 x i8], ptr %0, i64 %26
  %28 = load <8 x float>, ptr %27, align 4
  %29 = getelementptr [4 x i8], ptr %1, i64 %26
  %30 = load <8 x float>, ptr %29, align 4
  %31 = fmul <8 x float> %28, %30
  %32 = fadd <8 x float> %25, %31
  %33 = add nuw nsw i64 %5, 32
  %34 = getelementptr [4 x i8], ptr %0, i64 %33
  %35 = load <8 x float>, ptr %34, align 4
  %36 = getelementptr [4 x i8], ptr %1, i64 %33
  %37 = load <8 x float>, ptr %36, align 4
  %38 = fmul <8 x float> %35, %37
  %39 = fadd <8 x float> %32, %38
  %40 = add nuw nsw i64 %5, 40
  %41 = getelementptr [4 x i8], ptr %0, i64 %40
  %42 = load <8 x float>, ptr %41, align 4
  %43 = getelementptr [4 x i8], ptr %1, i64 %40
  %44 = load <8 x float>, ptr %43, align 4
  %45 = fmul <8 x float> %42, %44
  %46 = fadd <8 x float> %39, %45
  %47 = add nuw nsw i64 %5, 48
  %48 = getelementptr [4 x i8], ptr %0, i64 %47
  %49 = load <8 x float>, ptr %48, align 4
  %50 = getelementptr [4 x i8], ptr %1, i64 %47
  %51 = load <8 x float>, ptr %50, align 4
  %52 = fmul <8 x float> %49, %51
  %53 = fadd <8 x float> %46, %52
  %54 = add nuw nsw i64 %5, 56
  %55 = getelementptr [4 x i8], ptr %0, i64 %54
  %56 = load <8 x float>, ptr %55, align 4
  %57 = getelementptr [4 x i8], ptr %1, i64 %54
  %58 = load <8 x float>, ptr %57, align 4
  %59 = fmul <8 x float> %56, %58
  %60 = fadd <8 x float> %53, %59
  %61 = add nuw nsw i64 %5, 64
  %62 = getelementptr [4 x i8], ptr %0, i64 %61
  %63 = load <8 x float>, ptr %62, align 4
  %64 = getelementptr [4 x i8], ptr %1, i64 %61
  %65 = load <8 x float>, ptr %64, align 4
  %66 = fmul <8 x float> %63, %65
  %67 = fadd <8 x float> %60, %66
  %68 = add nuw nsw i64 %5, 72
  %69 = getelementptr [4 x i8], ptr %0, i64 %68
  %70 = load <8 x float>, ptr %69, align 4
  %71 = getelementptr [4 x i8], ptr %1, i64 %68
  %72 = load <8 x float>, ptr %71, align 4
  %73 = fmul <8 x float> %70, %72
  %74 = fadd <8 x float> %67, %73
  %75 = add nuw nsw i64 %5, 80
  %76 = icmp samesign ult i64 %5, 999920
  br i1 %76, label %3, label %77

77:                                               ; preds = %3
  %78 = tail call float @llvm.vector.reduce.fadd.v8f32(float 0.000000e+00, <8 x float> %74)
  ret float %78
}

; Function Attrs: mustprogress nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.vector.reduce.add.v8i32(<8 x i32>) #2

; Function Attrs: mustprogress nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.vector.reduce.fadd.v8f32(float, <8 x float>) #2

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: readwrite) }
attributes #1 = { nofree norecurse nosync nounwind memory(argmem: read) }
attributes #2 = { mustprogress nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
