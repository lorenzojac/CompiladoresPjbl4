module {
  llvm.func @zl_nucleo_1(%arg0: !llvm.ptr, %arg1: !llvm.ptr, %arg2: !llvm.ptr) attributes {passthrough = ["nounwind"]} {
    %0 = llvm.mlir.poison : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>
    %1 = llvm.insertvalue %arg2, %0[0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %2 = llvm.insertvalue %arg2, %1[1] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %3 = llvm.mlir.constant(0 : index) : i64
    %4 = llvm.insertvalue %3, %2[2] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %5 = llvm.mlir.constant(1024 : index) : i64
    %6 = llvm.insertvalue %5, %4[3, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %7 = llvm.mlir.constant(1 : index) : i64
    %8 = llvm.insertvalue %7, %6[4, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %9 = llvm.mlir.poison : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>
    %10 = llvm.insertvalue %arg1, %9[0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %11 = llvm.insertvalue %arg1, %10[1] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %12 = llvm.mlir.constant(0 : index) : i64
    %13 = llvm.insertvalue %12, %11[2] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %14 = llvm.mlir.constant(1024 : index) : i64
    %15 = llvm.insertvalue %14, %13[3, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %16 = llvm.mlir.constant(1 : index) : i64
    %17 = llvm.insertvalue %16, %15[4, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %18 = llvm.mlir.poison : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>
    %19 = llvm.insertvalue %arg0, %18[0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %20 = llvm.insertvalue %arg0, %19[1] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %21 = llvm.mlir.constant(0 : index) : i64
    %22 = llvm.insertvalue %21, %20[2] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %23 = llvm.mlir.constant(1024 : index) : i64
    %24 = llvm.insertvalue %23, %22[3, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %25 = llvm.mlir.constant(1 : index) : i64
    %26 = llvm.insertvalue %25, %24[4, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %27 = llvm.mlir.constant(0 : index) : i64
    %28 = llvm.mlir.constant(1024 : index) : i64
    %29 = llvm.mlir.constant(1 : index) : i64
    omp.parallel {
      omp.wsloop {
        omp.loop_nest (%arg3) : i64 = (%27) to (%28) step (%29) {
          %30 = llvm.extractvalue %26[1] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
          %31 = llvm.getelementptr inbounds|nuw %30[%arg3] : (!llvm.ptr, i64) -> !llvm.ptr, f32
          %32 = llvm.load %31 : !llvm.ptr -> f32
          %33 = llvm.extractvalue %17[1] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
          %34 = llvm.getelementptr inbounds|nuw %33[%arg3] : (!llvm.ptr, i64) -> !llvm.ptr, f32
          %35 = llvm.load %34 : !llvm.ptr -> f32
          %36 = llvm.fadd %32, %35 : f32
          %37 = llvm.extractvalue %8[1] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
          %38 = llvm.getelementptr inbounds|nuw %37[%arg3] : (!llvm.ptr, i64) -> !llvm.ptr, f32
          llvm.store %36, %38 : f32, !llvm.ptr
          omp.yield
        }
      }
      omp.terminator
    }
    llvm.return
  }
  omp.declare_reduction @__scf_reduction : f32 init {
  ^bb0(%arg0: f32):
    %0 = llvm.mlir.constant(0.000000e+00 : f32) : f32
    omp.yield(%0 : f32)
  } combiner {
  ^bb0(%arg0: f32, %arg1: f32):
    %0 = llvm.fadd %arg0, %arg1 : f32
    omp.yield(%0 : f32)
  } atomic {
  ^bb0(%arg0: !llvm.ptr, %arg1: !llvm.ptr):
    %0 = llvm.load %arg1 : !llvm.ptr -> f32
    %1 = llvm.atomicrmw fadd %arg0, %0 monotonic : !llvm.ptr, f32
    omp.yield
  }
  llvm.func @zl_nucleo_2(%arg0: !llvm.ptr) -> f32 attributes {passthrough = ["nounwind"]} {
    %0 = llvm.mlir.poison : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)>
    %1 = llvm.insertvalue %arg0, %0[0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %2 = llvm.insertvalue %arg0, %1[1] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %3 = llvm.mlir.constant(0 : index) : i64
    %4 = llvm.insertvalue %3, %2[2] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %5 = llvm.mlir.constant(1024 : index) : i64
    %6 = llvm.insertvalue %5, %4[3, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %7 = llvm.mlir.constant(1 : index) : i64
    %8 = llvm.insertvalue %7, %6[4, 0] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
    %9 = llvm.mlir.constant(0.000000e+00 : f32) : f32
    %10 = llvm.mlir.constant(0 : index) : i64
    %11 = llvm.mlir.constant(1024 : index) : i64
    %12 = llvm.mlir.constant(1 : index) : i64
    %13 = llvm.mlir.constant(1 : i64) : i64
    %14 = llvm.alloca %13 x f32 : (i64) -> !llvm.ptr
    llvm.store %9, %14 : f32, !llvm.ptr
    omp.parallel {
      omp.wsloop reduction(@__scf_reduction %14 -> %arg1 : !llvm.ptr) {
        omp.loop_nest (%arg2) : i64 = (%10) to (%11) step (%12) {
          %17 = llvm.extractvalue %8[1] : !llvm.struct<(ptr, ptr, i64, array<1 x i64>, array<1 x i64>)> 
          %18 = llvm.getelementptr inbounds|nuw %17[%arg2] : (!llvm.ptr, i64) -> !llvm.ptr, f32
          %19 = llvm.load %18 : !llvm.ptr -> f32
          %20 = llvm.load %arg1 : !llvm.ptr -> f32
          %21 = llvm.fadd %20, %19 : f32
          llvm.store %21, %arg1 : f32, !llvm.ptr
          omp.yield
        }
      }
      omp.terminator
    }
    %15 = llvm.load %14 : !llvm.ptr -> f32
    %16 = llvm.fadd %15, %9 : f32
    llvm.return %16 : f32
  }
}

