function(gpt4all_add_warning_options target)
    if (MSVC)
        return()
    endif()
    target_compile_options("${target}" PRIVATE
        # base options
        -Wall
        -Wextra
        # extra options
        -Wcast-align
        -Wextra-semi
        -Wformat=2
        -Wmissing-include-dirs
        -Wsuggest-override
        -Wvla
        # errors
        -Werror=format-security
        -Werror=init-self
        -Werror=pointer-arith
        -Werror=undef
        # disabled warnings
        -Wno-sign-compare
        -Wno-unused-parameter
    )
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options("${target}" PRIVATE
            -Wduplicated-branches
            -Wduplicated-cond
            -Wlogical-op
            -Wno-reorder
            -Wno-null-dereference
        )
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "^(Apple)?Clang$")
        target_compile_options("${target}" PRIVATE
            -Wunreachable-code-break
            -Wunreachable-code-return
            -Werror=pointer-integer-compare
            -Wno-reorder-ctor
        )
    endif()
endfunction()
