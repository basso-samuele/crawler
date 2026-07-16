cmake_minimum_required(VERSION 4.3.0)

project(crawler)

add_executable(crawler)
set_property(TARGET crawler PROPERTY CXX_STANDARD 20)

target_sources(crawler
    PRIVATE
        source/main.cpp
        source/Buffer.cpp
        source/FileDataSource.cpp
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

add_executable(FileDataSourceTest)
set_property(TARGET FileDataSourceTest PROPERTY CXX_STANDARD 20)
target_sources(FileDataSourceTest
    PRIVATE
        test/FileDataSourceTest.cpp
        source/FileDataSource.cpp
)

enable_testing()
add_test(
    NAME BufferTest
    COMMAND $<TARGET_FILE:BufferTest>
)
add_test(
    NAME FileDataSourceTest
    COMMAND $<TARGET_FILE:FileDataSourceTest>
)