REM update this to point to your vcvars32.bat path can be seen in "x86 Native Tools Command Prompt for VS 2022" shortcut properties
REM call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"

mkdir build86
cd build86
cmake -DCMAKE_CXX_COMPILER="cl.exe" -DCMAKE_C_COMPILER="cl.exe" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja ..
cmake --build . --config Debug
cd ..
PAUSE
