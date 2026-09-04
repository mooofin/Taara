module {
  %rna = "ribo.rna"() {sequence = "GGGAAACCCUUU"} : () -> tensor<?xi8>
  %fold = "ribo.fold"(%rna) {topology = "kissing_hairpin", min_loop = 3 : i64, pair_score = 1 : i64} : (tensor<?xi8>) -> i64
}
