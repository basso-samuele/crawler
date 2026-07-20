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

add_executable(EncodingTest)
target_sources(EncodingTest
    PRIVATE
        EncodingTest.cpp
)
target_link_libraries(EncodingTest
    PRIVATE
        core
)
target_compile_features(EncodingTest
    PUBLIC
        cxx_std_20
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
    NAME EncodingTest
    COMMAND $<TARGET_FILE:EncodingTest>
)