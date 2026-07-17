cmake_minimum_required(VERSION 4.3.0)

project(crawler)

add_subdirectory(core)
add_subdirectory(sandbox)

include(CTest)
add_subdirectory(test)