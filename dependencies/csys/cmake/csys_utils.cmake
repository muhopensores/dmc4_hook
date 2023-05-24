# Reference: https://github.com/gabime/spdlog/blob/v1.x/cmake/utils.cmake

function(csys_enable_warnings target_name)
    if (CSYS_BUILD_WARNINGS)
        message(STATUS "Enabled warnings for target: ${target_name}")

        # Get MSVC compiler options.
        if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            list(APPEND MSVC_OPTIONS "/W4" "/w14640" "/permissive-")

            if (MSVC_VERSION GREATER 1900) # Allow non fatal security warnings for msvc 2015
                list(APPEND MSVC_OPTIONS "/WX")
            endif ()
        endif ()

        # Set compiler options.
        target_compile_options(
                ${target_name}
                PRIVATE $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
                -Wall
                -Wextra
                -Wconversion
                -pedantic
                -Wfatal-errors>
                $<$<CXX_COMPILER_ID:MSVC>:${MSVC_OPTIONS}>)
    endif()
endfunction()