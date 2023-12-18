add_executable(bfr_tutorial_1_3
  bfr_tutorial_1_3.cpp
  )

target_link_libraries(bfr_tutorial_1_3
  utils
  Boost::headers
  OpenSubdiv::osdCPU_static
  OpenSubdiv::osdGPU_static
  )
