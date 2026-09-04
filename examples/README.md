# Examples

The fixtures in this directory exercise the biology and compiler contracts:

- `nested.mlir`: executable nested reference fold.
- `wobble.mlir`: GU wobble scoring.
- `shape_constraint.mlir`: validated SHAPE reactivity tensor.
- `shape_attached_fold.mlir`: executable SHAPE-aware nested fold.
- `h_type_shape.mlir`: executable SHAPE-aware bounded H-type fold.
- `detached_shape_constraint.mlir`: rejects evidence attached to another RNA.
- `structure.mlir`: balanced nested structure annotation.
- `pseudoknot_structure.mlir`: crossing structure annotation.
- `bounded_crossing.mlir`: explicit crossing-depth resource.
- `h_type.mlir`, `kissing_hairpin.mlir`, `general_crossing.mlir`: topology planning and diagnostics.
- `kissing_candidates.mlir`: exact bounded conflict-free candidate selection; `mixed_candidate_sources.mlir` checks source isolation.
- `kissing_candidate_wobble.mlir`: sequence-derived candidate scoring with an explicit GU wobble reward.
- `invalid_candidate_chemistry.mlir`: rejects chemically incompatible internal stem pairs.
- `invalid_candidate_wobble.mlir`: rejects negative candidate wobble rewards.
- `invalid_candidate_pair_score.mlir`: rejects negative candidate pair rewards.
- `energy_metadata.mlir`: validates a complete future thermodynamic context.
- `energy_metadata_reference.mlir`: preserves validated context through legal reference lowering.
- `invalid_lowered_energy.mlir`: rejects malformed context in lowered IR.
- `lowered_energy_valid.mlir`: accepts a serialized lowered artifact with context.
- `invalid_energy_metadata.mlir`: rejects invalid thermodynamic conditions.
- `invalid_*.mlir`: verifier regression cases.

Scoring assumptions and the boundary to thermodynamic models are documented in
`docs/SCORING.md`.
The planned thermodynamic backend contract is in `docs/THERMODYNAMIC_BACKEND.md`.

Run a fixture with the built executable:

```powershell
.\build-vs\Debug\bio-llvm.exe .\examples\nested.mlir
.\build-vs\Debug\bio-llvm.exe .\examples\bounded_crossing.mlir --plan
.\build-vs\Debug\bio-llvm.exe .\examples\kissing_candidates.mlir --select-sequence
```
