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
)

target_sources(core
    PRIVATE
        Buffer.cpp
    PUBLIC
    FILE_SET HEADERS
    FILES
        Buffer.h
        DataSource.h
)

if (LINUX)
    message(TRACE "Platform linux")
    add_subdirectory(platform/linux)
    target_include_directories(core
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/platform/linux
    )
elseif (WINDOWS)
    message(FATAL_ERROR "Missing implementation")
else ()
    message(FATAL_ERROR "Unsupported OS")
endif ()