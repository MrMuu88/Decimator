#include "stl_io.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <unordered_map>
#include <array>
#include <string>
#include <algorithm>
#include <cmath>

namespace {

// Hash and equality for a float3 vertex, operating on raw bit patterns to avoid
// floating-point comparison issues in the map lookup.
struct VertexKey {
    float x, y, z;
    bool operator==(const VertexKey& o) const {
        return x == o.x && y == o.y && z == o.z;
    }
};

struct VertexKeyHash {
    size_t operator()(const VertexKey& k) const noexcept {
        uint32_t bx, by, bz;
        memcpy(&bx, &k.x, 4);
        memcpy(&by, &k.y, 4);
        memcpy(&bz, &k.z, 4);
        size_t h = bx;
        h ^= (size_t)by + 0x9e3779b9u + (h << 6) + (h >> 2);
        h ^= (size_t)bz + 0x9e3779b9u + (h << 6) + (h >> 2);
        return h;
    }
};

#pragma pack(push, 1)
struct BinaryStlFacet {
    float normal[3];
    float v0[3];
    float v1[3];
    float v2[3];
    uint16_t attr;
};
#pragma pack(pop)

static_assert(sizeof(BinaryStlFacet) == 50, "BinaryStlFacet must be 50 bytes");

} // anonymous namespace

namespace Slic3r {

indexed_triangle_set read_stl(const std::string& path)
{
    indexed_triangle_set its;

    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) {
        fprintf(stderr, "Error: cannot open '%s'\n", path.c_str());
        return its;
    }

    // Read and inspect header to detect binary vs ASCII
    char header[80] = {};
    if (fread(header, 1, 80, fp) != 80) {
        fclose(fp);
        return its;
    }

    uint32_t face_count = 0;
    if (fread(&face_count, 4, 1, fp) != 1 || face_count == 0) {
        fclose(fp);
        return its;
    }

    // Sanity-check: a valid binary STL is exactly 84 + face_count*50 bytes.
    // If the header starts with "solid" it might be ASCII — fall through to
    // a simple ASCII parser in that case.
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    bool is_binary = (file_size == 84 + (long)face_count * 50);

    if (!is_binary) {
        // ASCII STL parser
        fseek(fp, 0, SEEK_SET);
        fclose(fp);

        fp = fopen(path.c_str(), "r");
        if (!fp) return its;

        std::unordered_map<VertexKey, int, VertexKeyHash> vmap;
        vmap.reserve(1024);

        char line[256];
        float vx, vy, vz;
        int tri_verts[3];
        int tri_count = 0;

        while (fgets(line, sizeof(line), fp)) {
            // skip leading whitespace
            const char* p = line;
            while (*p == ' ' || *p == '\t') ++p;

            if (strncmp(p, "vertex", 6) == 0) {
                if (sscanf(p + 6, "%f %f %f", &vx, &vy, &vz) == 3) {
                    VertexKey key{ vx, vy, vz };
                    auto it = vmap.find(key);
                    int idx;
                    if (it == vmap.end()) {
                        idx = (int)its.vertices.size();
                        its.vertices.push_back(stl_vertex(vx, vy, vz));
                        vmap[key] = idx;
                    } else {
                        idx = it->second;
                    }
                    tri_verts[tri_count % 3] = idx;
                    ++tri_count;
                    if (tri_count % 3 == 0) {
                        its.indices.push_back(
                            stl_triangle_vertex_indices(tri_verts[0], tri_verts[1], tri_verts[2]));
                    }
                }
            }
        }
        fclose(fp);
        return its;
    }

    // Binary STL
    fseek(fp, 84, SEEK_SET);

    std::unordered_map<VertexKey, int, VertexKeyHash> vmap;
    vmap.reserve(face_count * 2);
    its.indices.reserve(face_count);
    its.vertices.reserve(face_count);   // rough estimate; dedup will reduce it

    BinaryStlFacet facet;
    for (uint32_t i = 0; i < face_count; ++i) {
        if (fread(&facet, 50, 1, fp) != 1) break;

        int tri[3];
        const float* vpts[3] = { facet.v0, facet.v1, facet.v2 };
        for (int v = 0; v < 3; ++v) {
            VertexKey key{ vpts[v][0], vpts[v][1], vpts[v][2] };
            auto it = vmap.find(key);
            if (it == vmap.end()) {
                tri[v] = (int)its.vertices.size();
                its.vertices.push_back(stl_vertex(key.x, key.y, key.z));
                vmap[key] = tri[v];
            } else {
                tri[v] = it->second;
            }
        }
        its.indices.push_back(stl_triangle_vertex_indices(tri[0], tri[1], tri[2]));
    }

    fclose(fp);
    return its;
}

void write_stl(const std::string& path, const indexed_triangle_set& its)
{
    FILE* fp = fopen(path.c_str(), "wb");
    if (!fp) {
        fprintf(stderr, "Error: cannot write '%s'\n", path.c_str());
        return;
    }

    // 80-byte header
    char header[80] = "Binary STL written by decimator";
    fwrite(header, 1, 80, fp);

    uint32_t face_count = (uint32_t)its.indices.size();
    fwrite(&face_count, 4, 1, fp);

    BinaryStlFacet facet;
    memset(&facet, 0, sizeof(facet));

    for (uint32_t i = 0; i < face_count; ++i) {
        const auto& tri = its.indices[i];
        const stl_vertex& v0 = its.vertices[tri[0]];
        const stl_vertex& v1 = its.vertices[tri[1]];
        const stl_vertex& v2 = its.vertices[tri[2]];

        // Compute face normal
        stl_vertex e1 = v1 - v0;
        stl_vertex e2 = v2 - v0;
        stl_vertex n  = e1.cross(e2);
        float len = n.norm();
        if (len > 0.f) n /= len;

        facet.normal[0] = n.x(); facet.normal[1] = n.y(); facet.normal[2] = n.z();
        facet.v0[0] = v0.x(); facet.v0[1] = v0.y(); facet.v0[2] = v0.z();
        facet.v1[0] = v1.x(); facet.v1[1] = v1.y(); facet.v1[2] = v1.z();
        facet.v2[0] = v2.x(); facet.v2[1] = v2.y(); facet.v2[2] = v2.z();
        facet.attr = 0;

        fwrite(&facet, 50, 1, fp);
    }

    fclose(fp);
}

} // namespace Slic3r
