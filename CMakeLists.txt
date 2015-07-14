cmake_minimum_required(VERSION 2.8.7)
project(png2bmfont)

set(BOOST_MIN_VERSION "1.46.0")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

if (WIN32)
    set(BOOST_ROOT "c:\\boost_1_58_0")
endif()

find_package(Boost ${BOOST_MIN_VERSION} REQUIRED COMPONENTS system filesystem program_options)
include_directories(${Boost_INCLUDE_DIR})

set(SOURCE_FILES main.cpp MaxRectsBinPack/MaxRectsBinPack.cpp lodepng/lodepng.cpp tinyxml2/tinyxml2.cpp)

add_executable(png2bmfont ${SOURCE_FILES})
target_link_libraries(png2bmfont ${Boost_LIBRARIES})
