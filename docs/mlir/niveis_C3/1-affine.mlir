module {
  func.func @zl_nucleo_1(%arg0: memref<1024xf32>, %arg1: memref<1024xf32>, %arg2: memref<1024xf32>) attributes {llvm.passthrough = ["nounwind"]} {
    affine.parallel (%arg3) = (0) to (1024) {
      %0 = affine.load %arg0[%arg3] : memref<1024xf32>
      %1 = affine.load %arg1[%arg3] : memref<1024xf32>
      %2 = arith.addf %0, %1 : f32
      affine.store %2, %arg2[%arg3] : memref<1024xf32>
    }
    return
  }
  func.func @zl_nucleo_2(%arg0: memref<1024xf32>) -> f32 attributes {llvm.passthrough = ["nounwind"]} {
    %cst = arith.constant 0.000000e+00 : f32
    %0 = affine.parallel (%arg1) = (0) to (1024) reduce ("addf") -> (f32) {
      %2 = affine.load %arg0[%arg1] : memref<1024xf32>
      affine.yield %2 : f32
    }
    %1 = arith.addf %cst, %0 : f32
    return %1 : f32
  }
}

