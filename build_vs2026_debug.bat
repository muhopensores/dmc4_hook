mkdir build86
cd build86
cmake .. -G "Visual Studio 18 2026" -A "Win32"
cmake --build . --config Debug
cd ..
PAUSE