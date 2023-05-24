# CMake Project configuration was modeled after: https://github.com/gabime/spdlog

# Configure CMAKE Version.
cmake_minimum_required(VERSION 3.1...3.18)

# -----------------------------------------------------------------------------
# Start CSYS project.
# -----------------------------------------------------------------------------
include(cmake/csys_utils.cmake)

# Configure library version
set(CSYS_MAJOR_VERSION 1)
set(CSYS_MINOR_VERSION 0)
set(CSYS_PATCH_VERSION 0)
set(CSYS_VERSION
        ${CSYS_MAJOR_VERSION}.${CSYS_MINOR_VERSION}.${CSYS_PATCH_VERSION})

# Add Custom In-Game CLI Project.
project(csys VERSION ${CSYS_VERSION} LANGUAGES CXX)
message(STATUS "Build CSYS: ${PROJECT_VERSION}")

# Configure csys_version.h
configure_file(
        "${PROJECT_SOURCE_DIR}/cmake/csys_version.h.in"
        "${PROJECT_BINARY_DIR}/include/csys/version.h"
)

# Setup module subdirectory.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

# For installation.
include(GNUInstallDirs)

# -----------------------------------------------------------------------------
# Set default build to debug
# -----------------------------------------------------------------------------

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose Release or Debug" FORCE)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_DEBUG_POSTFIX D)
endif()

# -----------------------------------------------------------------------------
# Compiler config
# -----------------------------------------------------------------------------

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif ()

# -----------------------------------------------------------------------------
# Build Settings
# -----------------------------------------------------------------------------

# Include tests.
option(CSYS_BUILD_EXAMPLES "Include and build CSYS examples targets" OFF) # ON

# Build shared option
option(CSYS_BUILD_SHARED "Build shared library" OFF) # ON

# Precompiled headers option
option(CSYS_ENABLE_PCH "Build library using precompiled headers to improve compilation times" OFF) # ON

# Testing options.
option(CSYS_BUILD_TESTS "Build tests" OFF) # ON

# CSYS compiler warnings
option(CSYS_BUILD_WARNINGS "Enable compiler warnings" OFF) # ON

# Install options
option(CSYS_INSTALL "Generate the install target" OFF)

# - Tools -

# Include-What-You-Use
option(CSYS_ENABLE_IWYU "Enables the usage of include what you use" OFF) # ON
set(CSYS_IWYU_CLANG_VERSION "9" CACHE STRING "Clang version that include-what-you-use will use") # 9

# Notify build type.
message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

# -----------------------------------------------------------------------------
# Build Static/Shared Library
# -----------------------------------------------------------------------------

# csys files
set(CSYS_SOURCES ${csys_SOURCE_DIR}/src/csys.cpp)
set(CSYS_HEADER_PATH "${csys_SOURCE_DIR}/include/csys")
set(CSYS_HEADERS
        "${CSYS_HEADER_PATH}/csys.h"
        "${CSYS_HEADER_PATH}/autocomplete.h"
        "${CSYS_HEADER_PATH}/arguments.h"
        "${CSYS_HEADER_PATH}/command.h"
        "${CSYS_HEADER_PATH}/string.h"
        "${CSYS_HEADER_PATH}/system.h"
        "${CSYS_HEADER_PATH}/exceptions.h"
        "${CSYS_HEADER_PATH}/item.h"
        "${CSYS_HEADER_PATH}/argument_parser.h"
        "${CSYS_HEADER_PATH}/system.h")

# Add core csys target.
if (CSYS_BUILD_SHARED)
    add_library(csys SHARED ${CSYS_SOURCES} ${CSYS_HEADERS})
    target_compile_definitions(csys PUBLIC CSYS_SHARED_LIB)
else ()
    add_library(csys STATIC ${CSYS_SOURCES} ${CSYS_HEADERS})
    target_compile_definitions(csys PUBLIC CSYS_STATIC_LIB)  # Used for export macros.
endif ()

# Enable warnings.
csys_enable_warnings(csys)

# Define csys namespace
add_library(csys::csys ALIAS csys)

# By default we are compiling this library.
target_compile_definitions(csys PUBLIC CSYS_COMPILED_LIB)

# Library public include directories
target_include_directories(csys PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
                                                      "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

# Set precompiled headers if supported and enabled
if (COMMAND target_precompile_headers AND CSYS_ENABLE_PCH)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/csys_pch.h.in ${PROJECT_BINARY_DIR}/csys_pch.h @ONLY)
    target_precompile_headers(csys PRIVATE ${PROJECT_BINARY_DIR}/csys_pch.h)
endif ()

# Export no symbols by default. (To ensure cross-compiler consistent behaviour)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# -----------------------------------------------------------------------------
# Header only version
# -----------------------------------------------------------------------------

# Add header version csys
add_library(csys_header_only INTERFACE)
add_library(csys::csys_header_only ALIAS csys_header_only)

# Set sources/includes
target_include_directories(csys_header_only INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
                                                          "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

# Link csys private dependencies

# -----------------------------------------------------------------------------
# Development tools
# -----------------------------------------------------------------------------

if (CMAKE_BUILD_TYPE STREQUAL Debug)

    # Enable include-what-you-use
    if (CSYS_ENABLE_IWYU)
        # Setup repo and build binaries.
        include(cmake/csys_configure_iwyu.cmake)
        csys_setup_include_what_you_use(${CSYS_IWYU_CLANG_VERSION})

        # Enable include-what-you-use built in CMake support.
        set(IWYU_PATH "${CMAKE_SOURCE_DIR}/thirdparty/include-what-you-use/build/bin/include-what-you-use")
        set_target_properties(csys PROPERTIES CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
    endif ()

endif ()

# -----------------------------------------------------------------------------
# Build binaries
# -----------------------------------------------------------------------------

if (CSYS_BUILD_EXAMPLES)
    message(STATUS "Generating Example(s)")
    add_subdirectory(examples)
endif ()

if (CSYS_BUILD_TESTS)
    message(STATUS "Generating tests")
    enable_testing()
    add_subdirectory(tests)
endif ()

# -----------------------------------------------------------------------------
# Install CSYS
# -----------------------------------------------------------------------------

if (CSYS_INSTALL)

    message(STATUS "Generating install")

    # Install paths
    set(CONFIG_TARGETS_FILE "csysConfig.cmake")
    set(VERSION_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/csysConfigVersion.cmake")
    set(EXPORT_DEST_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/csys")

    # -------------------------------------------------------------------------
    # Install targets
    # -------------------------------------------------------------------------
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(TARGETS csys csys_header_only
            EXPORT csys_targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    # --------------------------------------------------------------------------
    # Install Config files and Export targets
    # --------------------------------------------------------------------------

    # Install CSYS Version config.
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
            ${VERSION_CONFIG_FILE}
            VERSION ${CSYS_VERSION}
            COMPATIBILITY AnyNewerVersion
    )
    install(FILES "${VERSION_CONFIG_FILE}" DESTINATION "${EXPORT_DEST_DIR}")
    install(FILES "${PROJECT_BINARY_DIR}/include/csys/version.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/csys/)

    # Install CSYS Targets.
    install(EXPORT csys_targets
            NAMESPACE csys::
            FILE ${CONFIG_TARGETS_FILE}
            DESTINATION ${EXPORT_DEST_DIR})

endif()

# TODO: Add IDE support for headers. (source_group)