cmake_minimum_required(VERSION 3.20)

project(tara VERSION 0.1.0 LANGUAGES CXX)

enable_testing()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
  set(CMAKE_CXX_STANDARD 26)
  message(STATUS "Using experimental C++26 mode with Clang")
else()
  set(CMAKE_CXX_STANDARD 23)
  message(STATUS "Using portable C++23 mode; this compiler does not advertise C++26")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MLIR_DIR "" CACHE PATH "Directory containing MLIRConfig.cmake")
find_package(MLIR CONFIG REQUIRED)
if(NOT MLIR_PACKAGE_VERSION AND LLVM_VERSION)
  set(MLIR_PACKAGE_VERSION "${LLVM_VERSION}")
elseif(NOT MLIR_PACKAGE_VERSION)
  set(MLIR_PACKAGE_VERSION "unknown")
endif()

message(STATUS "Using MLIR ${MLIR_PACKAGE_VERSION}")
message(STATUS "MLIR directory: ${MLIR_DIR}")

add_library(ribo-runtime src/runtime.cpp src/constraints.cpp src/lowering.cpp src/metrics.cpp src/sequence.cpp src/candidate_selection.cpp src/energy.cpp)
target_include_directories(ribo-runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(tara src/main.cpp src/kissing_candidate.cpp src/rna.cpp src/constraint_op.cpp src/lowered_fold.cpp)
add_executable(bio-llvm ALIAS tara)
target_compile_definitions(tara PRIVATE BIO_LLVM_VERSION="${PROJECT_VERSION}" BIO_LLVM_MLIR_VERSION="${MLIR_PACKAGE_VERSION}")
target_link_libraries(tara PRIVATE ribo-runtime)
add_executable(ribo-runtime-test tests/runtime_test.cpp)
target_link_libraries(ribo-runtime-test PRIVATE ribo-runtime)
add_executable(ribo-lowering-test tests/lowering_test.cpp)
target_link_libraries(ribo-lowering-test PRIVATE ribo-runtime)
add_executable(ribo-metrics-test tests/metrics_test.cpp)
target_link_libraries(ribo-metrics-test PRIVATE ribo-runtime)
add_executable(ribo-evaluate src/evaluate.cpp)
target_link_libraries(ribo-evaluate PRIVATE ribo-runtime)
add_executable(ribo-dataset-evaluate src/dataset_evaluate.cpp)
target_link_libraries(ribo-dataset-evaluate PRIVATE ribo-runtime)
add_executable(ribo-benchmark src/benchmark.cpp)
target_link_libraries(ribo-benchmark PRIVATE ribo-runtime)
install(TARGETS tara ribo-runtime ribo-evaluate ribo-dataset-evaluate ribo-benchmark RUNTIME DESTINATION bin ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/README.md DESTINATION share/doc/tara)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md DESTINATION share/doc/tara)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs/ DESTINATION share/doc/tara/docs
        FILES_MATCHING PATTERN "*.md")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/examples/ DESTINATION share/tara/examples
        FILES_MATCHING PATTERN "*.mlir" PATTERN "*.yaml")
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/examples/README.md DESTINATION share/tara/examples)
target_include_directories(tara PRIVATE ${MLIR_INCLUDE_DIRS})
target_link_directories(tara PRIVATE ${MLIR_LIBRARY_DIRS})
target_compile_definitions(tara PRIVATE ${MLIR_DEFINITIONS})

target_link_libraries(tara PRIVATE
  MLIRIR
  MLIRParser
  MLIRPass
  MLIRConvertToLLVMPass
  MLIRSupport
  MLIRArithDialect
)

if(MSVC)
  target_compile_options(tara PRIVATE /W4 /permissive-)
else()
  target_compile_options(tara PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME nested_fold_from_ir
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/nested.mlir)
set_tests_properties(nested_fold_from_ir PROPERTIES
  PASS_REGULAR_EXPRESSION "score=2 structure=..\\(.\\(...\\)\\)")

add_test(NAME tool_version
  COMMAND bio-llvm --version)
set_tests_properties(tool_version PROPERTIES
  PASS_REGULAR_EXPRESSION "Tara 0.1.0.*MLIR 24.0.0")

add_test(NAME tool_help
  COMMAND bio-llvm --help)
add_test(NAME nested_plan_json
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/nested.mlir --plan-json)
set_tests_properties(nested_plan_json PROPERTIES
  PASS_REGULAR_EXPRESSION "\\\"kind\\\":\\\"fold_plan\\\".*\\\"topology\\\":\\\"nested\\\".*\\\"supported\\\":true")
set_tests_properties(tool_help PROPERTIES
  PASS_REGULAR_EXPRESSION "usage: bio-llvm")

add_test(NAME runtime_nested_dp
  COMMAND ribo-runtime-test)
add_test(NAME nested_benchmark_smoke
  COMMAND ribo-benchmark 32 3 2)
set_tests_properties(nested_benchmark_smoke PROPERTIES
  PASS_REGULAR_EXPRESSION "kernel=nested_triangular_dp.*length=32.*score=.*runs=2")
add_test(NAME h_type_benchmark_smoke
  COMMAND ribo-benchmark --h-type 32 1 2 1 1)
set_tests_properties(h_type_benchmark_smoke PROPERTIES
  PASS_REGULAR_EXPRESSION "kernel=h_type_bounded_dp.*length=32.*stem1=2.*stem2=1.*runs=1")
add_test(NAME nested_benchmark_csv_sweep
  COMMAND ribo-benchmark --sweep 16 32 16)
set_tests_properties(nested_benchmark_csv_sweep PROPERTIES
  PASS_REGULAR_EXPRESSION "length,score,elapsed_us")
add_test(NAME nested_benchmark_rejects_bad_argument
  COMMAND ribo-benchmark definitely-not-a-length)
set_tests_properties(nested_benchmark_rejects_bad_argument PROPERTIES WILL_FAIL TRUE)
add_test(NAME nested_benchmark_rejects_trailing_argument
  COMMAND ribo-benchmark 32xyz)
set_tests_properties(nested_benchmark_rejects_trailing_argument PROPERTIES WILL_FAIL TRUE)
add_test(NAME nested_benchmark_boundary_sweep
  COMMAND ribo-benchmark --sweep 1 1 2147483647)
set_tests_properties(nested_benchmark_boundary_sweep PROPERTIES
  PASS_REGULAR_EXPRESSION "length,score,elapsed_us")
add_test(NAME lowering_policy
  COMMAND ribo-lowering-test)
add_test(NAME structure_metrics
  COMMAND ribo-metrics-test)
add_test(NAME evaluate_contacts
  COMMAND ribo-evaluate "()()" "()..")
set_tests_properties(evaluate_contacts PROPERTIES
  PASS_REGULAR_EXPRESSION "tp=1 fp=1 fn=0.*f1=0.666667")
add_test(NAME evaluate_contacts_json
  COMMAND ribo-evaluate --json "()()" "()..")
set_tests_properties(evaluate_contacts_json PROPERTIES
  PASS_REGULAR_EXPRESSION "\"schema\":\"ribo.metrics.v1\".*\"predicted_topology\":\"nested\".*\"reference_topology\":\"nested\".*\"f1\":0.666667")
add_test(NAME evaluate_rejects_malformed_structure
  COMMAND ribo-evaluate "@${CMAKE_CURRENT_SOURCE_DIR}/examples/malformed_structure.db" "()")
set_tests_properties(evaluate_rejects_malformed_structure PROPERTIES WILL_FAIL TRUE)
add_test(NAME evaluate_rejects_length_mismatch
  COMMAND ribo-evaluate "()" "()..")
set_tests_properties(evaluate_rejects_length_mismatch PROPERTIES WILL_FAIL TRUE)
add_test(NAME dataset_case_evaluation
  COMMAND ribo-dataset-evaluate ${CMAKE_CURRENT_SOURCE_DIR}/examples/dataset_case.yaml)
set_tests_properties(dataset_case_evaluation PROPERTIES
  PASS_REGULAR_EXPRESSION "id=nested_demo_001.*topology=nested.*structure=.*f1=")
add_test(NAME dataset_case_evaluation_json
  COMMAND ribo-dataset-evaluate --json ${CMAKE_CURRENT_SOURCE_DIR}/examples/dataset_case.yaml)
set_tests_properties(dataset_case_evaluation_json PROPERTIES
  PASS_REGULAR_EXPRESSION "schema.*ribo.dataset-metrics.v1.*min_loop.*3.*pair_score.*1.*wobble_score.*0")
add_test(NAME dataset_h_type_evaluation
  COMMAND ribo-dataset-evaluate ${CMAKE_CURRENT_SOURCE_DIR}/examples/dataset_crossing_case.yaml)
set_tests_properties(dataset_h_type_evaluation PROPERTIES
  PASS_REGULAR_EXPRESSION "topology=h_type_pseudoknot.*stem1_length=2.*stem2_length=1.*structure=.*f1=")
add_test(NAME dataset_h_type_evaluation_json
  COMMAND ribo-dataset-evaluate --json ${CMAKE_CURRENT_SOURCE_DIR}/examples/dataset_crossing_case.yaml)
set_tests_properties(dataset_h_type_evaluation_json PROPERTIES
  PASS_REGULAR_EXPRESSION "schema.*ribo.dataset-metrics.v1.*topology.*h_type_pseudoknot.*stem1_length.*2.*stem2_length.*1.*structure")

add_test(NAME valid_shape_constraint
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/shape_constraint.mlir --plan)

add_test(NAME attached_shape_constraint
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/shape_attached_fold.mlir)
set_tests_properties(attached_shape_constraint PROPERTIES
  PASS_REGULAR_EXPRESSION "score=1 structure=\\(\\)\\.\\.")

add_test(NAME attached_shape_constraint_plan
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/shape_attached_fold.mlir --plan)
set_tests_properties(attached_shape_constraint_plan PROPERTIES
  PASS_REGULAR_EXPRESSION "constraint=shape-soft")
add_test(NAME calibrated_shape_execution
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/shape_calibrated_fold.mlir)
set_tests_properties(calibrated_shape_execution PROPERTIES
  PASS_REGULAR_EXPRESSION "calibration=slope:2,intercept:-0.1")
add_test(NAME invalid_shape_calibration
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/invalid_shape_calibration.mlir)
set_tests_properties(invalid_shape_calibration PROPERTIES WILL_FAIL TRUE)
add_test(NAME calibrated_shape_lowering_metadata
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/shape_calibrated_fold.mlir --lower)
set_tests_properties(calibrated_shape_lowering_metadata PROPERTIES
  PASS_REGULAR_EXPRESSION "shape_intercept = -1.000000e-01.*shape_slope = 2")
add_test(NAME nested_lowering_artifact
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/nested.mlir --lower)
set_tests_properties(nested_lowering_artifact PROPERTIES
  PASS_REGULAR_EXPRESSION "ribo.lowered_fold.*dependencies = \".*\".*runtime = \"ribo-runtime.foldNested\".*state_rank = 2.*state_variables = \"i,j\".*storage = \"upper_triangular\\(i,j\\)\".*strategy = \"nested_triangular_dp\".*topology = \"nested\"")
add_test(NAME typed_topology_attribute
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/typed_topology.mlir)
set_tests_properties(typed_topology_attribute PROPERTIES
  PASS_REGULAR_EXPRESSION "score=1 structure=\(\)")
add_test(NAME all_typed_topology_attributes
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/typed_topologies.mlir --plan)
set_tests_properties(all_typed_topology_attributes PROPERTIES
  PASS_REGULAR_EXPRESSION "topology=general_crossing")

add_test(NAME invalid_shape_constraint
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/invalid_shape_constraint.mlir --plan)
set_tests_properties(invalid_shape_constraint PROPERTIES WILL_FAIL TRUE)

add_test(NAME detached_shape_constraint
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/detached_shape_constraint.mlir --plan)
set_tests_properties(detached_shape_constraint PROPERTIES WILL_FAIL TRUE)

add_test(NAME h_type_execution
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/h_type.mlir)
set_tests_properties(h_type_execution PROPERTIES
  PASS_REGULAR_EXPRESSION "score=4.*structure=.*\\[.*pair_indices=.*2-8,1-9,5-11,4-12")
add_test(NAME h_type_decomposition_plan
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/h_type.mlir --plan)
set_tests_properties(h_type_decomposition_plan PROPERTIES
  PASS_REGULAR_EXPRESSION "recurrence=bounded-h-type")
add_test(NAME h_type_lowering_artifact
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/h_type.mlir --lower)
set_tests_properties(h_type_lowering_artifact PROPERTIES
  PASS_REGULAR_EXPRESSION "gap1_min_loop = 1.*gap2_min_loop = 1.*gap3_min_loop = 1.*optimality = \"restricted-exact\".*runtime = \"ribo-runtime.foldHType\".*state_rank = 4.*stem_length = 2.*storage = \"bounded_crossing\\(a,c,b,d\\)\".*strategy = \"h_type_bounded_dp\".*time = \"O\\(n\\^4\\)\"")
add_test(NAME h_type_unequal_stems
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/h_type_unequal.mlir)
set_tests_properties(h_type_unequal_stems PROPERTIES
  PASS_REGULAR_EXPRESSION "score=.*structure=.*\\[.*pair_indices=")
add_test(NAME h_type_unequal_lowering_contract
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/h_type_unequal.mlir --lower)
set_tests_properties(h_type_unequal_lowering_contract PROPERTIES
  PASS_REGULAR_EXPRESSION "gap1_min_loop = 1.*gap2_min_loop = 1.*gap3_min_loop = 1.*runtime = \"ribo-runtime.foldHType\".*stem1_length = 2.*stem2_length = 1")
add_test(NAME h_type_requires_stem_length
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/invalid_h_type.mlir)
set_tests_properties(h_type_requires_stem_length PROPERTIES WILL_FAIL TRUE)
add_test(NAME h_type_shape_execution
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/h_type_shape.mlir)
set_tests_properties(h_type_shape_execution PROPERTIES
  PASS_REGULAR_EXPRESSION "constraint=shape-soft")
add_test(NAME h_type_shape_lowering_contract
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/h_type_shape.mlir --lower)
set_tests_properties(h_type_shape_lowering_contract PROPERTIES
  PASS_REGULAR_EXPRESSION "runtime = \"ribo-runtime.foldHTypeWithShape\"")
add_test(NAME nested_rejects_h_type_parameters
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/nested_h_type_parameter.mlir)
set_tests_properties(nested_rejects_h_type_parameters PROPERTIES WILL_FAIL TRUE)
add_test(NAME h_type_rejects_partial_stem_lengths
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/h_type_partial_lengths.mlir)
set_tests_properties(h_type_rejects_partial_stem_lengths PROPERTIES WILL_FAIL TRUE)

add_test(NAME invalid_rna_alphabet
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/invalid_sequence.mlir)
set_tests_properties(invalid_rna_alphabet PROPERTIES
  WILL_FAIL TRUE)

add_test(NAME invalid_loop_parameter
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/invalid_loop.mlir)
set_tests_properties(invalid_loop_parameter PROPERTIES
  WILL_FAIL TRUE)
add_test(NAME invalid_pair_score
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/invalid_pair_score.mlir)
set_tests_properties(invalid_pair_score PROPERTIES
  WILL_FAIL TRUE)

add_test(NAME kissing_hairpin_topology_plan
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/kissing_hairpin.mlir --plan)
set_tests_properties(kissing_hairpin_topology_plan PROPERTIES
  PASS_REGULAR_EXPRESSION "recurrence=tgb-ccj")

add_test(NAME kissing_candidate_contract
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/kissing_candidate.mlir --plan)
set_tests_properties(kissing_candidate_contract PROPERTIES
  PASS_REGULAR_EXPRESSION "candidate=kissing_hairpin stems=3 lengths=2,1,2 endpoints=0-11,1-10,3-8 loops=1,0,1,0,1 weight=6 shared_stem=2 shared_stem_score_once=true sequence_score=4")
add_test(NAME kissing_candidate_plan_json
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/kissing_candidate.mlir --plan-json)
set_tests_properties(kissing_candidate_plan_json PROPERTIES
  PASS_REGULAR_EXPRESSION "\\\"kind\\\":\\\"kissing_hairpin_candidate\\\".*\\\"sequence_score\\\":4.*\\\"shared_stem_score_once\\\":true")
add_test(NAME kissing_candidate_selection
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/kissing_candidate.mlir --select)
set_tests_properties(kissing_candidate_selection PROPERTIES
  PASS_REGULAR_EXPRESSION "selected_weight=6 candidates=1 indices=0")
add_test(NAME kissing_candidate_global_selection
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/kissing_candidates.mlir --select)
set_tests_properties(kissing_candidate_global_selection PROPERTIES
  PASS_REGULAR_EXPRESSION "selected_weight=8 candidates=1 indices=1")
add_test(NAME kissing_candidate_sequence_selection
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/kissing_candidates.mlir --select-sequence)
set_tests_properties(kissing_candidate_sequence_selection PROPERTIES
  PASS_REGULAR_EXPRESSION "selected_weight=4 candidates=1 indices=0")
add_test(NAME kissing_candidate_sequence_selection_json
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/kissing_candidates.mlir --select-sequence-json)
set_tests_properties(kissing_candidate_sequence_selection_json PROPERTIES
  PASS_REGULAR_EXPRESSION "\\\"schema\\\":\\\"ribo.selection.v1\\\".*\\\"selected_weight\\\":4.*\\\"indices\\\":\\\[0\\\]")
add_test(NAME kissing_candidate_wobble_selection
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/kissing_candidate_wobble.mlir --select-sequence)
set_tests_properties(kissing_candidate_wobble_selection PROPERTIES
  PASS_REGULAR_EXPRESSION "selected_weight=8 candidates=1 indices=0")
add_test(NAME kissing_candidate_wobble_plan_json
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/kissing_candidate_wobble.mlir --plan-json)
set_tests_properties(kissing_candidate_wobble_plan_json PROPERTIES
  PASS_REGULAR_EXPRESSION "\\\"sequence_score\\\":8.*\\\"pair_score\\\":2.*\\\"wobble_score\\\":2")
add_test(NAME invalid_kissing_candidate_wobble
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/invalid_candidate_wobble.mlir --plan)
set_tests_properties(invalid_kissing_candidate_wobble PROPERTIES WILL_FAIL TRUE)
add_test(NAME invalid_kissing_candidate_pair_score
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/invalid_candidate_pair_score.mlir --plan)
set_tests_properties(invalid_kissing_candidate_pair_score PROPERTIES WILL_FAIL TRUE)
add_test(NAME energy_metadata_contract
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/energy_metadata.mlir --plan)
set_tests_properties(energy_metadata_contract PROPERTIES PASS_REGULAR_EXPRESSION "topology=nested")
add_test(NAME invalid_energy_metadata
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/invalid_energy_metadata.mlir --plan)
set_tests_properties(invalid_energy_metadata PROPERTIES WILL_FAIL TRUE)
add_test(NAME energy_metadata_preserved_by_lowering
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/energy_metadata_reference.mlir --lower)
set_tests_properties(energy_metadata_preserved_by_lowering PROPERTIES
  PASS_REGULAR_EXPRESSION "parameter_set = \"Turner2023\".*temperature_k = 3.101500e")
add_test(NAME invalid_lowered_energy_metadata
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/invalid_lowered_energy.mlir --plan)
set_tests_properties(invalid_lowered_energy_metadata PROPERTIES WILL_FAIL TRUE)
add_test(NAME valid_lowered_energy_metadata
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/lowered_energy_valid.mlir --plan)
add_test(NAME kissing_candidate_selection_json
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/kissing_candidates.mlir --select-json)
set_tests_properties(kissing_candidate_selection_json PROPERTIES
  PASS_REGULAR_EXPRESSION "\\\"schema\\\":\\\"ribo.selection.v1\\\".*\\\"selected_weight\\\":8.*\\\"indices\\\":\\\[1\\\]")
add_test(NAME kissing_candidate_rejects_mixed_sources
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/mixed_candidate_sources.mlir --select)
set_tests_properties(kissing_candidate_rejects_mixed_sources PROPERTIES WILL_FAIL TRUE)
add_test(NAME invalid_kissing_candidate_contract
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/invalid_kissing_candidate.mlir --plan)
set_tests_properties(invalid_kissing_candidate_contract PROPERTIES WILL_FAIL TRUE)
add_test(NAME duplicate_kissing_endpoint_contract
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/duplicate_kissing_endpoint.mlir --plan)
set_tests_properties(duplicate_kissing_endpoint_contract PROPERTIES WILL_FAIL TRUE)
add_test(NAME kissing_candidate_requires_weight
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/invalid_candidate_weight.mlir --select)
set_tests_properties(kissing_candidate_requires_weight PROPERTIES WILL_FAIL TRUE)
add_test(NAME kissing_candidate_rejects_internal_chemistry
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/invalid_candidate_chemistry.mlir --select)
set_tests_properties(kissing_candidate_rejects_internal_chemistry PROPERTIES WILL_FAIL TRUE)

add_test(NAME general_crossing_topology_plan
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/general_crossing.mlir --plan)
set_tests_properties(general_crossing_topology_plan PROPERTIES
  PASS_REGULAR_EXPRESSION "strategy=explicit-heuristic-required")

add_test(NAME valid_dot_bracket_structure
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/structure.mlir --plan)
add_test(NAME classify_nested_structure
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/structure.mlir --classify)
set_tests_properties(classify_nested_structure PROPERTIES
  PASS_REGULAR_EXPRESSION "observed_topology=nested.*base_pairs=3")
add_test(NAME classify_crossing_structure
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/pseudoknot_structure.mlir --classify)
set_tests_properties(classify_crossing_structure PROPERTIES
  PASS_REGULAR_EXPRESSION "observed_topology=crossing.*observed_family=h_type_pseudoknot")
add_test(NAME classify_topology_mismatch
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/mismatched_structure_topology.mlir --classify)
set_tests_properties(classify_topology_mismatch PROPERTIES
  PASS_REGULAR_EXPRESSION "topology_consistent=no")
add_test(NAME classify_typed_structure
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/typed_structure.mlir --classify)
set_tests_properties(classify_typed_structure PROPERTIES
  PASS_REGULAR_EXPRESSION "declared_topology=h_type_pseudoknot.*topology_consistent=yes")
add_test(NAME strict_topology_mismatch
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/strict_mismatch.mlir --classify-strict)
set_tests_properties(strict_topology_mismatch PROPERTIES WILL_FAIL TRUE)

add_test(NAME invalid_dot_bracket_structure
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/invalid_structure.mlir --plan)
set_tests_properties(invalid_dot_bracket_structure PROPERTIES WILL_FAIL TRUE)

add_test(NAME crossing_structure_with_pseudoknot_topology
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/pseudoknot_structure.mlir --plan)

add_test(NAME kissing_structure_with_three_stem_alphabets
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/kissing_structure.mlir --classify)
set_tests_properties(kissing_structure_with_three_stem_alphabets PROPERTIES
  PASS_REGULAR_EXPRESSION "base_pairs=3.*observed_family=kissing_hairpin_candidate.*topology_consistent=yes")
add_test(NAME invalid_kissing_structure_contract
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/invalid_kissing_structure.mlir --classify)
set_tests_properties(invalid_kissing_structure_contract PROPERTIES WILL_FAIL TRUE)

add_test(NAME incompatible_base_pair
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/incompatible_pair.mlir --plan)
set_tests_properties(incompatible_base_pair PROPERTIES WILL_FAIL TRUE)

add_test(NAME gu_wobble_energy
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/wobble.mlir)
set_tests_properties(gu_wobble_energy PROPERTIES
  PASS_REGULAR_EXPRESSION "score=2")

add_test(NAME bounded_crossing_plan
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/bounded_crossing.mlir --plan)
set_tests_properties(bounded_crossing_plan PROPERTIES
  PASS_REGULAR_EXPRESSION "strategy=specialized-dp-required")

add_test(NAME bounded_crossing_requires_depth
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/missing_crossing_depth.mlir --plan)
set_tests_properties(bounded_crossing_requires_depth PROPERTIES WILL_FAIL TRUE)

add_test(NAME sequence_profile
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/nested.mlir --profile)
set_tests_properties(sequence_profile PROPERTIES
  PASS_REGULAR_EXPRESSION "gc_count=6")
add_test(NAME sequence_profile_base_counts
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/nested.mlir --profile)
set_tests_properties(sequence_profile_base_counts PROPERTIES
  PASS_REGULAR_EXPRESSION "u_count=1")

add_test(NAME nearest_neighbor_requires_energy_backend
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/nearest_neighbor_unimplemented.mlir)
set_tests_properties(nearest_neighbor_requires_energy_backend PROPERTIES WILL_FAIL TRUE)
add_test(NAME nearest_neighbor_lower_requires_energy_backend
  COMMAND bio-llvm ${CMAKE_CURRENT_SOURCE_DIR}/examples/nearest_neighbor_lower.mlir --lower)
set_tests_properties(nearest_neighbor_lower_requires_energy_backend PROPERTIES WILL_FAIL TRUE)
