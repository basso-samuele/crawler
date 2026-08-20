include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest
  GIT_TAG        063de7e9578f82b369302001269680b4b1553359
)

FetchContent_MakeAvailable(googletest)

add_executable(_test)
set_target_properties(_test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
target_sources(_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer/namedEntities.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer/numericEntities.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer/unicodeChars.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer/test1.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer/test2.cpp
    PUBLIC
    FILE_SET HEADERS
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/QueueTest.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/StringTest.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/UTF8StreamTest.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/TrieTest.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer/LexerTest.hpp
)
target_link_libraries(_test
    PRIVATE
        core
        gtest_main
)
add_test(
    NAME _test
    COMMAND $<TARGET_FILE:_test>
)