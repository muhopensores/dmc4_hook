mkdir build86
cd build86
cmake .. -G "Visual Studio 16 2019" -A "Win32"
cmake --build . --config Debug
cd ..