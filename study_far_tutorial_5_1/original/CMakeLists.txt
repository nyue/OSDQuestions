add_executable(far_tutorial_5_1
  far_tutorial_5_1.cpp
  )

target_link_libraries(far_tutorial_5_1
  Boost::headers
  OpenSubdiv::osdCPU_static
  OpenSubdiv::osdGPU_static
  )
