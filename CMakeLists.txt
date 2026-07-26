cmake_minimum_required(VERSION 4.3.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(crawler)

add_subdirectory(core)

include(CTest)
add_subdirectory(test)