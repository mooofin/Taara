module {
  %rna0 = "ribo.rna"() {sequence = "AUGC"} : () -> tensor<?xi8>
  %rna1 = "ribo.rna"() {sequence = "AUGC"} : () -> tensor<?xi8>
  %shape = "ribo.constraint"(%rna1) {kind = "shape",
      reactivity = dense<[0.2, 1.8, 0.0, 2.4]> : tensor<4xf32>}
      : (tensor<?xi8>) -> tensor<4xf32>
  %fold = "ribo.fold"(%rna0, %shape) {topology = "nested", min_loop = 0 : i64,
      pair_score = 1 : i64} : (tensor<?xi8>, tensor<4xf32>) -> i64
}
