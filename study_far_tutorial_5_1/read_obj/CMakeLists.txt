add_executable(obj_far_tutorial_5_1
  obj_far_tutorial_5_1.cpp
  )

target_link_libraries(obj_far_tutorial_5_1
  Boost::headers
  OpenSubdiv::osdCPU_static
  OpenSubdiv::osdGPU_static
  )
