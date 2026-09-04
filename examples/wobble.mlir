module {
  %rna = "ribo.rna"() {sequence = "GU"} : () -> tensor<?xi8>
  %fold = "ribo.fold"(%rna) {topology = "nested", min_loop = 0 : i64,
                              pair_score = 1 : i64, wobble_score = 2 : i64} : (tensor<?xi8>) -> i64
}
