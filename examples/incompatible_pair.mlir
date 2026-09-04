module {
  %rna = "ribo.rna"() {sequence = "GG"} : () -> tensor<?xi8>
  "ribo.structure"(%rna) {dot_bracket = "()", topology = "nested"} : (tensor<?xi8>) -> ()
}
