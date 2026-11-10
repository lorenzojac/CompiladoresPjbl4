module {
  func.func @zl_nucleo_1(%arg0: memref<1024xf32>, %arg1: memref<1024xf32>, %arg2: memref<1024xf32>) attributes {llvm.passthrough = ["nounwind"]} {
    %c0 = arith.constant 0 : index
    %c1024 = arith.constant 1024 : index
    %c1 = arith.constant 1 : index
    scf.for %arg3 = %c0 to %c1024 step %c1 {
      %0 = memref.load %arg0[%arg3] : memref<1024xf32>
      %1 = memref.load %arg1[%arg3] : memref<1024xf32>
      %2 = arith.addf %0, %1 : f32
      memref.store %2, %arg2[%arg3] : memref<1024xf32>
    }
    return
  }
  func.func @zl_nucleo_2(%arg0: memref<1024xf32>) -> f32 attributes {llvm.passthrough = ["nounwind"]} {
    %cst = arith.constant 0.000000e+00 : f32
    %c0 = arith.constant 0 : index
    %c1024 = arith.constant 1024 : index
    %c1 = arith.constant 1 : index
    %0 = scf.for %arg1 = %c0 to %c1024 step %c1 iter_args(%arg2 = %cst) -> (f32) {
      %1 = memref.load %arg0[%arg1] : memref<1024xf32>
      %2 = arith.addf %arg2, %1 : f32
      scf.yield %2 : f32
    }
    return %0 : f32
  }
}

