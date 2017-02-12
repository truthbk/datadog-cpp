cmake_minimum_required (VERSION 3.1.0)
project (datadogcpp)

set (CMAKE_CXX_STANDARD 11)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include_directories(include)

add_library (datadogcpp SHARED src/trace_dogstatsd.cpp)
