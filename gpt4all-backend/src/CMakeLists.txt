set(TARGET gpt4all-backend)

add_library(${TARGET} STATIC
    main.cpp
)
target_compile_features(${TARGET} PUBLIC cxx_std_23)
if (CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${TARGET} PUBLIC -fmodules-ts)
endif()
target_sources(${TARGET} PUBLIC
    FILE_SET gpt4all_backend TYPE CXX_MODULES FILES
    main.cppm
)
gpt4all_add_warning_options(${TARGET})
target_link_libraries(${TARGET} PUBLIC
    QCoro6::Coro Qt6::Core Qt6::Network
)
target_link_libraries(${TARGET} PRIVATE
    QCoro6::Network fmt::fmt
)
