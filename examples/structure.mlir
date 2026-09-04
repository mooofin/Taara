module {
  %rna = "ribo.rna"() {sequence = "GCGAAAACGC"} : () -> tensor<?xi8>
  "ribo.structure"(%rna) {dot_bracket = "(((....)))", topology = "nested"} : (tensor<?xi8>) -> ()
}

