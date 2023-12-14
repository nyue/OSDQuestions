add_executable(far_tutorial_1_2
  far_tutorial_1_2.cpp
  )

target_link_libraries(far_tutorial_1_2
  Boost::headers
  OpenSubdiv::osdCPU_static
  OpenSubdiv::osdGPU_static
  )
