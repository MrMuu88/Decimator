## Background

This repository contains the source code of BambuStudio, a 3D slicer for 3D printing. It has a high-quality algorithm to decimate 3D models without significant quality loss. The feature is accessible by right-clicking a model in the 3D editor and selecting "Simplify Model", which opens a dialog to configure and run the simplification.

The goal is to extract this algorithm into a standalone Windows console application for bulk STL decimation (thousands of files), parameterized from the command line. Bulk file handling will be done via a separate PowerShell script.

---

## Algorithm Location in BambuStudio

The "Simplify Model" feature uses a custom **Quadric Error Metrics (QEM)** implementation (based on Garland & Heckbert 1997). It is entirely in-house — no external mesh library is involved.

| Role | File | Key entry point |
|---|---|---|
| Algorithm (core) | `src/libslic3r/QuadricEdgeCollapse.cpp` | `its_quadric_edge_collapse()` at line 160 |
| Algorithm header | `src/libslic3r/QuadricEdgeCollapse.hpp` | function signature |
| Priority queue helper | `src/libslic3r/MutablePriorityQueue.hpp` | used internally |
| Mesh data struct | `src/admesh/stl.h` line 220 | `indexed_triangle_set`, `stl_vertex`, `stl_triangle_vertex_indices` |
| UI gizmo (not needed) | `src/slic3r/GUI/Gizmos/GLGizmoSimplify.cpp` | calls the algorithm at line 527 |

### Algorithm API

```cpp
void its_quadric_edge_collapse(
    indexed_triangle_set& its,       // mesh, modified in-place
    uint32_t triangle_count = 0,     // target triangle count (0 = use max_error)
    float* max_error = nullptr,      // max quadric error threshold; outputs last used error
    std::function<void(void)> throw_on_cancel = nullptr,
    std::function<void(int)>  statusfn = nullptr   // progress 0-100
);
```

`indexed_triangle_set` holds `std::vector<Vec3f> vertices` and `std::vector<Vec3i> indices` (3 vertex indices per triangle). Both Eigen types via `src/admesh/stl.h`.

---

## Standalone Console App — Design

**Command line interface:**

```
decimator.exe --input <file.stl> [--output <out.stl>] [--suffix <_decimated>] [--ratio <0-100>] [--max-error <float>]
```

- `--ratio`: percentage of triangles to keep (e.g. `50` = keep 50% of triangles)
- `--max-error`: alternative mode; maximum allowed quadric error (e.g. `0.1` = medium quality)
- `--output`: explicit output filename
- `--suffix`: suffix appended before `.stl` extension (default: `_decimated`)
- If neither `--output` nor `--suffix` is provided, default suffix `_decimated` is used

---

## Files to Extract from BambuStudio

Copy these three files directly into the new project:

| Source file | Change needed |
|---|---|
| `src/libslic3r/QuadricEdgeCollapse.cpp` | Remove TBB (see patch below) |
| `src/libslic3r/QuadricEdgeCollapse.hpp` | None |
| `src/libslic3r/MutablePriorityQueue.hpp` | None |

Also copy the Eigen headers (header-only, already in this repo):

- `src/eigen/` → copy entire folder into new project as `eigen/`

---

## Required Patch — Remove TBB

`QuadricEdgeCollapse.cpp` uses Intel TBB for one parallelized init loop. Replace it with a plain `for` loop to eliminate the TBB dependency. The algorithm remains fully correct; only the init phase loses parallelism (negligible for typical STL sizes).

**Line 5** — remove:
```cpp
#include <tbb/parallel_for.h>
```

Find the `tbb::parallel_for` call in the `init()` function and replace it with an equivalent sequential `for` loop over the same range.

---

## Files to Write (New Project)

| File | Purpose |
|---|---|
| `src/types.h` | Minimal `stl_vertex` (Eigen Vec3f), `stl_triangle_vertex_indices` (Eigen Vec3i), `indexed_triangle_set` struct, stub `FaceProperty`, `ArithmeticOnly` type alias |
| `src/stl_io.h` / `src/stl_io.cpp` | Binary STL reader (80-byte header + raw triangles → deduplicated `indexed_triangle_set`) and binary STL writer |
| `src/main.cpp` | Argument parsing, STL read, call `its_quadric_edge_collapse()`, STL write |
| `CMakeLists.txt` | Build configuration |

---

## New Project Structure

```
decimator/
  CMakeLists.txt
  eigen/                            <- copied from BambuStudio src/eigen/
  src/
    main.cpp
    types.h
    stl_io.h
    stl_io.cpp
    QuadricEdgeCollapse.hpp         <- copied, unchanged
    QuadricEdgeCollapse.cpp         <- copied, TBB removed
    MutablePriorityQueue.hpp        <- copied, unchanged
```

---

## Dependencies to Install

Nothing is currently installed on this machine (no MSVC, no CMake detected).

| Tool | Required | How to install |
|---|---|---|
| **Visual Studio 2022 Community** | Yes | https://visualstudio.microsoft.com/ — select "Desktop development with C++" workload |
| **CMake** | Yes | Tick "C++ CMake tools for Windows" in the VS installer (easiest), or download from cmake.org |
| **Eigen3** | Yes (header-only) | Already in this repo at `src/eigen/` — copy folder, no install needed |
| **TBB** | No | Eliminated by the patch above |
| **Boost** | No | Not needed — the admesh reader is not used; a simple binary STL reader is written instead |

Visual Studio 2022 Community is free. The VS installer can install CMake at the same time.

---

## Build Steps (after installing VS + CMake)

```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Output: `build/Release/decimator.exe`
