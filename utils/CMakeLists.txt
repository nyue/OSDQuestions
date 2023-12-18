add_library(utils
  far_utils.cpp
  shape_utils.cpp
  )

target_link_libraries(utils
  OpenSubdiv::osdCPU_static
  OpenSubdiv::osdGPU_static
  )
