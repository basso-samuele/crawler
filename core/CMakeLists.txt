add_library(core)

include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        486b55554f11c9cccc913e11a87085b2a91f706f
)

FetchContent_MakeAvailable(spdlog)

target_link_libraries(core
    PUBLIC
        spdlog
)

target_compile_definitions(core
    PUBLIC
        -DCRAWLERLOGDIR="${CMAKE_CURRENT_BINARY_DIR}/log/"
)

target_sources(core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/FileStream.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/SPDLog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Pipeline.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Decoder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Tokenizer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Definitions.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Tree.cpp
    PUBLIC
    FILE_SET HEADERS
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/FileStream.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Stream.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Log.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Patterns.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Singleton.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ServiceLocator.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Pipeline.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Decoder.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/TransactionalStream.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Definitions.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Tokenizer.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Tree.hpp
)