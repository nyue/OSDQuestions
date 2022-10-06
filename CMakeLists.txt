cmake_minimum_required(VERSION 3.12)

project(OSDQuestions)

find_package(OpenSubdiv CONFIG REQUIRED)

include_directories(${OpenSubdiv_INCLUDE_DIR})

add_subdirectory(utils)

include_directories(.)

add_subdirectory(limit_stencil_with_fvar_uv)
