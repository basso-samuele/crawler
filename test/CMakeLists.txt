add_executable(_test)
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