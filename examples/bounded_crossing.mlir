module {
  %rna = "ribo.rna"() {sequence = "GCGAAAGCUC"} : () -> tensor<?xi8>
  %fold = "ribo.fold"(%rna) {topology = "bounded_crossing", max_crossing_depth = 2 : i64, state_width = 3 : i64,
                              min_loop = 3 : i64, pair_score = 1 : i64} : (tensor<?xi8>) -> i64
}
