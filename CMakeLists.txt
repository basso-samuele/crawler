cmake_minimum_required(VERSION 4.3.0)

project(crawler C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_subdirectory(core)

include(CTest)
add_subdirectory(test)