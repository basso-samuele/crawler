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
    NAME EncodingTest
    COMMAND $<TARGET_FILE:EncodingTest>
)