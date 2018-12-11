cmake_minimum_required(VERSION 2.8)
project(double8scan)
enable_language(C)

find_package(JPEG)

include_directories(${JPEG_INCLUDE_DIRS})

add_executable(double8scan double8scan.c)
target_link_libraries(double8scan ${JPEG_LIBRARIES})
