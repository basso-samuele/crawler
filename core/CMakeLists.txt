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
        $<$<CONFIG:Debug>:DEBUG>
        -DLOG_DIR="${CMAKE_CURRENT_BINARY_DIR}/log/"
)

target_sources(core
    PRIVATE
        data/Buffer.cpp
        utils/Log.cpp
    PUBLIC
    FILE_SET HEADERS
    FILES
        data/Buffer.h
        data/DataSource.h
        utils/Log.h
        utils/Singleton.h
)

if (LINUX)
    add_subdirectory(platform/linux)
    target_include_directories(core
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/platform/linux
    )
elseif (WIN32)
    add_subdirectory(platform/windows)
    target_include_directories(core
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/platform/windows
    )
else ()
    message(FATAL_ERROR "Unsupported OS")
endif ()