set(TARGET test-backend)

configure_file(config.cppm.in "${CMAKE_CURRENT_BINARY_DIR}/config.cppm")

add_executable(${TARGET}
    main.cpp
)
target_compile_features(${TARGET} PUBLIC cxx_std_23)
if (CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${TARGET} PUBLIC -fmodules-ts)
endif()
target_sources(${TARGET} PRIVATE
    FILE_SET gpt4all_backend TYPE CXX_MODULES BASE_DIRS
        "${CMAKE_CURRENT_BINARY_DIR}"
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/config.cppm"
)
gpt4all_add_warning_options(${TARGET})
target_link_libraries(${TARGET} PRIVATE
    gpt4all-backend
)
