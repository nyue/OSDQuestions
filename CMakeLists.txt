cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(OSDQuestions)

find_package(VTK CONFIG REQUIRED)
find_package(Imath CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED)
find_package(OpenSubdiv CONFIG REQUIRED)

# include_directories(${OpenSubdiv_INCLUDE_DIR})

add_subdirectory(utils)

include_directories(.)

add_subdirectory(limit_stencil_with_fvar_uv)
add_subdirectory(patchmap)
add_subdirectory(far_tutorial)
add_subdirectory(bfr_tutorial)

