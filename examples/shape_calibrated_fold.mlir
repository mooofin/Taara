module {
  %rna = "ribo.rna"() {sequence = "AU"} : () -> tensor<?xi8>
  %shape = "ribo.constraint"(%rna) {kind = "shape",
    reactivity = dense<[0.1, 0.1]> : tensor<2xf32>} : (tensor<?xi8>) -> tensor<2xf32>
  %fold = "ribo.fold"(%rna, %shape) {topology = #ribo.nested, min_loop = 0 : i64,
    pair_score = 1 : i64, shape_slope = 2.0 : f64, shape_intercept = -0.1 : f64}
    : (tensor<?xi8>, tensor<2xf32>) -> i64
}
