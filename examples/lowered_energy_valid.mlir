module {
  "ribo.lowered_fold"() {topology = #ribo.nested, strategy = "nested_triangular_dp",
    runtime = "ribo-runtime.foldNested", state_rank = 2 : i64, state_variables = "i,j",
    dependencies = "interval", storage = "upper_triangular(i,j)", time = "O(n^3)",
    space = "O(n^2)", optimality = "exact", temperature_k = 310.15 : f64,
    sodium_m = 0.1 : f64, magnesium_m = 0.001 : f64, parameter_set = "Turner2023"} : () -> ()
}
