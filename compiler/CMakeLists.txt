cmake_minimum_required(VERSION 4.2)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

project(LibreFractals VERSION 0.1.0 LANGUAGES CXX)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)

if(EMSCRIPTEN)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
  target_compile_options(LibreFractals PRIVATE -O3)
  target_link_options(LibreFractals PRIVATE
    -lembind -sWASM=1 -sMODULARIZE=1 -sEXPORT_ES6=1 -sALLOW_MEMORY_GROWTH=1 -sINVOKE_RUN=0)
  set_target_properties(LibreFractals PROPERTIES SUFFIX ".js")
else()
  target_compile_options(LibreFractals PRIVATE
    $<$<CONFIG:Debug>:-g3;-O0;-fno-omit-frame-pointer;-DDEBUG;-fsanitize=address,undefined>
    $<$<CONFIG:Release>:-O3;-march=native;-DNDEBUG;-flto=auto>)
  target_link_options(LibreFractals PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>)
endif()
