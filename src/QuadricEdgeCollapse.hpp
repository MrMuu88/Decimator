// paper: https://people.eecs.berkeley.edu/~jrs/meshpapers/GarlandHeckbert2.pdf
// sum up: https://users.csc.calpoly.edu/~zwood/teaching/csc570/final06/jseeba/
// inspiration: https://github.com/sp4cerat/Fast-Quadric-Mesh-Simplification

#pragma once
#include <cstdint>
#include <functional>
#include "types.h"

namespace Slic3r {

/// Simplify mesh by Quadric Edge Collapse metric (Garland & Heckbert 1997).
/// @param its            IN/OUT triangle mesh to be simplified.
/// @param triangle_count Target triangle count. Pass 0 to use max_error instead.
/// @param max_error      Max quadric error threshold. On return contains the last
///                       error value used. Pass nullptr for no threshold.
/// @param throw_on_cancel Called periodically; throw from it to abort.
/// @param statusfn       Progress callback, values 0-100.
void its_quadric_edge_collapse(
    indexed_triangle_set &    its,
    uint32_t                  triangle_count  = 0,
    float *                   max_error       = nullptr,
    std::function<void(void)> throw_on_cancel = nullptr,
    std::function<void(int)>  statusfn        = nullptr);

} // namespace Slic3r
