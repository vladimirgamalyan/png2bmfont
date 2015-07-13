cmake_minimum_required(VERSION 2.8.7)
project(png2bmfont)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(BOOST_MIN_VERSION "1.58.0")

if (WIN32)
    set(BOOST_ROOT "c:\\boost_1_58_0")
endif()

find_package(Boost ${BOOST_MIN_VERSION} REQUIRED COMPONENTS system filesystem program_options)
include_directories(${Boost_INCLUDE_DIR})

set(SOURCE_FILES main.cpp MaxRectsBinPack/MaxRectsBinPack.cpp)

add_executable(png2bmfont ${SOURCE_FILES})
target_link_libraries(png2bmfont ${Boost_LIBRARIES})
