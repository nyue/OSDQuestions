add_executable( osd_patchmap
  shape_utils.cpp
  patchmap_main.cpp
  )

target_link_libraries( osd_patchmap
  Boost::headers
  OpenSubdiv::osdCPU_static
  OpenSubdiv::osdGPU_static
  )


add_executable(original_far_5_1
  original_far_tutorial_5_1.cpp
  )

target_link_libraries(original_far_5_1
  Boost::headers
  OpenSubdiv::osdCPU_static
  OpenSubdiv::osdGPU_static
  )

add_executable(dy_far_5_1
  david_yu_suggestions_far_tutorial_5_1.cpp
  shape_utils.cpp
  )

target_link_libraries(dy_far_5_1
  Boost::headers
  OpenSubdiv::osdCPU_static
  OpenSubdiv::osdGPU_static
  )
