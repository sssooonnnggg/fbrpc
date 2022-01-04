cmake_minimum_required(VERSION 3.16)
project(fbrpc)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use")

if(NOT "${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
    set(USE_VCPKG TRUE)
    message("USE_VCPKG: " ${USE_VCPKG})
endif()

if(USE_VCPKG)
  function (createSrcGroups ROOT FILE_LIST)
    string (LENGTH ${ROOT} curDirLen)
    set (TMP_FILE_LIST ${${FILE_LIST}})

    foreach(SOURCE ${TMP_FILE_LIST} )
      string(LENGTH ${SOURCE} fullPathLen)
      math(EXPR RelPathLen ${fullPathLen}-${curDirLen})
      string(SUBSTRING ${SOURCE} ${curDirLen} ${RelPathLen} curStr)

      string(REGEX REPLACE "[\\/]" "\\\\" normPath ${curStr} )
      string(REGEX MATCH "\\\\(.*)\\\\" ouput ${normPath} )
      if (NOT CMAKE_MATCH_1 STREQUAL "")
        source_group(${CMAKE_MATCH_1} FILES ${SOURCE} )
      else ()
        source_group("" FILES ${SOURCE}) 
      endif ()
    endforeach ()
  endfunction ()
endif()

set(FBRPC_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(LIB_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/fbrpc)

add_subdirectory(fbrpc)
add_subdirectory(tests)
add_subdirectory(generator)