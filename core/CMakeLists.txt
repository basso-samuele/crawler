add_library(core)

target_compile_definitions(core
    PUBLIC
        $<$<CONFIG:Debug>:CRAWLER_DEBUG>
)

target_sources(core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/attributes.c
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer.c
        ${CMAKE_CURRENT_SOURCE_DIR}/memory.c
        ${CMAKE_CURRENT_SOURCE_DIR}/parser.c
        ${CMAKE_CURRENT_SOURCE_DIR}/stream.c
        ${CMAKE_CURRENT_SOURCE_DIR}/string_buffer.c
        ${CMAKE_CURRENT_SOURCE_DIR}/token.c
        ${CMAKE_CURRENT_SOURCE_DIR}/utils.c
    PUBLIC
    FILE_SET HEADERS
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/attributes.h
        ${CMAKE_CURRENT_SOURCE_DIR}/buffer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/error.h
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/memory.h
        ${CMAKE_CURRENT_SOURCE_DIR}/parser.h
        ${CMAKE_CURRENT_SOURCE_DIR}/stream.h
        ${CMAKE_CURRENT_SOURCE_DIR}/string_buffer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/tags.h
        ${CMAKE_CURRENT_SOURCE_DIR}/token.h
        ${CMAKE_CURRENT_SOURCE_DIR}/utils.h
)