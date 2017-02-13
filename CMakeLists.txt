cmake_minimum_required (VERSION 3.1.0)
project (datadogcpp)

set (CMAKE_CXX_STANDARD 11)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include_directories(include)

set(libsrc src/trace_dogstatsd.cpp)

## we could do the following to compile just once, but I don't want
# the static compiled with -PIC, so build twice.
# add_library(objlib OBJECT ${libsrc})
#
# add_library (datadogcpp-static STATIC $<TARGET_OBJECTS:objlib>)
# add_library (datadogcpp-shared SHARED $<TARGET_OBJECTS:objlib>)

add_library (datadogcpp-static STATIC ${libsrc})
add_library (datadogcpp-shared SHARED ${libsrc})
