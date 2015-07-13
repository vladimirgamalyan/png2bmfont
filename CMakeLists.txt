cmake_minimum_required(VERSION 3.2)
project(png2bmfont)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp MaxRectsBinPack/MaxRectsBinPack.cpp)
add_executable(png2bmfont ${SOURCE_FILES})
