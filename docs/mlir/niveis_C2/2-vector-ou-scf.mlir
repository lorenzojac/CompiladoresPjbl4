module {
  func.func @zl_nucleo_1(%arg0: memref<1024xf32>, %arg1: memref<1024xf32>, %arg2: memref<1024xf32>) attributes {llvm.passthrough = ["nounwind"]} {
    %0 = ub.poison : f32
    %c0 = arith.constant 0 : index
    %c1024 = arith.constant 1024 : index
    %c8 = arith.constant 8 : index
    scf.for %arg3 = %c0 to %c1024 step %c8 {
      %1 = vector.transfer_read %arg0[%arg3], %0 {in_bounds = [true]} : memref<1024xf32>, vector<8xf32>
      %2 = vector.transfer_read %arg1[%arg3], %0 {in_bounds = [true]} : memref<1024xf32>, vector<8xf32>
      %3 = arith.addf %1, %2 : vector<8xf32>
      vector.transfer_write %3, %arg2[%arg3] {in_bounds = [true]} : vector<8xf32>, memref<1024xf32>
    }
    return
  }
  func.func @zl_nucleo_2(%arg0: memref<1024xf32>) -> f32 attributes {llvm.passthrough = ["nounwind"]} {
    %0 = ub.poison : f32
    %cst = arith.constant dense<0.000000e+00> : vector<8xf32>
    %c0 = arith.constant 0 : index
    %c1024 = arith.constant 1024 : index
    %c8 = arith.constant 8 : index
    %1 = scf.for %arg1 = %c0 to %c1024 step %c8 iter_args(%arg2 = %cst) -> (vector<8xf32>) {
      %3 = vector.transfer_read %arg0[%arg1], %0 {in_bounds = [true]} : memref<1024xf32>, vector<8xf32>
      %4 = arith.addf %arg2, %3 : vector<8xf32>
      scf.yield %4 : vector<8xf32>
    }
    %2 = vector.reduction <add>, %1 : vector<8xf32> into f32
    return %2 : f32
  }
}

