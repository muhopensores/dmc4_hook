mkdir build86
cd build86
cmake .. -G "Visual Studio 15 2017" -A "Win32"
cmake --build . --config Debug
cd ..