#include(ExternalProject)
find_package(Git REQUIRED)

if (NOT Git_FOUND)
	message(FATAL_ERROR "Git not found!")
endif ()

function(csys_setup_include_what_you_use CLANG_VERSION)

    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/thirdparty/include-what-you-use")
        # Info.
        message(STATUS "include-what-you-use clang setup version: ${CLANG_VERSION}")

        # Cloning IWYU.
        message(STATUS "Cloning include-what-you-use")
        execute_process(COMMAND ${GIT_EXECUTABLE} clone https://github.com/include-what-you-use/include-what-you-use.git
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/thirdparty/
                RESULT_VARIABLE GIT_RESULT)

        # Failed cloning.
        if(NOT EXISTS "${PROJECT_SOURCE_DIR}/thirdparty/include-what-you-use")
            message(FATAL_ERROR "git clone update https://github.com/include-what-you-use/include-what-you-use.git failed with ${GIT_RESULT}, delete library folder an try again.")
        endif()

        # Checkout clang_${CLANG_VERSION}.0 branch
        execute_process(COMMAND ${GIT_EXECUTABLE} checkout clang_${CLANG_VERSION}.0
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/thirdparty/include-what-you-use"
                RESULT_VARIABLE GIT_RESULT)

        # Start building IWYU
        message(STATUS "Building include-what-you-use")
        execute_process(COMMAND mkdir build
                        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/thirdparty/include-what-you-use)
        execute_process(COMMAND cmake -G "Unix Makefiles" -DCMAKE_PREFIX_PATH=/usr/lib/llvm-${CLANG_VERSION} ..
                        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/thirdparty/include-what-you-use/build)
        execute_process(COMMAND make
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/thirdparty/include-what-you-use/build)
    endif()
endfunction()