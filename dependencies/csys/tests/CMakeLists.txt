# Configure CMAKE Version.
cmake_minimum_required(VERSION 3.1...3.16)

# Start test project.
project(csys_tests LANGUAGES CXX)

# Stand alone build (TODO: Read more about this.
if(NOT TARGET csys)
    find_package(csys REQUIRED)
endif()

# Utilities.
include(../cmake/csys_utils.cmake)

# Sources.
set(CSYS_TEST_SOURCES
        test_autocomplete.cpp
        test_system.cpp
        test_string_argument.cpp
        test_char_argument.cpp
        test_item.cpp
        test_history.cpp
        main.cpp)

# Add Script test only if filesystem is found.
cmake_policy(SET CMP0057 NEW)
find_package(Filesystem)
if (Filesystem_FOUND)
    message(STATUS "Including csys::Script test")
    #list(APPEND CSYS_TEST_SOURCES test_script.cpp)
endif()

# Set up testing.
enable_testing()

set(IWYU_PATH "${CMAKE_SOURCE_DIR}/thirdparty/include-what-you-use/build/bin/include-what-you-use")

# Helper func.
function(csys_prepare_test test_target csys_lib)
    add_executable(${test_target} ${CSYS_TEST_SOURCES})
    csys_enable_warnings(${test_target})
    target_link_libraries(${test_target} PRIVATE ${csys_lib})
    if (CSYS_ENABLE_IWYU)
        set_target_properties(${test_target} PROPERTIES CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
    endif()
    add_test(NAME ${test_target} COMMAND ${test_target} -s -fc)
endfunction()

# Build tests with csys library.
if (CSYS_BUILD_TESTS)
    csys_prepare_test(csys_tests csys::csys)
endif()