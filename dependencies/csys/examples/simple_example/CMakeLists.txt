# Configure simple example executable
if (NOT TARGET csys)
    # External integration
    find_package(csys REQUIRED)
endif()

# Examples of using pre-compiled library
add_executable(simple_example simple_example_main.cpp)
target_link_libraries(simple_example csys::csys)