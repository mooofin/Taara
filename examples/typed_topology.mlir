module {
  %rna = "ribo.rna"() {sequence = "AU"} : () -> tensor<?xi8>
  %fold = "ribo.fold"(%rna) {topology = #ribo.nested, min_loop = 0 : i64,
                              pair_score = 1 : i64} : (tensor<?xi8>) -> i64
}
