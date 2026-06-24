#pragma once
#include "types.h"
#include <string>

namespace Slic3r {

// Read a binary or ASCII STL file and return an indexed triangle set.
// Returns an empty set on failure.
indexed_triangle_set read_stl(const std::string& path);

// Write an indexed triangle set as a binary STL file.
void write_stl(const std::string& path, const indexed_triangle_set& its);

} // namespace Slic3r
