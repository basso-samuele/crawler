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
        Log.cpp
        Buffer.cpp
        Encoding.cpp
    PUBLIC
    FILE_SET HEADERS
    FILES
        Log.hpp
        Singleton.hpp
        Buffer.hpp
        DataSource.hpp
        Encoding.hpp
)

if (LINUX)
    target_sources(core
        PRIVATE
            DataSourceLinux.cpp
    )
elseif (WIN32)
    target_sources(core
        PRIVATE
            DataSourceWindows.cpp
    )
else ()
    message(FATAL_ERROR "Unsupported OS")
endif ()