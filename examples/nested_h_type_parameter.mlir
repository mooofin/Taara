module {
  %rna = "ribo.rna"() {sequence = "GCGC"} : () -> tensor<?xi8>
  %fold = "ribo.fold"(%rna) {topology = #ribo.nested, min_loop = 1 : i64, stem_length = 1 : i64, pair_score = 1 : i64} : (tensor<?xi8>) -> i64
}
