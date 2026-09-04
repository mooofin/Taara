module {
  %rna = "ribo.rna"() {sequence = "GCAGCAAGCAGC"} : () -> tensor<?xi8>
  %fold = "ribo.fold"(%rna) {topology = #ribo.h_type_pseudoknot, min_loop = 1 : i64, gap1_min_loop = 1 : i64, gap2_min_loop = 1 : i64, gap3_min_loop = 1 : i64, stem1_length = 2 : i64, stem2_length = 1 : i64, pair_score = 1 : i64} : (tensor<?xi8>) -> i64
}
