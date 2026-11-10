module {
  func.func @zl_nucleo_1(%arg0: memref<1024xf32>, %arg1: memref<1024xf32>, %arg2: memref<1024xf32>) attributes {llvm.passthrough = ["nounwind"]} {
    %c0 = arith.constant 0 : index
    %c1024 = arith.constant 1024 : index
    %c1 = arith.constant 1 : index
    omp.parallel {
      omp.wsloop {
        omp.loop_nest (%arg3) : index = (%c0) to (%c1024) step (%c1) {
          %0 = memref.load %arg0[%arg3] : memref<1024xf32>
          %1 = memref.load %arg1[%arg3] : memref<1024xf32>
          %2 = arith.addf %0, %1 : f32
          memref.store %2, %arg2[%arg3] : memref<1024xf32>
          omp.yield
        }
      }
      omp.terminator
    }
    return
  }
  omp.declare_reduction @__scf_reduction : f32 init {
  ^bb0(%arg0: f32):
    %0 = llvm.mlir.constant(0.000000e+00 : f32) : f32
    omp.yield(%0 : f32)
  } combiner {
  ^bb0(%arg0: f32, %arg1: f32):
    %0 = arith.addf %arg0, %arg1 : f32
    omp.yield(%0 : f32)
  } atomic {
  ^bb0(%arg0: !llvm.ptr, %arg1: !llvm.ptr):
    %0 = llvm.load %arg1 : !llvm.ptr -> f32
    %1 = llvm.atomicrmw fadd %arg0, %0 monotonic : !llvm.ptr, f32
    omp.yield
  }
  func.func @zl_nucleo_2(%arg0: memref<1024xf32>) -> f32 attributes {llvm.passthrough = ["nounwind"]} {
    %cst = arith.constant 0.000000e+00 : f32
    %c0 = arith.constant 0 : index
    %c1024 = arith.constant 1024 : index
    %c1 = arith.constant 1 : index
    %0 = llvm.mlir.constant(1 : i64) : i64
    %1 = llvm.alloca %0 x f32 : (i64) -> !llvm.ptr
    llvm.store %cst, %1 : f32, !llvm.ptr
    omp.parallel {
      omp.wsloop reduction(@__scf_reduction %1 -> %arg1 : !llvm.ptr) {
        omp.loop_nest (%arg2) : index = (%c0) to (%c1024) step (%c1) {
          %4 = memref.load %arg0[%arg2] : memref<1024xf32>
          %5 = llvm.load %arg1 : !llvm.ptr -> f32
          %6 = arith.addf %5, %4 : f32
          llvm.store %6, %arg1 : f32, !llvm.ptr
          omp.yield
        }
      }
      omp.terminator
    }
    %2 = llvm.load %1 : !llvm.ptr -> f32
    %3 = arith.addf %2, %cst : f32
    return %3 : f32
  }
}

