module {
  %rna = "ribo.rna"() {sequence = "GCGC"} : () -> tensor<?xi8>
  %fold = "ribo.fold"(%rna) {topology = #ribo.nested, min_loop = 0 : i64, pair_score = 1 : i64,
    temperature_k = 0.0 : f64, sodium_m = 0.1 : f64, magnesium_m = 0.001 : f64,
    parameter_set = "Turner2023"} : (tensor<?xi8>) -> i32
}
