add_executable( osd_patchmap
  shape_utils.cpp
  patchmap_main.cpp
  )

target_link_libraries( osd_patchmap
  Boost::headers
  OpenSubdiv::osdCPU_static
  OpenSubdiv::osdGPU_static
  )


