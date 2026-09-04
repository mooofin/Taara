# Tara

![Tara — tiny abstractions for real acceleration](assets/tara.png)

`Tara` is a  RNA-folding compiler built on [MLIR]. RNA
topology is part of the IR, so it chooses the lowering strategy:

$$
\text{nested}\;\longrightarrow\;O(n^3)\text{ triangular DP}
\qquad
\text{pseudoknot}\;\longrightarrow\;\text{specialized backend}
$$

The nested and bounded H-type reference kernels are executable.

## Build

Set `MLIR_DIR` to the directory containing `MLIRConfig.cmake`.

### Ninja / Clang

```powershell
$env:MLIR_DIR = "C:/path/to/llvm-install/lib/cmake/mlir"
cmake --preset ninja
cmake --build --preset ninja-debug
ctest --preset ninja-tests
```

Clang uses experimental C++26 mode when available

### Visual Studio

```powershell
$env:MLIR_DIR = "C:/path/to/llvm-install/lib/cmake/mlir"
cmake --preset vs2026
cmake --build --preset vs2026-debug
ctest --preset vs2026-tests
```

MSVC uses C++23 for compatibility. Open the generated solution in Visual
Studio if you want to work interactively.

## For Trying  it

```powershell
.\build-vs\Debug\tara.exe examples\nested.mlir
.\build-vs\Debug\tara.exe examples\h_type.mlir --plan
```

The nested IR looks like this:

```mlir
module {
  %rna = "ribo.rna"() {sequence = "GCGAAAGCUC"} : () -> tensor<?xi8>
  %fold = "ribo.fold"(%rna) {
    topology = #ribo.nested, min_loop = 3 : i64, pair_score = 1 : i64
  } : (tensor<?xi8>) -> i64
}
```

For a sequence $x_1\ldots x_n$, the reference recurrence is the usual
Nussinov-style interval program:

$$
N_{i,j}=\max(
N_{i+1,j},\;
N_{i,j-1},\;
N_{i+1,j-1}+s(x_i,x_j)),
\qquad j-i>\ell,
$$

where $\ell$ is `min_loop` and

$$
s(a,b)=
\begin{cases}
p, & (a,b)\in\{AU,UA,CG,GC\},\\
w, & (a,b)\in\{GU,UG\},\\
0, & \text{otherwise.}
\end{cases}
$$

where pairing is allowed only when the minimum loop length is satisfied.
`pair_score` rewards Watson–Crick pairs; `wobble_score` optionally rewards
GU pairs. The result includes a score and a dot-bracket traceback.

Current working  commands:

```text
--plan                 show the selected recurrence
--plan-json            emit a versioned JSON plan
--lower                emit the lowered runtime contract
--profile              report sequence statistics
--classify             inspect a structure's topology
--select-sequence      score and select kissing candidates
```

## C++ runtime api

```cpp
#include "ribo/Runtime.h"

auto result = ribo::runtime::foldNested("GCGAAAGCUC", 3, 1);
```

[MLIR]: https://mlir.llvm.org/
