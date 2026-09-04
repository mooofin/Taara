module {
  %rna = "ribo.rna"() {sequence = "GCGAAAGCUC"} : () -> tensor<?xi8>
  "ribo.kissing_candidate"(%rna) {stem1_start = 4 : i64, stem1_end = 2 : i64, stem1_length = 1 : i64,
    stem2_start = 1 : i64, stem2_end = 8 : i64, stem2_length = 1 : i64, stem3_start = 3 : i64,
    stem3_end = 7 : i64, stem3_length = 1 : i64, loop1_min = 1 : i64, loop2_min = 0 : i64,
    loop3_min = 1 : i64, loop4_min = 0 : i64, loop5_min = 1 : i64,
    shared_stem = 4 : i64, shared_stem_score_once = false, weight = 3 : i64} : (tensor<?xi8>) -> ()
}
