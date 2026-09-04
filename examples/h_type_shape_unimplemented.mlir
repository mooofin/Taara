module {
  %rna = "ribo.rna"() {sequence = "GCAGCAAGCAGC"} : () -> tensor<?xi8>
  %shape = "ribo.constraint"(%rna) {kind = "shape", reactivity = dense<[0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]> : tensor<12xf32>} : (tensor<?xi8>) -> tensor<?xf32>
  %fold = "ribo.fold"(%rna, %shape) {topology = #ribo.h_type_pseudoknot, min_loop = 1 : i64, stem_length = 2 : i64, pair_score = 1 : i64} : (tensor<?xi8>, tensor<?xf32>) -> i64
}
