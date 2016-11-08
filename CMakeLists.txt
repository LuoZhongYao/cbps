cmake_minimum_required (VERSION 2.8)

project (cbps)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(include include/firmware)
link_directories(${CMAKE_BINARY_DIR}/lib)

add_subdirectory (lib)
add_subdirectory (apps/sink)
