; ModuleID = 'build/bench/n1000000/nucleos_C1-fastmath.ll'
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
  %16 = fadd fast <8 x float> %wide.load5, %wide.load
  %17 = fadd fast <8 x float> %wide.load6, %wide.load2
  %18 = fadd fast <8 x float> %wide.load7, %wide.load3
  %19 = fadd fast <8 x float> %wide.load8, %wide.load4
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
  %32 = fadd fast <8 x float> %wide.load5.1, %wide.load.1
  %33 = fadd fast <8 x float> %wide.load6.1, %wide.load2.1
  %34 = fadd fast <8 x float> %wide.load7.1, %wide.load3.1
  %35 = fadd fast <8 x float> %wide.load8.1, %wide.load4.1
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
  %46 = fadd fast float %45, %43
  %47 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %41
  store float %46, ptr %47, align 4
  %48 = or disjoint i64 %41, 1
  %49 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %48
  %50 = load float, ptr %49, align 4
  %51 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %48
  %52 = load float, ptr %51, align 4
  %53 = fadd fast float %52, %50
  %54 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %48
  store float %53, ptr %54, align 4
  %55 = add nuw nsw i64 %41, 2
  %56 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %55
  %57 = load float, ptr %56, align 4
  %58 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %55
  %59 = load float, ptr %58, align 4
  %60 = fadd fast float %59, %57
  %61 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %55
  store float %60, ptr %61, align 4
  %62 = add nuw nsw i64 %41, 3
  %63 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %62
  %64 = load float, ptr %63, align 4
  %65 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %62
  %66 = load float, ptr %65, align 4
  %67 = fadd fast float %66, %64
  %68 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %62
  store float %67, ptr %68, align 4
  %69 = add nuw nsw i64 %41, 4
  %70 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %69
  %71 = load float, ptr %70, align 4
  %72 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %69
  %73 = load float, ptr %72, align 4
  %74 = fadd fast float %73, %71
  %75 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %69
  store float %74, ptr %75, align 4
  %76 = add nuw nsw i64 %41, 5
  %77 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %76
  %78 = load float, ptr %77, align 4
  %79 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %76
  %80 = load float, ptr %79, align 4
  %81 = fadd fast float %80, %78
  %82 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %76
  store float %81, ptr %82, align 4
  %83 = add nuw nsw i64 %41, 6
  %84 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %83
  %85 = load float, ptr %84, align 4
  %86 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %83
  %87 = load float, ptr %86, align 4
  %88 = fadd fast float %87, %85
  %89 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %83
  store float %88, ptr %89, align 4
  %90 = add nuw nsw i64 %41, 7
  %91 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %90
  %92 = load float, ptr %91, align 4
  %93 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %90
  %94 = load float, ptr %93, align 4
  %95 = fadd fast float %94, %92
  %96 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %90
  store float %95, ptr %96, align 4
  %97 = add nuw nsw i64 %41, 8
  %98 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %97
  %99 = load float, ptr %98, align 4
  %100 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %97
  %101 = load float, ptr %100, align 4
  %102 = fadd fast float %101, %99
  %103 = getelementptr inbounds nuw [4 x i8], ptr %2, i64 %97
  store float %102, ptr %103, align 4
  %104 = add nuw nsw i64 %41, 9
  %105 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %104
  %106 = load float, ptr %105, align 4
  %107 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %104
  %108 = load float, ptr %107, align 4
  %109 = fadd fast float %108, %106
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
vector.ph:
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next.4, %vector.body ]
  %vec.phi = phi <8 x float> [ zeroinitializer, %vector.ph ], [ %41, %vector.body ]
  %vec.phi1 = phi <8 x float> [ zeroinitializer, %vector.ph ], [ %42, %vector.body ]
  %vec.phi2 = phi <8 x float> [ zeroinitializer, %vector.ph ], [ %43, %vector.body ]
  %vec.phi3 = phi <8 x float> [ zeroinitializer, %vector.ph ], [ %44, %vector.body ]
  %1 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %index
  %2 = getelementptr inbounds nuw i8, ptr %1, i64 32
  %3 = getelementptr inbounds nuw i8, ptr %1, i64 64
  %4 = getelementptr inbounds nuw i8, ptr %1, i64 96
  %wide.load = load <8 x float>, ptr %1, align 4
  %wide.load4 = load <8 x float>, ptr %2, align 4
  %wide.load5 = load <8 x float>, ptr %3, align 4
  %wide.load6 = load <8 x float>, ptr %4, align 4
  %5 = fadd fast <8 x float> %wide.load, %vec.phi
  %6 = fadd fast <8 x float> %wide.load4, %vec.phi1
  %7 = fadd fast <8 x float> %wide.load5, %vec.phi2
  %8 = fadd fast <8 x float> %wide.load6, %vec.phi3
  %9 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %index
  %10 = getelementptr inbounds nuw i8, ptr %9, i64 128
  %11 = getelementptr inbounds nuw i8, ptr %9, i64 160
  %12 = getelementptr inbounds nuw i8, ptr %9, i64 192
  %13 = getelementptr inbounds nuw i8, ptr %9, i64 224
  %wide.load.1 = load <8 x float>, ptr %10, align 4
  %wide.load4.1 = load <8 x float>, ptr %11, align 4
  %wide.load5.1 = load <8 x float>, ptr %12, align 4
  %wide.load6.1 = load <8 x float>, ptr %13, align 4
  %14 = fadd fast <8 x float> %wide.load.1, %5
  %15 = fadd fast <8 x float> %wide.load4.1, %6
  %16 = fadd fast <8 x float> %wide.load5.1, %7
  %17 = fadd fast <8 x float> %wide.load6.1, %8
  %18 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %index
  %19 = getelementptr inbounds nuw i8, ptr %18, i64 256
  %20 = getelementptr inbounds nuw i8, ptr %18, i64 288
  %21 = getelementptr inbounds nuw i8, ptr %18, i64 320
  %22 = getelementptr inbounds nuw i8, ptr %18, i64 352
  %wide.load.2 = load <8 x float>, ptr %19, align 4
  %wide.load4.2 = load <8 x float>, ptr %20, align 4
  %wide.load5.2 = load <8 x float>, ptr %21, align 4
  %wide.load6.2 = load <8 x float>, ptr %22, align 4
  %23 = fadd fast <8 x float> %wide.load.2, %14
  %24 = fadd fast <8 x float> %wide.load4.2, %15
  %25 = fadd fast <8 x float> %wide.load5.2, %16
  %26 = fadd fast <8 x float> %wide.load6.2, %17
  %27 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %index
  %28 = getelementptr inbounds nuw i8, ptr %27, i64 384
  %29 = getelementptr inbounds nuw i8, ptr %27, i64 416
  %30 = getelementptr inbounds nuw i8, ptr %27, i64 448
  %31 = getelementptr inbounds nuw i8, ptr %27, i64 480
  %wide.load.3 = load <8 x float>, ptr %28, align 4
  %wide.load4.3 = load <8 x float>, ptr %29, align 4
  %wide.load5.3 = load <8 x float>, ptr %30, align 4
  %wide.load6.3 = load <8 x float>, ptr %31, align 4
  %32 = fadd fast <8 x float> %wide.load.3, %23
  %33 = fadd fast <8 x float> %wide.load4.3, %24
  %34 = fadd fast <8 x float> %wide.load5.3, %25
  %35 = fadd fast <8 x float> %wide.load6.3, %26
  %36 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %index
  %37 = getelementptr inbounds nuw i8, ptr %36, i64 512
  %38 = getelementptr inbounds nuw i8, ptr %36, i64 544
  %39 = getelementptr inbounds nuw i8, ptr %36, i64 576
  %40 = getelementptr inbounds nuw i8, ptr %36, i64 608
  %wide.load.4 = load <8 x float>, ptr %37, align 4
  %wide.load4.4 = load <8 x float>, ptr %38, align 4
  %wide.load5.4 = load <8 x float>, ptr %39, align 4
  %wide.load6.4 = load <8 x float>, ptr %40, align 4
  %41 = fadd fast <8 x float> %wide.load.4, %32
  %42 = fadd fast <8 x float> %wide.load4.4, %33
  %43 = fadd fast <8 x float> %wide.load5.4, %34
  %44 = fadd fast <8 x float> %wide.load6.4, %35
  %index.next.4 = add nuw nsw i64 %index, 160
  %45 = icmp eq i64 %index.next.4, 1000000
  br i1 %45, label %middle.block, label %vector.body, !llvm.loop !6

middle.block:                                     ; preds = %vector.body
  %bin.rdx = fadd fast <8 x float> %42, %41
  %bin.rdx7 = fadd fast <8 x float> %43, %bin.rdx
  %bin.rdx8 = fadd fast <8 x float> %44, %bin.rdx7
  %46 = tail call fast float @llvm.vector.reduce.fadd.v8f32(float 0.000000e+00, <8 x float> %bin.rdx8)
  ret float %46
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define float @zl_nucleo_4(ptr nofree readonly captures(none) %0, ptr nofree readonly captures(none) %1) local_unnamed_addr #1 {
vector.ph:
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next.1, %vector.body ]
  %vec.phi = phi <8 x float> [ zeroinitializer, %vector.ph ], [ %30, %vector.body ]
  %vec.phi1 = phi <8 x float> [ zeroinitializer, %vector.ph ], [ %31, %vector.body ]
  %vec.phi2 = phi <8 x float> [ zeroinitializer, %vector.ph ], [ %32, %vector.body ]
  %vec.phi3 = phi <8 x float> [ zeroinitializer, %vector.ph ], [ %33, %vector.body ]
  %2 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %index
  %3 = getelementptr inbounds nuw i8, ptr %2, i64 32
  %4 = getelementptr inbounds nuw i8, ptr %2, i64 64
  %5 = getelementptr inbounds nuw i8, ptr %2, i64 96
  %wide.load = load <8 x float>, ptr %2, align 4
  %wide.load4 = load <8 x float>, ptr %3, align 4
  %wide.load5 = load <8 x float>, ptr %4, align 4
  %wide.load6 = load <8 x float>, ptr %5, align 4
  %6 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %index
  %7 = getelementptr inbounds nuw i8, ptr %6, i64 32
  %8 = getelementptr inbounds nuw i8, ptr %6, i64 64
  %9 = getelementptr inbounds nuw i8, ptr %6, i64 96
  %wide.load7 = load <8 x float>, ptr %6, align 4
  %wide.load8 = load <8 x float>, ptr %7, align 4
  %wide.load9 = load <8 x float>, ptr %8, align 4
  %wide.load10 = load <8 x float>, ptr %9, align 4
  %10 = fmul fast <8 x float> %wide.load7, %wide.load
  %11 = fmul fast <8 x float> %wide.load8, %wide.load4
  %12 = fmul fast <8 x float> %wide.load9, %wide.load5
  %13 = fmul fast <8 x float> %wide.load10, %wide.load6
  %14 = fadd fast <8 x float> %10, %vec.phi
  %15 = fadd fast <8 x float> %11, %vec.phi1
  %16 = fadd fast <8 x float> %12, %vec.phi2
  %17 = fadd fast <8 x float> %13, %vec.phi3
  %index.next = or disjoint i64 %index, 32
  %18 = getelementptr inbounds nuw [4 x i8], ptr %0, i64 %index.next
  %19 = getelementptr inbounds nuw i8, ptr %18, i64 32
  %20 = getelementptr inbounds nuw i8, ptr %18, i64 64
  %21 = getelementptr inbounds nuw i8, ptr %18, i64 96
  %wide.load.1 = load <8 x float>, ptr %18, align 4
  %wide.load4.1 = load <8 x float>, ptr %19, align 4
  %wide.load5.1 = load <8 x float>, ptr %20, align 4
  %wide.load6.1 = load <8 x float>, ptr %21, align 4
  %22 = getelementptr inbounds nuw [4 x i8], ptr %1, i64 %index.next
  %23 = getelementptr inbounds nuw i8, ptr %22, i64 32
  %24 = getelementptr inbounds nuw i8, ptr %22, i64 64
  %25 = getelementptr inbounds nuw i8, ptr %22, i64 96
  %wide.load7.1 = load <8 x float>, ptr %22, align 4
  %wide.load8.1 = load <8 x float>, ptr %23, align 4
  %wide.load9.1 = load <8 x float>, ptr %24, align 4
  %wide.load10.1 = load <8 x float>, ptr %25, align 4
  %26 = fmul fast <8 x float> %wide.load7.1, %wide.load.1
  %27 = fmul fast <8 x float> %wide.load8.1, %wide.load4.1
  %28 = fmul fast <8 x float> %wide.load9.1, %wide.load5.1
  %29 = fmul fast <8 x float> %wide.load10.1, %wide.load6.1
  %30 = fadd fast <8 x float> %26, %14
  %31 = fadd fast <8 x float> %27, %15
  %32 = fadd fast <8 x float> %28, %16
  %33 = fadd fast <8 x float> %29, %17
  %index.next.1 = add nuw nsw i64 %index, 64
  %34 = icmp eq i64 %index.next.1, 1000000
  br i1 %34, label %middle.block, label %vector.body, !llvm.loop !7

middle.block:                                     ; preds = %vector.body
  %bin.rdx = fadd fast <8 x float> %31, %30
  %bin.rdx11 = fadd fast <8 x float> %32, %bin.rdx
  %bin.rdx12 = fadd fast <8 x float> %33, %bin.rdx11
  %35 = tail call fast float @llvm.vector.reduce.fadd.v8f32(float 0.000000e+00, <8 x float> %bin.rdx12)
  ret float %35
}

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.vector.reduce.add.v8i32(<8 x i32>) #2

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.vector.reduce.fadd.v8f32(float, <8 x float>) #2

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
!6 = distinct !{!6, !2, !3}
!7 = distinct !{!7, !2, !3}
