add_executable(_test)
set_target_properties(_test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
target_sources(_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Test.cpp
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
set_target_properties(stream PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
target_sources(stream
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Stream.cpp
)
target_link_libraries(stream
    PRIVATE
        core
)
add_test(
    NAME stream
    COMMAND $<TARGET_FILE:stream>
)