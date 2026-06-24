#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>
#include "stl_io.h"
#include "QuadricEdgeCollapse.hpp"

// Quality presets (same values as BambuStudio's Detail Level slider)
static const float PRESET_EXTRAHIGH = 0.001f;
static const float PRESET_HIGH      = 0.01f;
static const float PRESET_MEDIUM    = 0.1f;
static const float PRESET_LOW       = 0.5f;
static const float PRESET_EXTRALOW  = 1.0f;

static void print_help()
{
    printf(
        "\n"
        "decimator.exe  -  STL mesh decimation tool\n"
        "==========================================\n"
        "\n"
        "  Reduces the triangle count of an STL file using the Quadric Edge Collapse\n"
        "  algorithm. The output is a new STL file with fewer triangles while\n"
        "  preserving the overall shape as much as possible.\n"
        "\n"
        "USAGE\n"
        "  decimator <input.stl> [quality option] [output option]\n"
        "\n"
        "  <input.stl>  Path to the input STL file to decimate. Required.\n"
        "\n"
        "QUALITY OPTIONS  (choose one; default is --ExtraHigh)\n"
        "\n"
        "  --ExtraHigh\n"
        "      Removes very few triangles. The result looks nearly identical to the\n"
        "      original. Use this when quality is critical.\n"
        "\n"
        "  --High\n"
        "      Removes a moderate number of triangles with minimal visible difference.\n"
        "      Good general-purpose choice for most models.\n"
        "\n"
        "  --Medium\n"
        "      Removes a significant number of triangles. Small surface details may\n"
        "      be slightly smoothed out.\n"
        "\n"
        "  --Low\n"
        "      Aggressive reduction. Visible loss of fine detail. Use when file size\n"
        "      matters more than accuracy.\n"
        "\n"
        "  --ExtraLow\n"
        "      Maximum reduction. The model shape is only roughly preserved.\n"
        "      Useful for preview meshes or LOD (level-of-detail) purposes.\n"
        "\n"
        "  --ratio <number>\n"
        "      Specify exactly what percentage of the original triangles to keep.\n"
        "      Accepts a number between 0 and 100.\n"
        "      Example: --ratio 25  keeps 25%% of triangles (removes 75%%).\n"
        "\n"
        "  --max-error <number>\n"
        "      Advanced option. Sets the maximum allowed quadric error threshold\n"
        "      directly. Lower values = higher quality. The presets above use these\n"
        "      values: ExtraHigh=0.001, High=0.01, Medium=0.1, Low=0.5, ExtraLow=1.0.\n"
        "      Use this if the presets are not fine-grained enough for your needs.\n"
        "\n"
        "OUTPUT OPTIONS  (choose one; default adds '_decimated' before the extension)\n"
        "\n"
        "  --outputfile <path>\n"
        "      Save the result to this exact file path.\n"
        "      Example: --outputfile C:\\output\\result.stl\n"
        "\n"
        "  --suffix <text>\n"
        "      Append this text to the original filename (before the .stl extension).\n"
        "      Default suffix is '_decimated' if neither --outputfile nor --suffix\n"
        "      is specified.\n"
        "      Example: --suffix _small  turns  model.stl  into  model_small.stl\n"
        "\n"
        "OTHER\n"
        "\n"
        "  --help\n"
        "      Show this help text and exit.\n"
        "\n"
        "EXAMPLES\n"
        "  decimator model.stl\n"
        "      Decimate with ExtraHigh quality. Saves as model_decimated.stl\n"
        "\n"
        "  decimator model.stl --Medium\n"
        "      Decimate with Medium quality. Saves as model_decimated.stl\n"
        "\n"
        "  decimator model.stl --High --suffix _high\n"
        "      Decimate with High quality. Saves as model_high.stl\n"
        "\n"
        "  decimator model.stl --ratio 25 --outputfile C:\\out\\small.stl\n"
        "      Keep 25%% of triangles. Save to a specific file path.\n"
        "\n"
    );
}

static std::string to_lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)tolower(c); });
    return s;
}

static std::string derive_output_path(const std::string& input, const std::string& suffix)
{
    auto dot = input.rfind('.');
    if (dot != std::string::npos && to_lower(input.substr(dot)) == ".stl")
        return input.substr(0, dot) + suffix + ".stl";
    return input + suffix + ".stl";
}

int main(int argc, char* argv[])
{
    // No arguments at all → show help
    if (argc < 2) {
        print_help();
        return 0;
    }

    std::string input_path;
    std::string output_path;
    std::string suffix    = "_decimated";
    float       ratio     = -1.f;
    float       max_error = -1.f;
    bool        use_ratio = false;
    bool        quality_set = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        std::string arg_lower = to_lower(arg);

        if (arg_lower == "--help" || arg_lower == "-h" || arg_lower == "/?") {
            print_help();
            return 0;
        }
        // Quality presets
        else if (arg_lower == "--extrahigh") {
            max_error   = PRESET_EXTRAHIGH;
            use_ratio   = false;
            quality_set = true;
        }
        else if (arg_lower == "--high") {
            max_error   = PRESET_HIGH;
            use_ratio   = false;
            quality_set = true;
        }
        else if (arg_lower == "--medium") {
            max_error   = PRESET_MEDIUM;
            use_ratio   = false;
            quality_set = true;
        }
        else if (arg_lower == "--low") {
            max_error   = PRESET_LOW;
            use_ratio   = false;
            quality_set = true;
        }
        else if (arg_lower == "--extralow") {
            max_error   = PRESET_EXTRALOW;
            use_ratio   = false;
            quality_set = true;
        }
        // Manual quality
        else if (arg_lower == "--ratio" && i + 1 < argc) {
            ratio       = (float)atof(argv[++i]);
            use_ratio   = true;
            quality_set = true;
        }
        else if (arg_lower == "--max-error" && i + 1 < argc) {
            max_error   = (float)atof(argv[++i]);
            use_ratio   = false;
            quality_set = true;
        }
        // Output options
        else if ((arg_lower == "--outputfile" || arg_lower == "--output") && i + 1 < argc) {
            output_path = argv[++i];
        }
        else if (arg_lower == "--suffix" && i + 1 < argc) {
            suffix = argv[++i];
        }
        // Positional: input file
        else if (arg[0] != '-' && input_path.empty()) {
            input_path = arg;
        }
        else {
            fprintf(stderr, "Error: unknown argument '%s'\n\n", arg.c_str());
            print_help();
            return 1;
        }
    }

    if (input_path.empty()) {
        fprintf(stderr, "Error: no input file specified.\n\n");
        print_help();
        return 1;
    }

    // Default quality: ExtraHigh
    if (!quality_set) {
        max_error = PRESET_EXTRAHIGH;
        use_ratio = false;
        printf("No quality setting provided, defaulting to --ExtraHigh (max-error %.3f)\n", max_error);
    }

    if (output_path.empty())
        output_path = derive_output_path(input_path, suffix);

    // --- Read ---
    printf("Reading:   %s\n", input_path.c_str());
    Slic3r::indexed_triangle_set its = Slic3r::read_stl(input_path);
    if (its.empty()) {
        fprintf(stderr, "Error: failed to read STL or file is empty.\n");
        return 1;
    }

    uint32_t input_tri  = (uint32_t)its.indices.size();
    uint32_t input_vert = (uint32_t)its.vertices.size();
    printf("Input:     %u triangles, %u vertices\n", input_tri, input_vert);

    // --- Determine target ---
    uint32_t target_count = 0;
    float    error_val    = 0.f;
    float*   error_ptr    = nullptr;

    if (use_ratio) {
        ratio        = std::max(0.f, std::min(100.f, ratio));
        target_count = (uint32_t)(input_tri * (ratio / 100.f));
        printf("Target:    %.1f%% -> %u triangles\n", ratio, target_count);
    } else {
        error_val = max_error;
        error_ptr = &error_val;
        printf("Target:    max-error %.4f\n", max_error);
    }

    // --- Decimate ---
    printf("Decimating...\n");
    int last_pct = -1;
    auto progress = [&last_pct](int pct) {
        if (pct != last_pct) {
            printf("\r  Progress: %3d%%", pct);
            fflush(stdout);
            last_pct = pct;
        }
    };

    Slic3r::its_quadric_edge_collapse(its, target_count, error_ptr, nullptr, progress);
    printf("\r  Progress: 100%%\n");

    // --- Report ---
    uint32_t output_tri  = (uint32_t)its.indices.size();
    uint32_t output_vert = (uint32_t)its.vertices.size();
    float    actual_pct  = input_tri > 0 ? (100.f * output_tri / input_tri) : 0.f;
    printf("Output:    %u triangles, %u vertices (%.1f%% of original)\n",
           output_tri, output_vert, actual_pct);

    // --- Write ---
    printf("Writing:   %s\n", output_path.c_str());
    Slic3r::write_stl(output_path, its);
    printf("Done.\n");

    return 0;
}
