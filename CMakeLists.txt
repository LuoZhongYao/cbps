cmake_minimum_required (VERSION 2.8)

project (cbps)

set(CMAKE_C_FLAGS_DEBUG "-gstabs+ -Werror" ) #"${CMAKE_CXX_FLAGS_DEBUG} -gstabs -Werror")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror")

add_definitions(-D__XAP__)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(include include/firmware)
link_directories(${CMAKE_BINARY_DIR}/lib)

add_subdirectory (lib)
add_subdirectory (kernel)
add_subdirectory (apps/sink)
