cmake_minimum_required(VERSION 2.8)
project(double8scan)
enable_language(C)

add_subdirectory(third_party/libjpeg-turbo)
include_directories(third_party/libjpeg-turbo)
include_directories(${CMAKE_BINARY_DIR}/third_party/libjpeg-turbo)

set(SOURCES
    double8scan.c)

if(WIN32)
  set(SOURCES ${SOURCES}
      third_party/getopt-for-windows/getopt.c)
  include_directories(third_party/getopt-for-windows)
endif()

add_executable(double8scan ${SOURCES})
add_dependencies(double8scan jpeg-static)
target_link_libraries(double8scan $<TARGET_LINKER_FILE:jpeg-static>)
