# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/music_player_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/music_player_autogen.dir/ParseCache.txt"
  "music_player_autogen"
  )
endif()
