cmake_minimum_required(VERSION 3.14)

SET(CMAKE_CXX_STANDARD 2a)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(test,test.cpp)