add_executable(far_tutorial_2_2
  far_tutorial_2_2.cpp
  )

target_link_libraries(far_tutorial_2_2
  Boost::headers
  OpenSubdiv::osdCPU_static
  OpenSubdiv::osdGPU_static
  )
