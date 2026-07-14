cmake_minimum_required(VERSION 4.3.0)

project(crawler)

add_executable(crawler)
set_property(TARGET crawler PROPERTY CXX_STANDARD 20)

target_sources(crawler
    PRIVATE
        source/main.cpp
        source/Buffer.cpp
)

target_compile_definitions(crawler
    PRIVATE
)

add_executable(BufferTest)
set_property(TARGET BufferTest PROPERTY CXX_STANDARD 20)
target_sources(BufferTest
    PRIVATE
        test/BufferTest.cpp
        source/Buffer.cpp
)
enable_testing()
add_test(
    NAME BufferTest
    COMMAND $<TARGET_FILE:BufferTest>
)