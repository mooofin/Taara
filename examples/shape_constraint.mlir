module {
  %rna = "ribo.rna"() {sequence = "AUGC"} : () -> tensor<?xi8>
  %shape = "ribo.constraint"(%rna) {kind = "shape",
      reactivity = dense<[0.2, 1.8, 0.0, 2.4]> : tensor<4xf32>}
      : (tensor<?xi8>) -> tensor<4xf32>
}
