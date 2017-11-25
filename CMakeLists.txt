cmake_minimum_required(VERSION 3.8)
project(ImageCompression)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenCV REQUIRED)
set(SOURCE_FILES main.cpp)
include_directories(${OpenCV_INCLUDE_DIRS})


add_executable(ImageCompression ${SOURCE_FILES})
target_link_libraries(ImageCompression ${OpenCV_LIBS})