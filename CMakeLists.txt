cmake_minimum_required(VERSION 4.3.0)

project(crawler)

add_subdirectory(core)

include(CTest)
add_subdirectory(test)