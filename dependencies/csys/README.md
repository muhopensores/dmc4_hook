# CSYS
<table>
    <tr>
        <td>
            master branch
        </td>
        <td>
            Linux/OSX <a href="https://travis-ci.com/rmxbalanque/csys"><img src="https://travis-ci.com/rmxbalanque/csys.svg?branch=master"></a>
        </td>
        <td> 
            Windows <a href="https://ci.appveyor.com/project/rmxbalanque/csys"><img src="https://ci.appveyor.com/api/projects/status/p5e3c6rdysatd6v9/branch/master?svg=true"></a>
        </td>
    </tr>
    <tr>
        <td>
            dev branch
        </td>
        <td>
            Linux/OSX <a href="https://travis-ci.com/rmxbalanque/csys"><img src="https://travis-ci.com/rmxbalanque/csys.svg?branch=development"></a>
        </td>
        <td>
            Windows <a href="https://ci.appveyor.com/project/rmxbalanque/csys"><img src="https://ci.appveyor.com/api/projects/status/p5e3c6rdysatd6v9/branch/development?svg=true"></a>
        </td>
    </tr>
</table>

[![MIT license](https://img.shields.io/badge/License-MIT-green.svg)](https://lbesson.mit-license.org/)
[![Language](https://img.shields.io/badge/language-C++-blue.svg)](https://isocpp.org/)
[![Standard](https://img.shields.io/badge/c%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)

Modern C++, header-only/compiled, custom console system library.

## Features
- Full and partial tab autocomplete with suggestions.
- Register variables to console system for easy modification.
- Command history support.
- Simple command scripts support.
- Custom data types console logging support. (overload << operator)
- Custom functions with arbitrary arguments registration support. 
- Header only or compiled.
- No external dependencies.

## Platforms
- The following platforms are supported:
    - __Windows__: Tested on Windows 10 Professional 64-bit. (__Static/Header-only__)
    - __Linux__: Tested on Ubuntu 18.04.4 LTS (__Static/Shared/Header-only__)
    
## Requirements:
- C++17 capable compiler:
    - Visual C++ 15 (Visual Studio 2017)
    - Clang 5
    - GCC 7
- CMake 3.10+

## Integration
If you wish to use the _csys library_ without CMake you can copy the [source folder](https://github.com/rmxbalanque/csys/tree/master/include/csys) and proceed to build with C++17 capable compiler. (_Header only version_)
      
### CMake
Targets:

- _Header-only_: Use the `csys::csys_header_only` interface library target.
- _Static/Shared_: Use the `csys::csys` library target together with the `-DCSYS_BUILD_SHARED=ON/OFF` cmake variable. 

#### External
To use this library from a CMake project, you can locate it by using the `find_package()` command and use the imported targets defined above.
```CMake
# CMakeLists.txt

find_package(csys 1.0.0 REQUIRED)
...
add_library(your_library ...)
...
target_link_libraries(your_library PRIVATE csys::csys)
```
#### Embedded
To embed the library into an existing CMake project, clone source and place it in a subdirectory and call `add_subdirectory()` in your `CMakeLists.txt`.
```CMake
# CMakeLists.txt

# By adding the subdirectory the csys::csys and csys::csys_header_only targets will be defined. 
# Note that the csys::csys target will be a static library by default.
add_subdirectory(csys)
...
add_library(your_library ...)
...
target_link_libraries(your_library PRIVATE csys::csys)
```

See example [CMakeLists.txt](https://github.com/rmxbalanque/csys/blob/master/examples/simple_example/CMakeLists.txt) on how to use.

## Documentation
Current documentation available on the [Wiki](https://github.com/rmxbalanque/csys/wiki). (WIP)
