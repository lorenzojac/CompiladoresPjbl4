// Nucleos da DSL vetorial de exemplo.mac (PjBL 4, Parte B).
// Rebaixados ate LLVM IR por scripts/pipeline.sh.

// linha 4: atribuicao vetorial elemento a elemento
func.func @zl_nucleo_1(%v0: memref<1024xf32>, %v1: memref<1024xf32>, %v2: memref<1024xf32>) attributes {llvm.passthrough = ["nounwind"]} {
  affine.for %i = 0 to 1024 {
    %x0 = affine.load %v0[%i] : memref<1024xf32>
    %x1 = affine.load %v1[%i] : memref<1024xf32>
    %t0 = arith.addf %x0, %x1 : f32
    affine.store %t0, %v2[%i] : memref<1024xf32>
  }
  return
}

// linha 5: reducao somavet
func.func @zl_nucleo_2(%v0: memref<1024xf32>) -> f32 attributes {llvm.passthrough = ["nounwind"]} {
  %zero = arith.constant 0.000000e+00 : f32
  %r = affine.for %i = 0 to 1024 iter_args(%acc = %zero) -> (f32) {
    %x0 = affine.load %v0[%i] : memref<1024xf32>
    %n = arith.addf %acc, %x0 : f32
    affine.yield %n : f32
  }
  return %r : f32
}

