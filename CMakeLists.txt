cmake_minimum_required(VERSION 2.8)
project(double8scan)
enable_language(C)

find_package(JPEG)

include_directories(${JPEG_INCLUDE_DIRS})

set(SOURCES
    double8scan.c)

if(WIN32)
  set(SOURCES ${SOURCES}
      third_party/getopt-for-windows/getopt.c)
  include_directories(third_party/getopt-for-windows)
endif()

add_executable(double8scan ${SOURCES})
target_link_libraries(double8scan ${JPEG_LIBRARIES})
