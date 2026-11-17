@echo off
rem Builds hello_plugin.dll (32-bit, release) with Visual Studio 2022's x86 tools and the hook's ImGui sources.
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
if errorlevel 1 call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
cd /d %~dp0
set IMGUI=..\..\dependencies\imgui
if not exist build mkdir build
cl /nologo /LD /O2 /MT /EHsc /std:c++20 /W3 /DIMGUI_DISABLE_OBSOLETE_FUNCTIONS /I..\include /I%IMGUI% hello_plugin.cpp %IMGUI%\imgui.cpp %IMGUI%\imgui_draw.cpp %IMGUI%\imgui_widgets.cpp %IMGUI%\imgui_tables.cpp /Fobuild\ /Febuild\hello_plugin.dll
echo build exit %errorlevel%
