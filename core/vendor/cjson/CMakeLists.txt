add_library(cjson)

target_sources(cjson
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/cJSON.c
    PUBLIC
    FILE_SET HEADERS
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/cJSON.h
)