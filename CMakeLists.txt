set(name json-test)
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS -g -Wall)

cmake_minimum_required(VERSION 2.20)
project(${name})
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_library(json SHARED json.cpp)
add_executable(${name} test.cpp)
target_link_libraries(${name} json)
