# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/QtImageOverlay_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/QtImageOverlay_autogen.dir/ParseCache.txt"
  "QtImageOverlay_autogen"
  )
endif()
