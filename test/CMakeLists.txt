add_executable(BufferTest)
target_sources(BufferTest
    PRIVATE
        BufferTest.cpp
)
target_link_libraries(BufferTest
    PRIVATE
        core
)
target_compile_features(BufferTest
    PUBLIC
        cxx_std_20
)

add_executable(FileDataSourceTest)
target_sources(FileDataSourceTest
    PRIVATE
        FileDataSourceTest.cpp
)
target_link_libraries(FileDataSourceTest
    PRIVATE
        core
)
target_compile_features(FileDataSourceTest
    PUBLIC
        cxx_std_20
)

add_executable(BOMTest)
target_sources(BOMTest
    PRIVATE
        BOMTest.cpp
)
target_link_libraries(BOMTest
    PRIVATE
        core
)
target_compile_features(BOMTest
    PUBLIC
        cxx_std_20
)
set(ASSETS_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets)
target_compile_definitions(BOMTest
    PRIVATE
        -DBOM_TEST_INDEX="${ASSETS_TEST_DIR}/index.html"
        -DBOM_TEST_UTF8="${ASSETS_TEST_DIR}/utf8.html"
        -DBOM_TEST_UTF16BE="${ASSETS_TEST_DIR}/utf16be.html"
        -DBOM_TEST_UTF16LE="${ASSETS_TEST_DIR}/utf16le.html"
)

add_test(
    NAME BufferTest
    COMMAND $<TARGET_FILE:BufferTest>
)
add_test(
    NAME FileDataSourceTest
    COMMAND $<TARGET_FILE:FileDataSourceTest>
)
add_test(
    NAME BOMTest
    COMMAND $<TARGET_FILE:BOMTest>
)