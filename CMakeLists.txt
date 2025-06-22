cmake_minimum_required(VERSION 3.6)

project(dmc4_hook)

#add_compile_options(/MT)

include_directories(${CMAKE_SOURCE_DIR}/dependencies)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/imgui)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/minhook/include)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/spdlog/include)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/csys/include)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/glm)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/debug-draw)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/ImGuizmo)

include_directories($ENV{DXSDK_DIR}/Include)
link_directories("$ENV{DXSDK_DIR}/Lib/x86")

# set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-format-style='file'")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:threadSafeInit- /MP /std:c++17")

# for msvc hot reload
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	set(CMAKE_CXX_FLAGS_DEBUG "/MDd /ZI /Ob0 /Od /RTC1")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")
endif()

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

# Disable exceptions
#string(REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#add_compile_definitions(_HAS_EXCEPTIONS=0)

set(LOG_FILENAME "${CMAKE_PROJECT_NAME}_framework_log.txt")
set(LOG_ENTRY "${CMAKE_PROJECT_NAME} ModFramework entry")
set(CONFIG_FILENAME "${CMAKE_PROJECT_NAME}.cfg")

configure_file(Config.hpp.in ${CMAKE_SOURCE_DIR}/src/Config.hpp @ONLY)
#add_compile_options(/W0)
#add_compile_options(/MT)

# Statically compile runtime
string(REGEX REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
string(REGEX REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}") # minhook

add_subdirectory(dependencies)
add_subdirectory(src)