add_executable(limit_stencil_with_fvar_uv
  bfr_tutorial_1_3.cpp
  )

target_link_libraries(limit_stencil_with_fvar_uv
  OpenSubdiv::osdCPU_static
  OpenSubdiv::osdGPU_static
  utils
  )
