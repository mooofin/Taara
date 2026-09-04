module {
  %rna = "ribo.rna"() {sequence = "GCAGCAAGCAGC"} : () -> tensor<?xi8>
  "ribo.structure"(%rna) {dot_bracket = "([.....])...", topology = #ribo.kissing_hairpin} : (tensor<?xi8>) -> ()
}
