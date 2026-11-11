REM locate the VS install with vswhere, then load the 32-bit (x86) MSVC environment
set "VSROOT="
for /f "usebackq delims=" %%i in (`vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%i"
if not defined VSROOT (
  echo No Visual Studio install with the C++ x86/x64 toolchain found. 1>&2
  exit /b 1
)
call "%VSROOT%\VC\Auxiliary\Build\vcvars32.bat" >nul

mkdir build_ninja
cd build_ninja
cmake -DCMAKE_CXX_COMPILER="cl.exe" -DCMAKE_C_COMPILER="cl.exe" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja ..
cmake --build . --config Debug
REM cmake writes compile_commands.json into build_ninja (CMAKE_EXPORT_COMPILE_COMMANDS=ON); move it to the project root so clangd finds it
move /Y compile_commands.json ..
cd ..
PAUSE
