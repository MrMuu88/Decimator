#pragma once
#include <cstdint>
#include <vector>
#include <limits>
#include <type_traits>
#include <Eigen/Geometry>

// Global scalar types used by the algorithm
using coord_t  = int32_t;
using coordf_t = double;

template<typename T, typename O = T>
using ArithmeticOnly = std::enable_if_t<std::is_arithmetic<T>::value, O>;

namespace Slic3r {

using Vec3f = Eigen::Matrix<float,  3, 1, Eigen::DontAlign>;
using Vec3d = Eigen::Matrix<double, 3, 1, Eigen::DontAlign>;
using Vec3i = Eigen::Matrix<int,    3, 1, Eigen::DontAlign>;

using stl_vertex                  = Vec3f;
using stl_triangle_vertex_indices = Eigen::Matrix<int, 3, 1, Eigen::DontAlign>;

struct FaceProperty {};  // stub — not used by the decimation algorithm

struct indexed_triangle_set {
    std::vector<stl_triangle_vertex_indices> indices;
    std::vector<stl_vertex>                  vertices;
    std::vector<FaceProperty>                properties;

    bool empty() const { return indices.empty() || vertices.empty(); }
};

} // namespace Slic3r
