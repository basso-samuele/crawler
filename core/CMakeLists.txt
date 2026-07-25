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

target_compile_features(core
    PUBLIC
        cxx_std_20
)

target_compile_definitions(core
    PUBLIC
        -DCRAWLERLOGDIR="${CMAKE_CURRENT_BINARY_DIR}/log/"
)

target_sources(core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Encoding.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FileInputStream.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/InputStream.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/SPDLog.cpp
    PUBLIC
    FILE_SET HEADERS
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/Encoding.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/EncodingLabels.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FileInputStream.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/InputStream.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Log.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Patterns.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Singleton.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ServiceLocator.hpp
)