; ModuleID = 'build/bench/n1000000/nucleos_C1.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-w64-windows-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite)
define void @zl_nucleo_1(ptr nofree readonly captures(none) %0, ptr nofree readonly captures(none) %1, ptr nofree writeonly captures(none) %2) local_unnamed_addr #0 {
vector.memcheck:
  %3 = ptrtoaddr ptr %2 to i64
  %4 = ptrtoaddr ptr %0 to i64
  %5 = ptrtoaddr ptr %1 to i64
  %6 = sub i64 %4, %3
  %diff.check = icmp ugt i64 %6, -128
  %7 = sub i64 %5, %3
  %diff.check1 = icmp ugt i64 %7, -128
  %conflict.rdx = or i1 %diff.check, %diff.check1
  br i1 %conflict.rdx, label %scalar.ph, label %vector.body

vector.body:                                      ; preds = %vector.memcheck, %vector.body
  %index = phi i64 [ %index.next.1, %vector.body ], [ 0, %vector.memcheck ]
  %8 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %index
  %9 = getelementptr inbounds nuw i8, ptr %8, i64 32
  %10 = getelementptr inbounds nuw i8, ptr %8, i64 64
  %11 = getelementptr inbounds nuw i8, ptr %8, i64 96
  %wide.load = load <8 x float>, ptr %8, align 4
  %wide.load2 = load <8 x float>, ptr %9, align 4
  %wide.load3 = load <8 x float>, ptr %10, align 4
  %wide.load4 = load <8 x float>, ptr %11, align 4
  %12 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %index
  %13 = getelementptr inbounds nuw i8, ptr %12, i64 32
  %14 = getelementptr inbounds nuw i8, ptr %12, i64 64
  %15 = getelementptr inbounds nuw i8, ptr %12, i64 96
  %wide.load5 = load <8 x float>, ptr %12, align 4
  %wide.load6 = load <8 x float>, ptr %13, align 4
  %wide.load7 = load <8 x float>, ptr %14, align 4
  %wide.load8 = load <8 x float>, ptr %15, align 4
  %16 = fadd <8 x float> %wide.load, %wide.load5
  %17 = fadd <8 x float> %wide.load2, %wide.load6
  %18 = fadd <8 x float> %wide.load3, %wide.load7
  %19 = fadd <8 x float> %wide.load4, %wide.load8
  %20 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %index
  %21 = getelementptr inbounds nuw i8, ptr %20, i64 32
  %22 = getelementptr inbounds nuw i8, ptr %20, i64 64
  %23 = getelementptr inbounds nuw i8, ptr %20, i64 96
  store <8 x float> %16, ptr %20, align 4
  store <8 x float> %17, ptr %21, align 4
  store <8 x float> %18, ptr %22, align 4
  store <8 x float> %19, ptr %23, align 4
  %index.next = or disjoint i64 %index, 32
  %24 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %index.next
  %25 = getelementptr inbounds nuw i8, ptr %24, i64 32
  %26 = getelementptr inbounds nuw i8, ptr %24, i64 64
  %27 = getelementptr inbounds nuw i8, ptr %24, i64 96
  %wide.load.1 = load <8 x float>, ptr %24, align 4
  %wide.load2.1 = load <8 x float>, ptr %25, align 4
  %wide.load3.1 = load <8 x float>, ptr %26, align 4
  %wide.load4.1 = load <8 x float>, ptr %27, align 4
  %28 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %index.next
  %29 = getelementptr inbounds nuw i8, ptr %28, i64 32
  %30 = getelementptr inbounds nuw i8, ptr %28, i64 64
  %31 = getelementptr inbounds nuw i8, ptr %28, i64 96
  %wide.load5.1 = load <8 x float>, ptr %28, align 4
  %wide.load6.1 = load <8 x float>, ptr %29, align 4
  %wide.load7.1 = load <8 x float>, ptr %30, align 4
  %wide.load8.1 = load <8 x float>, ptr %31, align 4
  %32 = fadd <8 x float> %wide.load.1, %wide.load5.1
  %33 = fadd <8 x float> %wide.load2.1, %wide.load6.1
  %34 = fadd <8 x float> %wide.load3.1, %wide.load7.1
  %35 = fadd <8 x float> %wide.load4.1, %wide.load8.1
  %36 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %index.next
  %37 = getelementptr inbounds nuw i8, ptr %36, i64 32
  %38 = getelementptr inbounds nuw i8, ptr %36, i64 64
  %39 = getelementptr inbounds nuw i8, ptr %36, i64 96
  store <8 x float> %32, ptr %36, align 4
  store <8 x float> %33, ptr %37, align 4
  store <8 x float> %34, ptr %38, align 4
  store <8 x float> %35, ptr %39, align 4
  %index.next.1 = add nuw nsw i64 %index, 64
  %40 = icmp eq i64 %index.next.1, 1000000
  br i1 %40, label %middle.block, label %vector.body, !llvm.loop !1

scalar.ph:                                        ; preds = %vector.memcheck, %scalar.ph
  %41 = phi i64 [ %111, %scalar.ph ], [ 0, %vector.memcheck ]
  %42 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %41
  %43 = load float, ptr %42, align 4
  %44 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %41
  %45 = load float, ptr %44, align 4
  %46 = fadd float %43, %45
  %47 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %41
  store float %46, ptr %47, align 4
  %48 = or disjoint i64 %41, 1
  %49 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %48
  %50 = load float, ptr %49, align 4
  %51 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %48
  %52 = load float, ptr %51, align 4
  %53 = fadd float %50, %52
  %54 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %48
  store float %53, ptr %54, align 4
  %55 = add nuw nsw i64 %41, 2
  %56 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %55
  %57 = load float, ptr %56, align 4
  %58 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %55
  %59 = load float, ptr %58, align 4
  %60 = fadd float %57, %59
  %61 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %55
  store float %60, ptr %61, align 4
  %62 = add nuw nsw i64 %41, 3
  %63 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %62
  %64 = load float, ptr %63, align 4
  %65 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %62
  %66 = load float, ptr %65, align 4
  %67 = fadd float %64, %66
  %68 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %62
  store float %67, ptr %68, align 4
  %69 = add nuw nsw i64 %41, 4
  %70 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %69
  %71 = load float, ptr %70, align 4
  %72 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %69
  %73 = load float, ptr %72, align 4
  %74 = fadd float %71, %73
  %75 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %69
  store float %74, ptr %75, align 4
  %76 = add nuw nsw i64 %41, 5
  %77 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %76
  %78 = load float, ptr %77, align 4
  %79 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %76
  %80 = load float, ptr %79, align 4
  %81 = fadd float %78, %80
  %82 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %76
  store float %81, ptr %82, align 4
  %83 = add nuw nsw i64 %41, 6
  %84 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %83
  %85 = load float, ptr %84, align 4
  %86 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %83
  %87 = load float, ptr %86, align 4
  %88 = fadd float %85, %87
  %89 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %83
  store float %88, ptr %89, align 4
  %90 = add nuw nsw i64 %41, 7
  %91 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %90
  %92 = load float, ptr %91, align 4
  %93 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %90
  %94 = load float, ptr %93, align 4
  %95 = fadd float %92, %94
  %96 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %90
  store float %95, ptr %96, align 4
  %97 = add nuw nsw i64 %41, 8
  %98 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %97
  %99 = load float, ptr %98, align 4
  %100 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %97
  %101 = load float, ptr %100, align 4
  %102 = fadd float %99, %101
  %103 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %97
  store float %102, ptr %103, align 4
  %104 = add nuw nsw i64 %41, 9
  %105 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %104
  %106 = load float, ptr %105, align 4
  %107 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %104
  %108 = load float, ptr %107, align 4
  %109 = fadd float %106, %108
  %110 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %104
  store float %109, ptr %110, align 4
  %111 = add nuw nsw i64 %41, 10
  %exitcond.not.9 = icmp eq i64 %111, 1000000
  br i1 %exitcond.not.9, label %middle.block, label %scalar.ph, !llvm.loop !4

middle.block:                                     ; preds = %vector.body, %scalar.ph
  ret void
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define i32 @zl_nucleo_2(ptr nofree readonly captures(none) %0) local_unnamed_addr #1 {
vector.ph:
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next.4, %vector.body ]
  %vec.phi = phi <8 x i32> [ zeroinitializer, %vector.ph ], [ %41, %vector.body ]
  %vec.phi1 = phi <8 x i32> [ zeroinitializer, %vector.ph ], [ %42, %vector.body ]
  %vec.phi2 = phi <8 x i32> [ zeroinitializer, %vector.ph ], [ %43, %vector.body ]
  %vec.phi3 = phi <8 x i32> [ zeroinitializer, %vector.ph ], [ %44, %vector.body ]
  %1 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %index
  %2 = getelementptr inbounds nuw i8, ptr %1, i64 32
  %3 = getelementptr inbounds nuw i8, ptr %1, i64 64
  %4 = getelementptr inbounds nuw i8, ptr %1, i64 96
  %wide.load = load <8 x i32>, ptr %1, align 4
  %wide.load4 = load <8 x i32>, ptr %2, align 4
  %wide.load5 = load <8 x i32>, ptr %3, align 4
  %wide.load6 = load <8 x i32>, ptr %4, align 4
  %5 = add <8 x i32> %wide.load, %vec.phi
  %6 = add <8 x i32> %wide.load4, %vec.phi1
  %7 = add <8 x i32> %wide.load5, %vec.phi2
  %8 = add <8 x i32> %wide.load6, %vec.phi3
  %9 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %index
  %10 = getelementptr inbounds nuw i8, ptr %9, i64 128
  %11 = getelementptr inbounds nuw i8, ptr %9, i64 160
  %12 = getelementptr inbounds nuw i8, ptr %9, i64 192
  %13 = getelementptr inbounds nuw i8, ptr %9, i64 224
  %wide.load.1 = load <8 x i32>, ptr %10, align 4
  %wide.load4.1 = load <8 x i32>, ptr %11, align 4
  %wide.load5.1 = load <8 x i32>, ptr %12, align 4
  %wide.load6.1 = load <8 x i32>, ptr %13, align 4
  %14 = add <8 x i32> %wide.load.1, %5
  %15 = add <8 x i32> %wide.load4.1, %6
  %16 = add <8 x i32> %wide.load5.1, %7
  %17 = add <8 x i32> %wide.load6.1, %8
  %18 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %index
  %19 = getelementptr inbounds nuw i8, ptr %18, i64 256
  %20 = getelementptr inbounds nuw i8, ptr %18, i64 288
  %21 = getelementptr inbounds nuw i8, ptr %18, i64 320
  %22 = getelementptr inbounds nuw i8, ptr %18, i64 352
  %wide.load.2 = load <8 x i32>, ptr %19, align 4
  %wide.load4.2 = load <8 x i32>, ptr %20, align 4
  %wide.load5.2 = load <8 x i32>, ptr %21, align 4
  %wide.load6.2 = load <8 x i32>, ptr %22, align 4
  %23 = add <8 x i32> %wide.load.2, %14
  %24 = add <8 x i32> %wide.load4.2, %15
  %25 = add <8 x i32> %wide.load5.2, %16
  %26 = add <8 x i32> %wide.load6.2, %17
  %27 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %index
  %28 = getelementptr inbounds nuw i8, ptr %27, i64 384
  %29 = getelementptr inbounds nuw i8, ptr %27, i64 416
  %30 = getelementptr inbounds nuw i8, ptr %27, i64 448
  %31 = getelementptr inbounds nuw i8, ptr %27, i64 480
  %wide.load.3 = load <8 x i32>, ptr %28, align 4
  %wide.load4.3 = load <8 x i32>, ptr %29, align 4
  %wide.load5.3 = load <8 x i32>, ptr %30, align 4
  %wide.load6.3 = load <8 x i32>, ptr %31, align 4
  %32 = add <8 x i32> %wide.load.3, %23
  %33 = add <8 x i32> %wide.load4.3, %24
  %34 = add <8 x i32> %wide.load5.3, %25
  %35 = add <8 x i32> %wide.load6.3, %26
  %36 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %index
  %37 = getelementptr inbounds nuw i8, ptr %36, i64 512
  %38 = getelementptr inbounds nuw i8, ptr %36, i64 544
  %39 = getelementptr inbounds nuw i8, ptr %36, i64 576
  %40 = getelementptr inbounds nuw i8, ptr %36, i64 608
  %wide.load.4 = load <8 x i32>, ptr %37, align 4
  %wide.load4.4 = load <8 x i32>, ptr %38, align 4
  %wide.load5.4 = load <8 x i32>, ptr %39, align 4
  %wide.load6.4 = load <8 x i32>, ptr %40, align 4
  %41 = add <8 x i32> %wide.load.4, %32
  %42 = add <8 x i32> %wide.load4.4, %33
  %43 = add <8 x i32> %wide.load5.4, %34
  %44 = add <8 x i32> %wide.load6.4, %35
  %index.next.4 = add nuw nsw i64 %index, 160
  %45 = icmp eq i64 %index.next.4, 1000000
  br i1 %45, label %middle.block, label %vector.body, !llvm.loop !5

middle.block:                                     ; preds = %vector.body
  %bin.rdx = add <8 x i32> %42, %41
  %bin.rdx7 = add <8 x i32> %43, %bin.rdx
  %bin.rdx8 = add <8 x i32> %44, %bin.rdx7
  %46 = tail call i32 @llvm.vector.reduce.add.v8i32(<8 x i32> %bin.rdx8)
  ret i32 %46
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define float @zl_nucleo_3(ptr nofree readonly captures(none) %0) local_unnamed_addr #1 {
  br label %2

2:                                                ; preds = %2, %1
  %3 = phi float [ 0.000000e+00, %1 ], [ %83, %2 ]
  %4 = phi i64 [ 0, %1 ], [ %84, %2 ]
  %5 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %6 = load float, ptr %5, align 4
  %7 = fadd float %3, %6
  %8 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %9 = getelementptr inbounds nuw i8, ptr %8, i64 4
  %10 = load float, ptr %9, align 4
  %11 = fadd float %7, %10
  %12 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %13 = getelementptr inbounds nuw i8, ptr %12, i64 8
  %14 = load float, ptr %13, align 4
  %15 = fadd float %11, %14
  %16 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %17 = getelementptr inbounds nuw i8, ptr %16, i64 12
  %18 = load float, ptr %17, align 4
  %19 = fadd float %15, %18
  %20 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %21 = getelementptr inbounds nuw i8, ptr %20, i64 16
  %22 = load float, ptr %21, align 4
  %23 = fadd float %19, %22
  %24 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %25 = getelementptr inbounds nuw i8, ptr %24, i64 20
  %26 = load float, ptr %25, align 4
  %27 = fadd float %23, %26
  %28 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %29 = getelementptr inbounds nuw i8, ptr %28, i64 24
  %30 = load float, ptr %29, align 4
  %31 = fadd float %27, %30
  %32 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %33 = getelementptr inbounds nuw i8, ptr %32, i64 28
  %34 = load float, ptr %33, align 4
  %35 = fadd float %31, %34
  %36 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %37 = getelementptr inbounds nuw i8, ptr %36, i64 32
  %38 = load float, ptr %37, align 4
  %39 = fadd float %35, %38
  %40 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %41 = getelementptr inbounds nuw i8, ptr %40, i64 36
  %42 = load float, ptr %41, align 4
  %43 = fadd float %39, %42
  %44 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %45 = getelementptr inbounds nuw i8, ptr %44, i64 40
  %46 = load float, ptr %45, align 4
  %47 = fadd float %43, %46
  %48 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %49 = getelementptr inbounds nuw i8, ptr %48, i64 44
  %50 = load float, ptr %49, align 4
  %51 = fadd float %47, %50
  %52 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %53 = getelementptr inbounds nuw i8, ptr %52, i64 48
  %54 = load float, ptr %53, align 4
  %55 = fadd float %51, %54
  %56 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %57 = getelementptr inbounds nuw i8, ptr %56, i64 52
  %58 = load float, ptr %57, align 4
  %59 = fadd float %55, %58
  %60 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %61 = getelementptr inbounds nuw i8, ptr %60, i64 56
  %62 = load float, ptr %61, align 4
  %63 = fadd float %59, %62
  %64 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %65 = getelementptr inbounds nuw i8, ptr %64, i64 60
  %66 = load float, ptr %65, align 4
  %67 = fadd float %63, %66
  %68 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %69 = getelementptr inbounds nuw i8, ptr %68, i64 64
  %70 = load float, ptr %69, align 4
  %71 = fadd float %67, %70
  %72 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %73 = getelementptr inbounds nuw i8, ptr %72, i64 68
  %74 = load float, ptr %73, align 4
  %75 = fadd float %71, %74
  %76 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %77 = getelementptr inbounds nuw i8, ptr %76, i64 72
  %78 = load float, ptr %77, align 4
  %79 = fadd float %75, %78
  %80 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %4
  %81 = getelementptr inbounds nuw i8, ptr %80, i64 76
  %82 = load float, ptr %81, align 4
  %83 = fadd float %79, %82
  %84 = add nuw nsw i64 %4, 20
  %exitcond.not.19 = icmp eq i64 %84, 1000000
  br i1 %exitcond.not.19, label %85, label %2

85:                                               ; preds = %2
  ret float %83
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define float @zl_nucleo_4(ptr nofree readonly captures(none) %0, ptr nofree readonly captures(none) %1) local_unnamed_addr #1 {
  br label %3

3:                                                ; preds = %3, %2
  %4 = phi float [ 0.000000e+00, %2 ], [ %74, %3 ]
  %5 = phi i64 [ 0, %2 ], [ %75, %3 ]
  %6 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %5
  %7 = load float, ptr %6, align 4
  %8 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %5
  %9 = load float, ptr %8, align 4
  %10 = fmul float %7, %9
  %11 = fadd float %4, %10
  %12 = or disjoint i64 %5, 1
  %13 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %12
  %14 = load float, ptr %13, align 4
  %15 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %12
  %16 = load float, ptr %15, align 4
  %17 = fmul float %14, %16
  %18 = fadd float %11, %17
  %19 = add nuw nsw i64 %5, 2
  %20 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %19
  %21 = load float, ptr %20, align 4
  %22 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %19
  %23 = load float, ptr %22, align 4
  %24 = fmul float %21, %23
  %25 = fadd float %18, %24
  %26 = add nuw nsw i64 %5, 3
  %27 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %26
  %28 = load float, ptr %27, align 4
  %29 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %26
  %30 = load float, ptr %29, align 4
  %31 = fmul float %28, %30
  %32 = fadd float %25, %31
  %33 = add nuw nsw i64 %5, 4
  %34 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %33
  %35 = load float, ptr %34, align 4
  %36 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %33
  %37 = load float, ptr %36, align 4
  %38 = fmul float %35, %37
  %39 = fadd float %32, %38
  %40 = add nuw nsw i64 %5, 5
  %41 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %40
  %42 = load float, ptr %41, align 4
  %43 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %40
  %44 = load float, ptr %43, align 4
  %45 = fmul float %42, %44
  %46 = fadd float %39, %45
  %47 = add nuw nsw i64 %5, 6
  %48 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %47
  %49 = load float, ptr %48, align 4
  %50 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %47
  %51 = load float, ptr %50, align 4
  %52 = fmul float %49, %51
  %53 = fadd float %46, %52
  %54 = add nuw nsw i64 %5, 7
  %55 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %54
  %56 = load float, ptr %55, align 4
  %57 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %54
  %58 = load float, ptr %57, align 4
  %59 = fmul float %56, %58
  %60 = fadd float %53, %59
  %61 = add nuw nsw i64 %5, 8
  %62 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %61
  %63 = load float, ptr %62, align 4
  %64 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %61
  %65 = load float, ptr %64, align 4
  %66 = fmul float %63, %65
  %67 = fadd float %60, %66
  %68 = add nuw nsw i64 %5, 9
  %69 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %68
  %70 = load float, ptr %69, align 4
  %71 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %68
  %72 = load float, ptr %71, align 4
  %73 = fmul float %70, %72
  %74 = fadd float %67, %73
  %75 = add nuw nsw i64 %5, 10
  %exitcond.not.9 = icmp eq i64 %75, 1000000
  br i1 %exitcond.not.9, label %76, label %3

76:                                               ; preds = %3
  ret float %74
}

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.vector.reduce.add.v8i32(<8 x i32>) #2

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: readwrite) }
attributes #1 = { nofree norecurse nosync nounwind memory(argmem: read) }
attributes #2 = { nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !{!1, !2, !3}
!2 = !{!"llvm.loop.isvectorized", i32 1}
!3 = !{!"llvm.loop.unroll.runtime.disable"}
!4 = distinct !{!4, !2}
!5 = distinct !{!5, !2, !3}
