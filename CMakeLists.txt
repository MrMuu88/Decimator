cmake_minimum_required(VERSION 3.20)
project(decimator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_definitions(
        _USE_MATH_DEFINES
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        NDEBUG
    )
    add_compile_options(/W3 /MP /EHsc /O2 /MT)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/eigen
)

add_executable(decimator
    src/main.cpp
    src/stl_io.cpp
    src/QuadricEdgeCollapse.cpp
)

set_target_properties(decimator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}
)
