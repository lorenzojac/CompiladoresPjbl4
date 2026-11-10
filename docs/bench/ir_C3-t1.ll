; ModuleID = 'build/bench/n1000000/nucleos_C3-t1.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-w64-windows-gnu"

%struct.ident_t = type { i32, i32, i32, i32, ptr }

@0 = private unnamed_addr constant [23 x i8] c";unknown;unknown;0;0;;\00", align 1
@1 = private unnamed_addr constant %struct.ident_t { i32 0, i32 2, i32 0, i32 22, ptr @0 }, align 8
@2 = private unnamed_addr constant %struct.ident_t { i32 0, i32 514, i32 0, i32 22, ptr @0 }, align 8
@3 = private unnamed_addr constant %struct.ident_t { i32 0, i32 66, i32 0, i32 22, ptr @0 }, align 8
@4 = private unnamed_addr constant %struct.ident_t { i32 0, i32 18, i32 0, i32 22, ptr @0 }, align 8
@.gomp_critical_user_.reduction.var = common global [8 x i32] zeroinitializer, align 8

; Function Attrs: nounwind
define void @zl_nucleo_1(ptr %0, ptr %1, ptr %2) local_unnamed_addr #0 {
entry:
  %structArg = alloca { ptr, ptr, ptr }, align 8
  %.reloaded = alloca { ptr, ptr, i64, [1 x i64], [1 x i64] }, align 8
  %.reloaded6 = alloca { ptr, ptr, i64, [1 x i64], [1 x i64] }, align 8
  %.reloaded7 = alloca { ptr, ptr, i64, [1 x i64], [1 x i64] }, align 8
  store ptr %0, ptr %.reloaded, align 8
  %.fca.1.gep14 = getelementptr inbounds nuw i8, ptr %.reloaded, i64 8
  store ptr %0, ptr %.fca.1.gep14, align 8
  %.fca.2.gep16 = getelementptr inbounds nuw i8, ptr %.reloaded, i64 16
  store i64 0, ptr %.fca.2.gep16, align 8
  %.fca.3.0.gep18 = getelementptr inbounds nuw i8, ptr %.reloaded, i64 24
  store i64 1000000, ptr %.fca.3.0.gep18, align 8
  %.fca.4.0.gep20 = getelementptr inbounds nuw i8, ptr %.reloaded, i64 32
  store i64 1, ptr %.fca.4.0.gep20, align 8
  store ptr %1, ptr %.reloaded6, align 8
  %.fca.1.gep4 = getelementptr inbounds nuw i8, ptr %.reloaded6, i64 8
  store ptr %1, ptr %.fca.1.gep4, align 8
  %.fca.2.gep6 = getelementptr inbounds nuw i8, ptr %.reloaded6, i64 16
  store i64 0, ptr %.fca.2.gep6, align 8
  %.fca.3.0.gep8 = getelementptr inbounds nuw i8, ptr %.reloaded6, i64 24
  store i64 1000000, ptr %.fca.3.0.gep8, align 8
  %.fca.4.0.gep10 = getelementptr inbounds nuw i8, ptr %.reloaded6, i64 32
  store i64 1, ptr %.fca.4.0.gep10, align 8
  store ptr %2, ptr %.reloaded7, align 8
  %.fca.1.gep = getelementptr inbounds nuw i8, ptr %.reloaded7, i64 8
  store ptr %2, ptr %.fca.1.gep, align 8
  %.fca.2.gep = getelementptr inbounds nuw i8, ptr %.reloaded7, i64 16
  store i64 0, ptr %.fca.2.gep, align 8
  %.fca.3.0.gep = getelementptr inbounds nuw i8, ptr %.reloaded7, i64 24
  store i64 1000000, ptr %.fca.3.0.gep, align 8
  %.fca.4.0.gep = getelementptr inbounds nuw i8, ptr %.reloaded7, i64 32
  store i64 1, ptr %.fca.4.0.gep, align 8
  store ptr %.reloaded, ptr %structArg, align 8
  %gep_.reloaded6 = getelementptr inbounds nuw i8, ptr %structArg, i64 8
  store ptr %.reloaded6, ptr %gep_.reloaded6, align 8
  %gep_.reloaded7 = getelementptr inbounds nuw i8, ptr %structArg, i64 16
  store ptr %.reloaded7, ptr %gep_.reloaded7, align 8
  call void (ptr, i32, ptr, ...) @__kmpc_fork_call(ptr nonnull @1, i32 1, ptr nonnull @zl_nucleo_1..omp_par, ptr nonnull %structArg)
  ret void
}

; Function Attrs: nounwind
define internal void @zl_nucleo_1..omp_par(ptr noalias nofree readnone captures(none) %tid.addr, ptr noalias nofree readnone captures(none) %zero.addr, ptr nofree readonly captures(none) %0) #0 {
omp.par.entry:
  %loadgep_.reloaded = load ptr, ptr %0, align 8, !align !1
  %gep_.reloaded6 = getelementptr i8, ptr %0, i64 8
  %loadgep_.reloaded6 = load ptr, ptr %gep_.reloaded6, align 8, !align !1
  %gep_.reloaded7 = getelementptr i8, ptr %0, i64 16
  %loadgep_.reloaded7 = load ptr, ptr %gep_.reloaded7, align 8, !align !1
  %p.lastiter = alloca i32, align 4
  %p.lowerbound = alloca i64, align 8
  %p.upperbound = alloca i64, align 8
  %p.stride = alloca i64, align 8
  %.elt1 = getelementptr inbounds nuw i8, ptr %loadgep_.reloaded, i64 8
  %.unpack2 = load ptr, ptr %.elt1, align 8
  %.elt12 = getelementptr inbounds nuw i8, ptr %loadgep_.reloaded6, i64 8
  %.unpack13 = load ptr, ptr %.elt12, align 8
  %.elt23 = getelementptr inbounds nuw i8, ptr %loadgep_.reloaded7, i64 8
  %.unpack24 = load ptr, ptr %.elt23, align 8
  store i64 0, ptr %p.lowerbound, align 8
  store i64 999999, ptr %p.upperbound, align 8
  store i64 1, ptr %p.stride, align 8
  %omp_global_thread_num = tail call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_for_static_init_8u(ptr nonnull @2, i32 %omp_global_thread_num, i32 34, ptr nonnull %p.lastiter, ptr nonnull %p.lowerbound, ptr nonnull %p.upperbound, ptr nonnull %p.stride, i64 1, i64 0)
  %1 = load i64, ptr %p.lowerbound, align 8
  %2 = load i64, ptr %p.upperbound, align 8
  %reass.sub = sub i64 %2, %1
  %omp_loop.cmp33.not = icmp eq i64 %reass.sub, -1
  br i1 %omp_loop.cmp33.not, label %omp_loop.exit, label %omp_loop.body.preheader

omp_loop.body.preheader:                          ; preds = %omp.par.entry
  %3 = add i64 %2, 1
  %4 = sub i64 %3, %1
  %xtraiter = and i64 %4, 7
  %5 = icmp ult i64 %reass.sub, 7
  br i1 %5, label %omp_loop.body.epil.preheader, label %omp_loop.body.preheader.new

omp_loop.body.preheader.new:                      ; preds = %omp_loop.body.preheader
  %unroll_iter = and i64 %4, -8
  br label %omp_loop.body

omp_loop.exit.loopexit.unr-lcssa:                 ; preds = %omp_loop.body
  %lcmp.mod.not = icmp eq i64 %xtraiter, 0
  br i1 %lcmp.mod.not, label %omp_loop.exit, label %omp_loop.body.epil.preheader

omp_loop.body.epil.preheader:                     ; preds = %omp_loop.exit.loopexit.unr-lcssa, %omp_loop.body.preheader
  %omp_loop.iv34.epil.init = phi i64 [ 0, %omp_loop.body.preheader ], [ %omp_loop.next.7, %omp_loop.exit.loopexit.unr-lcssa ]
  %lcmp.mod35 = icmp ne i64 %xtraiter, 0
  call void @llvm.assume(i1 %lcmp.mod35)
  br label %omp_loop.body.epil

omp_loop.body.epil:                               ; preds = %omp_loop.body.epil, %omp_loop.body.epil.preheader
  %omp_loop.iv34.epil = phi i64 [ %omp_loop.next.epil, %omp_loop.body.epil ], [ %omp_loop.iv34.epil.init, %omp_loop.body.epil.preheader ]
  %epil.iter = phi i64 [ %epil.iter.next, %omp_loop.body.epil ], [ 0, %omp_loop.body.epil.preheader ]
  %6 = add i64 %omp_loop.iv34.epil, %1
  %7 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %6
  %8 = load float, ptr %7, align 4
  %9 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %6
  %10 = load float, ptr %9, align 4
  %11 = fadd float %8, %10
  %12 = getelementptr inbounds nuw [4 x i8], ptr %.unpack24, i64 %6
  store float %11, ptr %12, align 4
  %omp_loop.next.epil = add nuw i64 %omp_loop.iv34.epil, 1
  %epil.iter.next = add i64 %epil.iter, 1
  %epil.iter.cmp.not = icmp eq i64 %epil.iter.next, %xtraiter
  br i1 %epil.iter.cmp.not, label %omp_loop.exit, label %omp_loop.body.epil, !llvm.loop !2

omp_loop.exit:                                    ; preds = %omp_loop.exit.loopexit.unr-lcssa, %omp_loop.body.epil, %omp.par.entry
  call void @__kmpc_for_static_fini(ptr nonnull @2, i32 %omp_global_thread_num)
  %omp_global_thread_num5 = call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_barrier(ptr nonnull @3, i32 %omp_global_thread_num5)
  ret void

omp_loop.body:                                    ; preds = %omp_loop.body, %omp_loop.body.preheader.new
  %omp_loop.iv34 = phi i64 [ 0, %omp_loop.body.preheader.new ], [ %omp_loop.next.7, %omp_loop.body ]
  %niter = phi i64 [ 0, %omp_loop.body.preheader.new ], [ %niter.next.7, %omp_loop.body ]
  %13 = add i64 %omp_loop.iv34, %1
  %14 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %13
  %15 = load float, ptr %14, align 4
  %16 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %13
  %17 = load float, ptr %16, align 4
  %18 = fadd float %15, %17
  %19 = getelementptr inbounds nuw [4 x i8], ptr %.unpack24, i64 %13
  store float %18, ptr %19, align 4
  %omp_loop.next = or disjoint i64 %omp_loop.iv34, 1
  %20 = add i64 %omp_loop.next, %1
  %21 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %20
  %22 = load float, ptr %21, align 4
  %23 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %20
  %24 = load float, ptr %23, align 4
  %25 = fadd float %22, %24
  %26 = getelementptr inbounds nuw [4 x i8], ptr %.unpack24, i64 %20
  store float %25, ptr %26, align 4
  %omp_loop.next.1 = or disjoint i64 %omp_loop.iv34, 2
  %27 = add i64 %omp_loop.next.1, %1
  %28 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %27
  %29 = load float, ptr %28, align 4
  %30 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %27
  %31 = load float, ptr %30, align 4
  %32 = fadd float %29, %31
  %33 = getelementptr inbounds nuw [4 x i8], ptr %.unpack24, i64 %27
  store float %32, ptr %33, align 4
  %omp_loop.next.2 = or disjoint i64 %omp_loop.iv34, 3
  %34 = add i64 %omp_loop.next.2, %1
  %35 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %34
  %36 = load float, ptr %35, align 4
  %37 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %34
  %38 = load float, ptr %37, align 4
  %39 = fadd float %36, %38
  %40 = getelementptr inbounds nuw [4 x i8], ptr %.unpack24, i64 %34
  store float %39, ptr %40, align 4
  %omp_loop.next.3 = or disjoint i64 %omp_loop.iv34, 4
  %41 = add i64 %omp_loop.next.3, %1
  %42 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %41
  %43 = load float, ptr %42, align 4
  %44 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %41
  %45 = load float, ptr %44, align 4
  %46 = fadd float %43, %45
  %47 = getelementptr inbounds nuw [4 x i8], ptr %.unpack24, i64 %41
  store float %46, ptr %47, align 4
  %omp_loop.next.4 = or disjoint i64 %omp_loop.iv34, 5
  %48 = add i64 %omp_loop.next.4, %1
  %49 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %48
  %50 = load float, ptr %49, align 4
  %51 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %48
  %52 = load float, ptr %51, align 4
  %53 = fadd float %50, %52
  %54 = getelementptr inbounds nuw [4 x i8], ptr %.unpack24, i64 %48
  store float %53, ptr %54, align 4
  %omp_loop.next.5 = or disjoint i64 %omp_loop.iv34, 6
  %55 = add i64 %omp_loop.next.5, %1
  %56 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %55
  %57 = load float, ptr %56, align 4
  %58 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %55
  %59 = load float, ptr %58, align 4
  %60 = fadd float %57, %59
  %61 = getelementptr inbounds nuw [4 x i8], ptr %.unpack24, i64 %55
  store float %60, ptr %61, align 4
  %omp_loop.next.6 = or disjoint i64 %omp_loop.iv34, 7
  %62 = add i64 %omp_loop.next.6, %1
  %63 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %62
  %64 = load float, ptr %63, align 4
  %65 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %62
  %66 = load float, ptr %65, align 4
  %67 = fadd float %64, %66
  %68 = getelementptr inbounds nuw [4 x i8], ptr %.unpack24, i64 %62
  store float %67, ptr %68, align 4
  %omp_loop.next.7 = add nuw i64 %omp_loop.iv34, 8
  %niter.next.7 = add i64 %niter, 8
  %niter.ncmp.7 = icmp eq i64 %niter.next.7, %unroll_iter
  br i1 %niter.ncmp.7, label %omp_loop.exit.loopexit.unr-lcssa, label %omp_loop.body
}

; Function Attrs: nounwind
define i32 @zl_nucleo_2(ptr %0) local_unnamed_addr #0 {
entry:
  %structArg = alloca { ptr, ptr }, align 8
  %.reloaded = alloca { ptr, ptr, i64, [1 x i64], [1 x i64] }, align 8
  %1 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  store ptr %0, ptr %.reloaded, align 8
  %.fca.1.gep = getelementptr inbounds nuw i8, ptr %.reloaded, i64 8
  store ptr %0, ptr %.fca.1.gep, align 8
  %.fca.2.gep = getelementptr inbounds nuw i8, ptr %.reloaded, i64 16
  store i64 0, ptr %.fca.2.gep, align 8
  %.fca.3.0.gep = getelementptr inbounds nuw i8, ptr %.reloaded, i64 24
  store i64 1000000, ptr %.fca.3.0.gep, align 8
  %.fca.4.0.gep = getelementptr inbounds nuw i8, ptr %.reloaded, i64 32
  store i64 1, ptr %.fca.4.0.gep, align 8
  store ptr %.reloaded, ptr %structArg, align 8
  %gep_ = getelementptr inbounds nuw i8, ptr %structArg, i64 8
  store ptr %1, ptr %gep_, align 8
  call void (ptr, i32, ptr, ...) @__kmpc_fork_call(ptr nonnull @1, i32 1, ptr nonnull @zl_nucleo_2..omp_par, ptr nonnull %structArg)
  %2 = load i32, ptr %1, align 4
  ret i32 %2
}

; Function Attrs: nounwind
define internal void @zl_nucleo_2..omp_par(ptr noalias nofree readnone captures(none) %tid.addr, ptr noalias nofree readnone captures(none) %zero.addr, ptr nofree readonly captures(none) %0) #0 {
omp.par.entry:
  %loadgep_.reloaded = load ptr, ptr %0, align 8, !align !1
  %gep_ = getelementptr i8, ptr %0, i64 8
  %loadgep_ = load ptr, ptr %gep_, align 8, !align !4
  %p.lastiter = alloca i32, align 4
  %p.lowerbound = alloca i64, align 8
  %p.upperbound = alloca i64, align 8
  %p.stride = alloca i64, align 8
  %1 = alloca i32, align 4
  %red.array = alloca [1 x ptr], align 8
  %.elt1 = getelementptr inbounds nuw i8, ptr %loadgep_.reloaded, i64 8
  %.unpack2 = load ptr, ptr %.elt1, align 8
  store i32 0, ptr %1, align 4
  store i64 0, ptr %p.lowerbound, align 8
  store i64 999999, ptr %p.upperbound, align 8
  store i64 1, ptr %p.stride, align 8
  %omp_global_thread_num = tail call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_for_static_init_8u(ptr nonnull @2, i32 %omp_global_thread_num, i32 34, ptr nonnull %p.lastiter, ptr nonnull %p.lowerbound, ptr nonnull %p.upperbound, ptr nonnull %p.stride, i64 1, i64 0)
  %2 = load i64, ptr %p.lowerbound, align 8
  %3 = load i64, ptr %p.upperbound, align 8
  %reass.sub = sub i64 %3, %2
  %invariant.gep = getelementptr [4 x i8], ptr %.unpack2, i64 %2
  %omp_loop.cmp11.not = icmp eq i64 %reass.sub, -1
  br i1 %omp_loop.cmp11.not, label %omp_loop.exit, label %omp_loop.body.preheader

omp_loop.body.preheader:                          ; preds = %omp.par.entry
  %4 = add i64 %3, 1
  %5 = sub i64 %4, %2
  %xtraiter = and i64 %5, 7
  %6 = icmp ult i64 %reass.sub, 7
  br i1 %6, label %omp_loop.body.epil.preheader, label %omp_loop.body.preheader.new

omp_loop.body.preheader.new:                      ; preds = %omp_loop.body.preheader
  %unroll_iter = and i64 %5, -8
  br label %omp_loop.body

omp_loop.exit.loopexit.unr-lcssa:                 ; preds = %omp_loop.body
  %lcmp.mod.not = icmp eq i64 %xtraiter, 0
  br i1 %lcmp.mod.not, label %omp_loop.exit, label %omp_loop.body.epil.preheader

omp_loop.body.epil.preheader:                     ; preds = %omp_loop.exit.loopexit.unr-lcssa, %omp_loop.body.preheader
  %omp_loop.iv12.epil.init = phi i64 [ 0, %omp_loop.body.preheader ], [ %omp_loop.next.7, %omp_loop.exit.loopexit.unr-lcssa ]
  %.epil.init = phi i32 [ 0, %omp_loop.body.preheader ], [ %36, %omp_loop.exit.loopexit.unr-lcssa ]
  %lcmp.mod13 = icmp ne i64 %xtraiter, 0
  call void @llvm.assume(i1 %lcmp.mod13)
  br label %omp_loop.body.epil

omp_loop.body.epil:                               ; preds = %omp_loop.body.epil, %omp_loop.body.epil.preheader
  %omp_loop.iv12.epil = phi i64 [ %omp_loop.next.epil, %omp_loop.body.epil ], [ %omp_loop.iv12.epil.init, %omp_loop.body.epil.preheader ]
  %7 = phi i32 [ %9, %omp_loop.body.epil ], [ %.epil.init, %omp_loop.body.epil.preheader ]
  %epil.iter = phi i64 [ %epil.iter.next, %omp_loop.body.epil ], [ 0, %omp_loop.body.epil.preheader ]
  %gep.epil = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12.epil
  %8 = load i32, ptr %gep.epil, align 4
  %9 = add i32 %7, %8
  store i32 %9, ptr %1, align 4
  %omp_loop.next.epil = add nuw i64 %omp_loop.iv12.epil, 1
  %epil.iter.next = add i64 %epil.iter, 1
  %epil.iter.cmp.not = icmp eq i64 %epil.iter.next, %xtraiter
  br i1 %epil.iter.cmp.not, label %omp_loop.exit, label %omp_loop.body.epil, !llvm.loop !5

omp_loop.exit:                                    ; preds = %omp_loop.exit.loopexit.unr-lcssa, %omp_loop.body.epil, %omp.par.entry
  call void @__kmpc_for_static_fini(ptr nonnull @2, i32 %omp_global_thread_num)
  %omp_global_thread_num5 = call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_barrier(ptr nonnull @3, i32 %omp_global_thread_num5)
  store ptr %1, ptr %red.array, align 8
  %omp_global_thread_num6 = call i32 @__kmpc_global_thread_num(ptr nonnull @4)
  %reduce = call i32 @__kmpc_reduce(ptr nonnull @4, i32 %omp_global_thread_num6, i32 1, i64 8, ptr nonnull %red.array, ptr nonnull @.omp.reduction.func, ptr nonnull @.gomp_critical_user_.reduction.var)
  switch i32 %reduce, label %reduce.finalize [
    i32 1, label %reduce.switch.nonatomic
    i32 2, label %reduce.switch.atomic
  ]

reduce.switch.atomic:                             ; preds = %omp_loop.exit
  %10 = load i32, ptr %1, align 4
  %11 = atomicrmw add ptr %loadgep_, i32 %10 monotonic, align 4
  br label %reduce.finalize

reduce.switch.nonatomic:                          ; preds = %omp_loop.exit
  %red.value.0 = load i32, ptr %loadgep_, align 4
  %red.private.value.0 = load i32, ptr %1, align 4
  %12 = add i32 %red.private.value.0, %red.value.0
  store i32 %12, ptr %loadgep_, align 4
  call void @__kmpc_end_reduce(ptr nonnull @4, i32 %omp_global_thread_num6, ptr nonnull @.gomp_critical_user_.reduction.var)
  br label %reduce.finalize

reduce.finalize:                                  ; preds = %reduce.switch.nonatomic, %reduce.switch.atomic, %omp_loop.exit
  %omp_global_thread_num7 = call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_barrier(ptr nonnull @3, i32 %omp_global_thread_num7)
  ret void

omp_loop.body:                                    ; preds = %omp_loop.body, %omp_loop.body.preheader.new
  %omp_loop.iv12 = phi i64 [ 0, %omp_loop.body.preheader.new ], [ %omp_loop.next.7, %omp_loop.body ]
  %13 = phi i32 [ 0, %omp_loop.body.preheader.new ], [ %36, %omp_loop.body ]
  %niter = phi i64 [ 0, %omp_loop.body.preheader.new ], [ %niter.next.7, %omp_loop.body ]
  %gep = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12
  %14 = load i32, ptr %gep, align 4
  %15 = add i32 %13, %14
  store i32 %15, ptr %1, align 4
  %16 = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12
  %gep.1 = getelementptr i8, ptr %16, i64 4
  %17 = load i32, ptr %gep.1, align 4
  %18 = add i32 %15, %17
  store i32 %18, ptr %1, align 4
  %19 = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12
  %gep.2 = getelementptr i8, ptr %19, i64 8
  %20 = load i32, ptr %gep.2, align 4
  %21 = add i32 %18, %20
  store i32 %21, ptr %1, align 4
  %22 = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12
  %gep.3 = getelementptr i8, ptr %22, i64 12
  %23 = load i32, ptr %gep.3, align 4
  %24 = add i32 %21, %23
  store i32 %24, ptr %1, align 4
  %25 = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12
  %gep.4 = getelementptr i8, ptr %25, i64 16
  %26 = load i32, ptr %gep.4, align 4
  %27 = add i32 %24, %26
  store i32 %27, ptr %1, align 4
  %28 = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12
  %gep.5 = getelementptr i8, ptr %28, i64 20
  %29 = load i32, ptr %gep.5, align 4
  %30 = add i32 %27, %29
  store i32 %30, ptr %1, align 4
  %31 = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12
  %gep.6 = getelementptr i8, ptr %31, i64 24
  %32 = load i32, ptr %gep.6, align 4
  %33 = add i32 %30, %32
  store i32 %33, ptr %1, align 4
  %34 = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12
  %gep.7 = getelementptr i8, ptr %34, i64 28
  %35 = load i32, ptr %gep.7, align 4
  %36 = add i32 %33, %35
  store i32 %36, ptr %1, align 4
  %omp_loop.next.7 = add nuw i64 %omp_loop.iv12, 8
  %niter.next.7 = add i64 %niter, 8
  %niter.ncmp.7 = icmp eq i64 %niter.next.7, %unroll_iter
  br i1 %niter.ncmp.7, label %omp_loop.exit.loopexit.unr-lcssa, label %omp_loop.body
}

; Function Attrs: nounwind
define float @zl_nucleo_3(ptr %0) local_unnamed_addr #0 {
entry:
  %structArg = alloca { ptr, ptr }, align 8
  %.reloaded = alloca { ptr, ptr, i64, [1 x i64], [1 x i64] }, align 8
  %1 = alloca float, align 4
  store float 0.000000e+00, ptr %1, align 4
  store ptr %0, ptr %.reloaded, align 8
  %.fca.1.gep = getelementptr inbounds nuw i8, ptr %.reloaded, i64 8
  store ptr %0, ptr %.fca.1.gep, align 8
  %.fca.2.gep = getelementptr inbounds nuw i8, ptr %.reloaded, i64 16
  store i64 0, ptr %.fca.2.gep, align 8
  %.fca.3.0.gep = getelementptr inbounds nuw i8, ptr %.reloaded, i64 24
  store i64 1000000, ptr %.fca.3.0.gep, align 8
  %.fca.4.0.gep = getelementptr inbounds nuw i8, ptr %.reloaded, i64 32
  store i64 1, ptr %.fca.4.0.gep, align 8
  store ptr %.reloaded, ptr %structArg, align 8
  %gep_ = getelementptr inbounds nuw i8, ptr %structArg, i64 8
  store ptr %1, ptr %gep_, align 8
  call void (ptr, i32, ptr, ...) @__kmpc_fork_call(ptr nonnull @1, i32 1, ptr nonnull @zl_nucleo_3..omp_par, ptr nonnull %structArg)
  %2 = load float, ptr %1, align 4
  %3 = fadd float %2, 0.000000e+00
  ret float %3
}

; Function Attrs: nounwind
define internal void @zl_nucleo_3..omp_par(ptr noalias nofree readnone captures(none) %tid.addr, ptr noalias nofree readnone captures(none) %zero.addr, ptr nofree readonly captures(none) %0) #0 {
omp.par.entry:
  %loadgep_.reloaded = load ptr, ptr %0, align 8, !align !1
  %gep_ = getelementptr i8, ptr %0, i64 8
  %loadgep_ = load ptr, ptr %gep_, align 8, !align !4
  %p.lastiter = alloca i32, align 4
  %p.lowerbound = alloca i64, align 8
  %p.upperbound = alloca i64, align 8
  %p.stride = alloca i64, align 8
  %1 = alloca float, align 4
  %red.array = alloca [1 x ptr], align 8
  %.elt1 = getelementptr inbounds nuw i8, ptr %loadgep_.reloaded, i64 8
  %.unpack2 = load ptr, ptr %.elt1, align 8
  store float 0.000000e+00, ptr %1, align 4
  store i64 0, ptr %p.lowerbound, align 8
  store i64 999999, ptr %p.upperbound, align 8
  store i64 1, ptr %p.stride, align 8
  %omp_global_thread_num = tail call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_for_static_init_8u(ptr nonnull @2, i32 %omp_global_thread_num, i32 34, ptr nonnull %p.lastiter, ptr nonnull %p.lowerbound, ptr nonnull %p.upperbound, ptr nonnull %p.stride, i64 1, i64 0)
  %2 = load i64, ptr %p.lowerbound, align 8
  %3 = load i64, ptr %p.upperbound, align 8
  %reass.sub = sub i64 %3, %2
  %invariant.gep = getelementptr [4 x i8], ptr %.unpack2, i64 %2
  %omp_loop.cmp11.not = icmp eq i64 %reass.sub, -1
  br i1 %omp_loop.cmp11.not, label %omp_loop.exit, label %omp_loop.body.preheader

omp_loop.body.preheader:                          ; preds = %omp.par.entry
  %4 = add i64 %3, 1
  %5 = sub i64 %4, %2
  %xtraiter = and i64 %5, 7
  %6 = icmp ult i64 %reass.sub, 7
  br i1 %6, label %omp_loop.body.epil.preheader, label %omp_loop.body.preheader.new

omp_loop.body.preheader.new:                      ; preds = %omp_loop.body.preheader
  %unroll_iter = and i64 %5, -8
  br label %omp_loop.body

omp_loop.exit.loopexit.unr-lcssa:                 ; preds = %omp_loop.body
  %lcmp.mod.not = icmp eq i64 %xtraiter, 0
  br i1 %lcmp.mod.not, label %omp_loop.exit, label %omp_loop.body.epil.preheader

omp_loop.body.epil.preheader:                     ; preds = %omp_loop.exit.loopexit.unr-lcssa, %omp_loop.body.preheader
  %omp_loop.iv12.epil.init = phi i64 [ 0, %omp_loop.body.preheader ], [ %omp_loop.next.7, %omp_loop.exit.loopexit.unr-lcssa ]
  %.epil.init = phi float [ 0.000000e+00, %omp_loop.body.preheader ], [ %36, %omp_loop.exit.loopexit.unr-lcssa ]
  %lcmp.mod13 = icmp ne i64 %xtraiter, 0
  call void @llvm.assume(i1 %lcmp.mod13)
  br label %omp_loop.body.epil

omp_loop.body.epil:                               ; preds = %omp_loop.body.epil, %omp_loop.body.epil.preheader
  %omp_loop.iv12.epil = phi i64 [ %omp_loop.next.epil, %omp_loop.body.epil ], [ %omp_loop.iv12.epil.init, %omp_loop.body.epil.preheader ]
  %7 = phi float [ %9, %omp_loop.body.epil ], [ %.epil.init, %omp_loop.body.epil.preheader ]
  %epil.iter = phi i64 [ %epil.iter.next, %omp_loop.body.epil ], [ 0, %omp_loop.body.epil.preheader ]
  %gep.epil = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12.epil
  %8 = load float, ptr %gep.epil, align 4
  %9 = fadd float %8, %7
  store float %9, ptr %1, align 4
  %omp_loop.next.epil = add nuw i64 %omp_loop.iv12.epil, 1
  %epil.iter.next = add i64 %epil.iter, 1
  %epil.iter.cmp.not = icmp eq i64 %epil.iter.next, %xtraiter
  br i1 %epil.iter.cmp.not, label %omp_loop.exit, label %omp_loop.body.epil, !llvm.loop !6

omp_loop.exit:                                    ; preds = %omp_loop.exit.loopexit.unr-lcssa, %omp_loop.body.epil, %omp.par.entry
  call void @__kmpc_for_static_fini(ptr nonnull @2, i32 %omp_global_thread_num)
  %omp_global_thread_num5 = call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_barrier(ptr nonnull @3, i32 %omp_global_thread_num5)
  store ptr %1, ptr %red.array, align 8
  %omp_global_thread_num6 = call i32 @__kmpc_global_thread_num(ptr nonnull @4)
  %reduce = call i32 @__kmpc_reduce(ptr nonnull @4, i32 %omp_global_thread_num6, i32 1, i64 8, ptr nonnull %red.array, ptr nonnull @.omp.reduction.func.1, ptr nonnull @.gomp_critical_user_.reduction.var)
  switch i32 %reduce, label %reduce.finalize [
    i32 1, label %reduce.switch.nonatomic
    i32 2, label %reduce.switch.atomic
  ]

reduce.switch.atomic:                             ; preds = %omp_loop.exit
  %10 = load float, ptr %1, align 4
  %11 = atomicrmw fadd ptr %loadgep_, float %10 monotonic, align 4
  br label %reduce.finalize

reduce.switch.nonatomic:                          ; preds = %omp_loop.exit
  %red.value.0 = load float, ptr %loadgep_, align 4
  %red.private.value.0 = load float, ptr %1, align 4
  %12 = fadd float %red.value.0, %red.private.value.0
  store float %12, ptr %loadgep_, align 4
  call void @__kmpc_end_reduce(ptr nonnull @4, i32 %omp_global_thread_num6, ptr nonnull @.gomp_critical_user_.reduction.var)
  br label %reduce.finalize

reduce.finalize:                                  ; preds = %reduce.switch.nonatomic, %reduce.switch.atomic, %omp_loop.exit
  %omp_global_thread_num7 = call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_barrier(ptr nonnull @3, i32 %omp_global_thread_num7)
  ret void

omp_loop.body:                                    ; preds = %omp_loop.body, %omp_loop.body.preheader.new
  %omp_loop.iv12 = phi i64 [ 0, %omp_loop.body.preheader.new ], [ %omp_loop.next.7, %omp_loop.body ]
  %13 = phi float [ 0.000000e+00, %omp_loop.body.preheader.new ], [ %36, %omp_loop.body ]
  %niter = phi i64 [ 0, %omp_loop.body.preheader.new ], [ %niter.next.7, %omp_loop.body ]
  %gep = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12
  %14 = load float, ptr %gep, align 4
  %15 = fadd float %14, %13
  store float %15, ptr %1, align 4
  %16 = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12
  %gep.1 = getelementptr i8, ptr %16, i64 4
  %17 = load float, ptr %gep.1, align 4
  %18 = fadd float %17, %15
  store float %18, ptr %1, align 4
  %19 = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12
  %gep.2 = getelementptr i8, ptr %19, i64 8
  %20 = load float, ptr %gep.2, align 4
  %21 = fadd float %20, %18
  store float %21, ptr %1, align 4
  %22 = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12
  %gep.3 = getelementptr i8, ptr %22, i64 12
  %23 = load float, ptr %gep.3, align 4
  %24 = fadd float %23, %21
  store float %24, ptr %1, align 4
  %25 = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12
  %gep.4 = getelementptr i8, ptr %25, i64 16
  %26 = load float, ptr %gep.4, align 4
  %27 = fadd float %26, %24
  store float %27, ptr %1, align 4
  %28 = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12
  %gep.5 = getelementptr i8, ptr %28, i64 20
  %29 = load float, ptr %gep.5, align 4
  %30 = fadd float %29, %27
  store float %30, ptr %1, align 4
  %31 = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12
  %gep.6 = getelementptr i8, ptr %31, i64 24
  %32 = load float, ptr %gep.6, align 4
  %33 = fadd float %32, %30
  store float %33, ptr %1, align 4
  %34 = getelementptr [4 x i8], ptr %invariant.gep, i64 %omp_loop.iv12
  %gep.7 = getelementptr i8, ptr %34, i64 28
  %35 = load float, ptr %gep.7, align 4
  %36 = fadd float %35, %33
  store float %36, ptr %1, align 4
  %omp_loop.next.7 = add nuw i64 %omp_loop.iv12, 8
  %niter.next.7 = add i64 %niter, 8
  %niter.ncmp.7 = icmp eq i64 %niter.next.7, %unroll_iter
  br i1 %niter.ncmp.7, label %omp_loop.exit.loopexit.unr-lcssa, label %omp_loop.body
}

; Function Attrs: nounwind
define float @zl_nucleo_4(ptr %0, ptr %1) local_unnamed_addr #0 {
entry:
  %structArg = alloca { ptr, ptr, ptr }, align 8
  %.reloaded = alloca { ptr, ptr, i64, [1 x i64], [1 x i64] }, align 8
  %.reloaded8 = alloca { ptr, ptr, i64, [1 x i64], [1 x i64] }, align 8
  %2 = alloca float, align 4
  store float 0.000000e+00, ptr %2, align 4
  store ptr %0, ptr %.reloaded, align 8
  %.fca.1.gep4 = getelementptr inbounds nuw i8, ptr %.reloaded, i64 8
  store ptr %0, ptr %.fca.1.gep4, align 8
  %.fca.2.gep6 = getelementptr inbounds nuw i8, ptr %.reloaded, i64 16
  store i64 0, ptr %.fca.2.gep6, align 8
  %.fca.3.0.gep8 = getelementptr inbounds nuw i8, ptr %.reloaded, i64 24
  store i64 1000000, ptr %.fca.3.0.gep8, align 8
  %.fca.4.0.gep10 = getelementptr inbounds nuw i8, ptr %.reloaded, i64 32
  store i64 1, ptr %.fca.4.0.gep10, align 8
  store ptr %1, ptr %.reloaded8, align 8
  %.fca.1.gep = getelementptr inbounds nuw i8, ptr %.reloaded8, i64 8
  store ptr %1, ptr %.fca.1.gep, align 8
  %.fca.2.gep = getelementptr inbounds nuw i8, ptr %.reloaded8, i64 16
  store i64 0, ptr %.fca.2.gep, align 8
  %.fca.3.0.gep = getelementptr inbounds nuw i8, ptr %.reloaded8, i64 24
  store i64 1000000, ptr %.fca.3.0.gep, align 8
  %.fca.4.0.gep = getelementptr inbounds nuw i8, ptr %.reloaded8, i64 32
  store i64 1, ptr %.fca.4.0.gep, align 8
  store ptr %.reloaded, ptr %structArg, align 8
  %gep_.reloaded8 = getelementptr inbounds nuw i8, ptr %structArg, i64 8
  store ptr %.reloaded8, ptr %gep_.reloaded8, align 8
  %gep_ = getelementptr inbounds nuw i8, ptr %structArg, i64 16
  store ptr %2, ptr %gep_, align 8
  call void (ptr, i32, ptr, ...) @__kmpc_fork_call(ptr nonnull @1, i32 1, ptr nonnull @zl_nucleo_4..omp_par, ptr nonnull %structArg)
  %3 = load float, ptr %2, align 4
  %4 = fadd float %3, 0.000000e+00
  ret float %4
}

; Function Attrs: nounwind
define internal void @zl_nucleo_4..omp_par(ptr noalias nofree readnone captures(none) %tid.addr, ptr noalias nofree readnone captures(none) %zero.addr, ptr nofree readonly captures(none) %0) #0 {
omp.par.entry:
  %loadgep_.reloaded = load ptr, ptr %0, align 8, !align !1
  %gep_.reloaded8 = getelementptr i8, ptr %0, i64 8
  %loadgep_.reloaded8 = load ptr, ptr %gep_.reloaded8, align 8, !align !1
  %gep_ = getelementptr i8, ptr %0, i64 16
  %loadgep_ = load ptr, ptr %gep_, align 8, !align !4
  %p.lastiter = alloca i32, align 4
  %p.lowerbound = alloca i64, align 8
  %p.upperbound = alloca i64, align 8
  %p.stride = alloca i64, align 8
  %1 = alloca float, align 4
  %red.array = alloca [1 x ptr], align 8
  %.elt1 = getelementptr inbounds nuw i8, ptr %loadgep_.reloaded, i64 8
  %.unpack2 = load ptr, ptr %.elt1, align 8
  %.elt12 = getelementptr inbounds nuw i8, ptr %loadgep_.reloaded8, i64 8
  %.unpack13 = load ptr, ptr %.elt12, align 8
  store float 0.000000e+00, ptr %1, align 4
  store i64 0, ptr %p.lowerbound, align 8
  store i64 999999, ptr %p.upperbound, align 8
  store i64 1, ptr %p.stride, align 8
  %omp_global_thread_num = tail call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_for_static_init_8u(ptr nonnull @2, i32 %omp_global_thread_num, i32 34, ptr nonnull %p.lastiter, ptr nonnull %p.lowerbound, ptr nonnull %p.upperbound, ptr nonnull %p.stride, i64 1, i64 0)
  %2 = load i64, ptr %p.lowerbound, align 8
  %3 = load i64, ptr %p.upperbound, align 8
  %reass.sub = sub i64 %3, %2
  %omp_loop.cmp22.not = icmp eq i64 %reass.sub, -1
  br i1 %omp_loop.cmp22.not, label %omp_loop.exit, label %omp_loop.body.preheader

omp_loop.body.preheader:                          ; preds = %omp.par.entry
  %4 = add i64 %3, 1
  %5 = sub i64 %4, %2
  %xtraiter = and i64 %5, 7
  %6 = icmp ult i64 %reass.sub, 7
  br i1 %6, label %omp_loop.body.epil.preheader, label %omp_loop.body.preheader.new

omp_loop.body.preheader.new:                      ; preds = %omp_loop.body.preheader
  %unroll_iter = and i64 %5, -8
  br label %omp_loop.body

omp_loop.exit.loopexit.unr-lcssa:                 ; preds = %omp_loop.body
  %lcmp.mod.not = icmp eq i64 %xtraiter, 0
  br i1 %lcmp.mod.not, label %omp_loop.exit, label %omp_loop.body.epil.preheader

omp_loop.body.epil.preheader:                     ; preds = %omp_loop.exit.loopexit.unr-lcssa, %omp_loop.body.preheader
  %omp_loop.iv23.epil.init = phi i64 [ 0, %omp_loop.body.preheader ], [ %omp_loop.next.7, %omp_loop.exit.loopexit.unr-lcssa ]
  %.epil.init = phi float [ 0.000000e+00, %omp_loop.body.preheader ], [ %74, %omp_loop.exit.loopexit.unr-lcssa ]
  %lcmp.mod24 = icmp ne i64 %xtraiter, 0
  call void @llvm.assume(i1 %lcmp.mod24)
  br label %omp_loop.body.epil

omp_loop.body.epil:                               ; preds = %omp_loop.body.epil, %omp_loop.body.epil.preheader
  %omp_loop.iv23.epil = phi i64 [ %omp_loop.next.epil, %omp_loop.body.epil ], [ %omp_loop.iv23.epil.init, %omp_loop.body.epil.preheader ]
  %7 = phi float [ %14, %omp_loop.body.epil ], [ %.epil.init, %omp_loop.body.epil.preheader ]
  %epil.iter = phi i64 [ %epil.iter.next, %omp_loop.body.epil ], [ 0, %omp_loop.body.epil.preheader ]
  %8 = add i64 %omp_loop.iv23.epil, %2
  %9 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %8
  %10 = load float, ptr %9, align 4
  %11 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %8
  %12 = load float, ptr %11, align 4
  %13 = fmul float %10, %12
  %14 = fadd float %7, %13
  store float %14, ptr %1, align 4
  %omp_loop.next.epil = add nuw i64 %omp_loop.iv23.epil, 1
  %epil.iter.next = add i64 %epil.iter, 1
  %epil.iter.cmp.not = icmp eq i64 %epil.iter.next, %xtraiter
  br i1 %epil.iter.cmp.not, label %omp_loop.exit, label %omp_loop.body.epil, !llvm.loop !7

omp_loop.exit:                                    ; preds = %omp_loop.exit.loopexit.unr-lcssa, %omp_loop.body.epil, %omp.par.entry
  call void @__kmpc_for_static_fini(ptr nonnull @2, i32 %omp_global_thread_num)
  %omp_global_thread_num5 = call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_barrier(ptr nonnull @3, i32 %omp_global_thread_num5)
  store ptr %1, ptr %red.array, align 8
  %omp_global_thread_num6 = call i32 @__kmpc_global_thread_num(ptr nonnull @4)
  %reduce = call i32 @__kmpc_reduce(ptr nonnull @4, i32 %omp_global_thread_num6, i32 1, i64 8, ptr nonnull %red.array, ptr nonnull @.omp.reduction.func.2, ptr nonnull @.gomp_critical_user_.reduction.var)
  switch i32 %reduce, label %reduce.finalize [
    i32 1, label %reduce.switch.nonatomic
    i32 2, label %reduce.switch.atomic
  ]

reduce.switch.atomic:                             ; preds = %omp_loop.exit
  %15 = load float, ptr %1, align 4
  %16 = atomicrmw fadd ptr %loadgep_, float %15 monotonic, align 4
  br label %reduce.finalize

reduce.switch.nonatomic:                          ; preds = %omp_loop.exit
  %red.value.0 = load float, ptr %loadgep_, align 4
  %red.private.value.0 = load float, ptr %1, align 4
  %17 = fadd float %red.value.0, %red.private.value.0
  store float %17, ptr %loadgep_, align 4
  call void @__kmpc_end_reduce(ptr nonnull @4, i32 %omp_global_thread_num6, ptr nonnull @.gomp_critical_user_.reduction.var)
  br label %reduce.finalize

reduce.finalize:                                  ; preds = %reduce.switch.nonatomic, %reduce.switch.atomic, %omp_loop.exit
  %omp_global_thread_num7 = call i32 @__kmpc_global_thread_num(ptr nonnull @1)
  call void @__kmpc_barrier(ptr nonnull @3, i32 %omp_global_thread_num7)
  ret void

omp_loop.body:                                    ; preds = %omp_loop.body, %omp_loop.body.preheader.new
  %omp_loop.iv23 = phi i64 [ 0, %omp_loop.body.preheader.new ], [ %omp_loop.next.7, %omp_loop.body ]
  %18 = phi float [ 0.000000e+00, %omp_loop.body.preheader.new ], [ %74, %omp_loop.body ]
  %niter = phi i64 [ 0, %omp_loop.body.preheader.new ], [ %niter.next.7, %omp_loop.body ]
  %19 = add i64 %omp_loop.iv23, %2
  %20 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %19
  %21 = load float, ptr %20, align 4
  %22 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %19
  %23 = load float, ptr %22, align 4
  %24 = fmul float %21, %23
  %25 = fadd float %18, %24
  store float %25, ptr %1, align 4
  %omp_loop.next = or disjoint i64 %omp_loop.iv23, 1
  %26 = add i64 %omp_loop.next, %2
  %27 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %26
  %28 = load float, ptr %27, align 4
  %29 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %26
  %30 = load float, ptr %29, align 4
  %31 = fmul float %28, %30
  %32 = fadd float %25, %31
  store float %32, ptr %1, align 4
  %omp_loop.next.1 = or disjoint i64 %omp_loop.iv23, 2
  %33 = add i64 %omp_loop.next.1, %2
  %34 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %33
  %35 = load float, ptr %34, align 4
  %36 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %33
  %37 = load float, ptr %36, align 4
  %38 = fmul float %35, %37
  %39 = fadd float %32, %38
  store float %39, ptr %1, align 4
  %omp_loop.next.2 = or disjoint i64 %omp_loop.iv23, 3
  %40 = add i64 %omp_loop.next.2, %2
  %41 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %40
  %42 = load float, ptr %41, align 4
  %43 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %40
  %44 = load float, ptr %43, align 4
  %45 = fmul float %42, %44
  %46 = fadd float %39, %45
  store float %46, ptr %1, align 4
  %omp_loop.next.3 = or disjoint i64 %omp_loop.iv23, 4
  %47 = add i64 %omp_loop.next.3, %2
  %48 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %47
  %49 = load float, ptr %48, align 4
  %50 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %47
  %51 = load float, ptr %50, align 4
  %52 = fmul float %49, %51
  %53 = fadd float %46, %52
  store float %53, ptr %1, align 4
  %omp_loop.next.4 = or disjoint i64 %omp_loop.iv23, 5
  %54 = add i64 %omp_loop.next.4, %2
  %55 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %54
  %56 = load float, ptr %55, align 4
  %57 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %54
  %58 = load float, ptr %57, align 4
  %59 = fmul float %56, %58
  %60 = fadd float %53, %59
  store float %60, ptr %1, align 4
  %omp_loop.next.5 = or disjoint i64 %omp_loop.iv23, 6
  %61 = add i64 %omp_loop.next.5, %2
  %62 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %61
  %63 = load float, ptr %62, align 4
  %64 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %61
  %65 = load float, ptr %64, align 4
  %66 = fmul float %63, %65
  %67 = fadd float %60, %66
  store float %67, ptr %1, align 4
  %omp_loop.next.6 = or disjoint i64 %omp_loop.iv23, 7
  %68 = add i64 %omp_loop.next.6, %2
  %69 = getelementptr inbounds nuw [4 x i8], ptr %.unpack2, i64 %68
  %70 = load float, ptr %69, align 4
  %71 = getelementptr inbounds nuw [4 x i8], ptr %.unpack13, i64 %68
  %72 = load float, ptr %71, align 4
  %73 = fmul float %70, %72
  %74 = fadd float %67, %73
  store float %74, ptr %1, align 4
  %omp_loop.next.7 = add nuw i64 %omp_loop.iv23, 8
  %niter.next.7 = add i64 %niter, 8
  %niter.ncmp.7 = icmp eq i64 %niter.next.7, %unroll_iter
  br i1 %niter.ncmp.7, label %omp_loop.exit.loopexit.unr-lcssa, label %omp_loop.body
}

; Function Attrs: nounwind
declare void @__kmpc_for_static_init_8u(ptr, i32, i32, ptr, ptr, ptr, ptr, i64, i64) local_unnamed_addr #0

; Function Attrs: nounwind
declare void @__kmpc_for_static_fini(ptr, i32) local_unnamed_addr #0

; Function Attrs: nounwind
declare i32 @__kmpc_global_thread_num(ptr) local_unnamed_addr #0

; Function Attrs: convergent nounwind
declare void @__kmpc_barrier(ptr, i32) local_unnamed_addr #1

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none, target_mem: none)
define internal void @.omp.reduction.func(ptr nofree readonly captures(none) %0, ptr nofree readonly captures(none) %1) #2 {
  %3 = load ptr, ptr %0, align 8
  %4 = load i32, ptr %3, align 4
  %5 = load ptr, ptr %1, align 8
  %6 = load i32, ptr %5, align 4
  %7 = add i32 %6, %4
  store i32 %7, ptr %3, align 4
  ret void
}

; Function Attrs: convergent nounwind
declare i32 @__kmpc_reduce(ptr, i32, i32, i64, ptr, ptr, ptr) local_unnamed_addr #1

; Function Attrs: convergent nounwind
declare void @__kmpc_end_reduce(ptr, i32, ptr) local_unnamed_addr #1

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none, target_mem: none)
define internal void @.omp.reduction.func.1(ptr nofree readonly captures(none) %0, ptr nofree readonly captures(none) %1) #2 {
  %3 = load ptr, ptr %0, align 8
  %4 = load float, ptr %3, align 4
  %5 = load ptr, ptr %1, align 8
  %6 = load float, ptr %5, align 4
  %7 = fadd float %4, %6
  store float %7, ptr %3, align 4
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none, target_mem: none)
define internal void @.omp.reduction.func.2(ptr nofree readonly captures(none) %0, ptr nofree readonly captures(none) %1) #2 {
  %3 = load ptr, ptr %0, align 8
  %4 = load float, ptr %3, align 4
  %5 = load ptr, ptr %1, align 8
  %6 = load float, ptr %5, align 4
  %7 = fadd float %4, %6
  store float %7, ptr %3, align 4
  ret void
}

; Function Attrs: nounwind
declare !callback !8 void @__kmpc_fork_call(ptr, i32, ptr, ...) local_unnamed_addr #0

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write)
declare void @llvm.assume(i1 noundef) #3

attributes #0 = { nounwind }
attributes #1 = { convergent nounwind }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none, target_mem: none) }
attributes #3 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = !{i64 8}
!2 = distinct !{!2, !3}
!3 = !{!"llvm.loop.unroll.disable"}
!4 = !{i64 4}
!5 = distinct !{!5, !3}
!6 = distinct !{!6, !3}
!7 = distinct !{!7, !3}
!8 = !{!9}
!9 = !{i64 2, i64 -1, i64 -1, i1 true}
