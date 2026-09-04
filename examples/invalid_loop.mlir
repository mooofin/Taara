module {
  %rna = "ribo.rna"() {sequence = "GCGAAAGCUC"} : () -> tensor<?xi8>
  %fold = "ribo.fold"(%rna) {topology = "nested", min_loop = -1 : i64, pair_score = 1 : i64} : (tensor<?xi8>) -> i64
}
