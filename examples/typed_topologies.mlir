module {
  %rna = "ribo.rna"() {sequence = "A"} : () -> tensor<?xi8>
  %nested = "ribo.fold"(%rna) {topology = #ribo.nested, min_loop = 0 : i64, pair_score = 1 : i64} : (tensor<?xi8>) -> i64
  %h = "ribo.fold"(%rna) {topology = #ribo.h_type_pseudoknot, min_loop = 0 : i64, stem_length = 1 : i64, pair_score = 1 : i64} : (tensor<?xi8>) -> i64
  %kiss = "ribo.fold"(%rna) {topology = #ribo.kissing_hairpin, min_loop = 0 : i64, pair_score = 1 : i64} : (tensor<?xi8>) -> i64
  %bounded = "ribo.fold"(%rna) {topology = #ribo.bounded_crossing, max_crossing_depth = 1 : i64, state_width = 2 : i64,
                               min_loop = 0 : i64, pair_score = 1 : i64} : (tensor<?xi8>) -> i64
  %general = "ribo.fold"(%rna) {topology = #ribo.general_crossing, min_loop = 0 : i64, pair_score = 1 : i64} : (tensor<?xi8>) -> i64
}
