@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
cl /nologo /std:c++17 /EHsc /O2 src\dev\netplay\NetdumpTool.cpp /Fo:build86\lib\NetdumpTool.obj /Fe:build86\bin\netdump_tool.exe
