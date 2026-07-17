add_executable(sandbox)

target_sources(sandbox
    PRIVATE
        main.cpp
)

target_link_libraries(sandbox
    PRIVATE
        core
)

target_compile_features(sandbox
    PUBLIC
        cxx_std_20
)