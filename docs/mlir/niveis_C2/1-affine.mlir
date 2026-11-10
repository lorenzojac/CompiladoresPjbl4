module {
  func.func @zl_nucleo_1(%arg0: memref<1024xf32>, %arg1: memref<1024xf32>, %arg2: memref<1024xf32>) attributes {llvm.passthrough = ["nounwind"]} {
    affine.for %arg3 = 0 to 1024 step 8 {
      %0 = ub.poison : f32
      %1 = vector.transfer_read %arg0[%arg3], %0 {in_bounds = [true]} : memref<1024xf32>, vector<8xf32>
      %2 = ub.poison : f32
      %3 = vector.transfer_read %arg1[%arg3], %2 {in_bounds = [true]} : memref<1024xf32>, vector<8xf32>
      %4 = arith.addf %1, %3 : vector<8xf32>
      vector.transfer_write %4, %arg2[%arg3] {in_bounds = [true]} : vector<8xf32>, memref<1024xf32>
    }
    return
  }
  func.func @zl_nucleo_2(%arg0: memref<1024xf32>) -> f32 attributes {llvm.passthrough = ["nounwind"]} {
    %cst = arith.constant 0.000000e+00 : f32
    %cst_0 = arith.constant dense<0.000000e+00> : vector<8xf32>
    %0 = affine.for %arg1 = 0 to 1024 step 8 iter_args(%arg2 = %cst_0) -> (vector<8xf32>) {
      %2 = ub.poison : f32
      %3 = vector.transfer_read %arg0[%arg1], %2 {in_bounds = [true]} : memref<1024xf32>, vector<8xf32>
      %4 = arith.addf %arg2, %3 : vector<8xf32>
      affine.yield %4 : vector<8xf32>
    }
    %1 = vector.reduction <add>, %0 : vector<8xf32> into f32
    return %1 : f32
  }
}

