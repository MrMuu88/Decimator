# Decimator

A standalone Windows console application for batch STL mesh decimation. It reduces the triangle count of STL files using the **Quadric Edge Collapse (QEM)** algorithm extracted from [BambuStudio](https://github.com/bambulab/BambuStudio), which implements Garland & Heckbert 1997.

The tool is designed for bulk decimation of large numbers of STL files, driven from a PowerShell script.

if you like this project [![Buy Me a Coffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-support-yellow?logo=buy-me-a-coffee)](https://buymeacoffee.com/mrmuu)

---

## Usage

```
decimator.exe <input.stl> [quality option] [output option]
```
for decimating models in bulk, copy the decimator.exe into a folder with STLs, or add it to the PATH environment variable
```
PS> folder_with_STLs\ get-childitem |%{.\decimator.exe $_ [quality option] [output option]}
```

### Quality options (choose one; if none provided, default is `--ExtraHigh`)

| Flag | Max-error | Effect |
|---|---|---|
| `--ExtraHigh` | 0.001 | Near-identical to original |
| `--High` | 0.01 | Minimal visible difference |
| `--Medium` | 0.1 | Noticeable reduction, small details smoothed |
| `--Low` | 0.5 | Aggressive reduction, fine detail lost |
| `--ExtraLow` | 1.0 | Maximum reduction, shape roughly preserved |
| `--ratio <0–100>` | — | Keep exactly N% of triangles |
| `--max-error <float>` | — | Set quadric error threshold directly |

### Output options (choose one; if non provided, default suffixes output file  with `_decimated`)

| Flag | Effect |
|---|---|
| `--outputfile <path>` | Save to an exact file path |
| `--suffix <text>` | Append text before `.stl` (e.g. `_small` → `model_small.stl`) |

### Examples

```powershell
# Default quality, saves as model_decimated.stl
decimator.exe model.stl

# Medium quality, default output name
decimator.exe model.stl --Medium

# High quality with custom suffix
decimator.exe model.stl --High --suffix _high

# Keep 25% of triangles, save to specific path
decimator.exe model.stl --ratio 25 --outputfile C:\output\small.stl

# Set quadric error threshold directly
decimator.exe model.stl --max-error 0.05 --suffix _custom
```

---

## Building

### Requirements

- **Visual Studio 2022** (or the VS Insiders build) with the "Desktop development with C++" workload, which includes MSVC and Ninja
- CMake (included with Visual Studio — tick "C++ CMake tools for Windows" in the installer)
- Eigen3 headers — already bundled in `eigen/`

### Quick build (recommended)

Run `build.bat` from the repo root. It sets up the MSVC environment, configures with Ninja, and builds in Release mode.

```bat
build.bat
```

Output: `build\decimator.exe`

### Manual build

```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

---

## Project structure

```
decimator/
  CMakeLists.txt
  build.bat
  eigen/                          # Bundled Eigen3 headers (header-only)
  src/
    main.cpp                      # CLI argument parsing and orchestration
    types.h                       # indexed_triangle_set, stl_vertex, stl_triangle_vertex_indices
    stl_io.h / stl_io.cpp         # Binary STL reader and writer
    QuadricEdgeCollapse.hpp/.cpp  # QEM algorithm (from BambuStudio, TBB removed)
    MutablePriorityQueue.hpp      # Priority queue helper used by the algorithm
```

### Notes on the algorithm

`QuadricEdgeCollapse.cpp` is copied from BambuStudio's `src/libslic3r/` with one change: the single `tbb::parallel_for` in the init phase is replaced with a plain `for` loop, eliminating the Intel TBB dependency. The algorithm output is identical.

The core entry point:

```cpp
void its_quadric_edge_collapse(
    indexed_triangle_set& its,
    uint32_t triangle_count = 0,      // target triangle count (0 = use max_error)
    float* max_error = nullptr,       // max quadric error; outputs last used error
    std::function<void(void)> throw_on_cancel = nullptr,
    std::function<void(int)>  statusfn = nullptr  // progress 0–100
);
```

---

## License

The QEM algorithm (`QuadricEdgeCollapse.cpp/.hpp`, `MutablePriorityQueue.hpp`) and Eigen are subject to their respective upstream licenses (BambuStudio: AGPL-3.0; Eigen: MPL-2.0). The remaining source files (`main.cpp`, `stl_io.*`, `types.h`) are original to this project.
