add_library(core)

add_subdirectory(vendor/cjson)

target_compile_definitions(core
    PUBLIC
        $<$<CONFIG:Debug>:CRAWLER_DEBUG>
        CRAWLER_CHAR_REF_TRIE="${CMAKE_CURRENT_BINARY_DIR}/entities.bin"
)

target_sources(core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/attributes.c
        ${CMAKE_CURRENT_SOURCE_DIR}/named_ref.c
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer.c
        ${CMAKE_CURRENT_SOURCE_DIR}/parser.c
        ${CMAKE_CURRENT_SOURCE_DIR}/queue.c
        ${CMAKE_CURRENT_SOURCE_DIR}/stream.c
        ${CMAKE_CURRENT_SOURCE_DIR}/string_buffer.c
        ${CMAKE_CURRENT_SOURCE_DIR}/token.c
        ${CMAKE_CURRENT_SOURCE_DIR}/trie.c
        ${CMAKE_CURRENT_SOURCE_DIR}/utils.c
    PUBLIC
    FILE_SET HEADERS
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/attributes.h
        ${CMAKE_CURRENT_SOURCE_DIR}/buffer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/named_ref.h
        ${CMAKE_CURRENT_SOURCE_DIR}/error.h
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/parser.h
        ${CMAKE_CURRENT_SOURCE_DIR}/queue.h
        ${CMAKE_CURRENT_SOURCE_DIR}/stream.h
        ${CMAKE_CURRENT_SOURCE_DIR}/string_buffer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/tags.h
        ${CMAKE_CURRENT_SOURCE_DIR}/token.h
        ${CMAKE_CURRENT_SOURCE_DIR}/trie.h
        ${CMAKE_CURRENT_SOURCE_DIR}/utils.h
)

add_executable(genentities)

target_sources(genentities
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/genentities.c
)

target_link_libraries(genentities
    PUBLIC
        core
        cjson
)

target_compile_definitions(genentities
    PUBLIC
        CRAWLER_FILE_ENTITIES="${CMAKE_CURRENT_SOURCE_DIR}/entities.json"
        CRAWLER_CHAR_REF_TRIE="${CMAKE_CURRENT_BINARY_DIR}/entities.bin"
)

add_custom_command(TARGET genentities POST_BUILD
    COMMAND $<TARGET_FILE:genentities>
    COMMENT "Generating entities..."
)