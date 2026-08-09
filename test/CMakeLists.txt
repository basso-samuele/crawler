add_executable(_test)
target_sources(_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Utils.cpp
)
target_link_libraries(_test
    PRIVATE
        core
)
add_test(
    NAME _test
    COMMAND $<TARGET_FILE:_test>
)

add_executable(stream)
target_sources(stream
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Stream.cpp
)
target_link_libraries(stream
    PRIVATE
        core
)
target_compile_definitions(stream
    PRIVATE
        -DCRAWLER_STREAM_ASSET_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/stream.html"
)
add_test(
    NAME stream
    COMMAND $<TARGET_FILE:stream>
)