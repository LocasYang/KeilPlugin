# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\KeilPluginCoreTest_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\KeilPluginCoreTest_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\KeilPluginStudio_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\KeilPluginStudio_autogen.dir\\ParseCache.txt"
  "KeilPluginCoreTest_autogen"
  "KeilPluginStudio_autogen"
  )
endif()
