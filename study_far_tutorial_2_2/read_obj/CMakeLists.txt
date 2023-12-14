add_executable(obj_far_tutorial_2_2
  obj_far_tutorial_2_2.cpp
  )

target_link_libraries(obj_far_tutorial_2_2
  utils
  Boost::headers
  OpenSubdiv::osdCPU_static
  OpenSubdiv::osdGPU_static
  )
