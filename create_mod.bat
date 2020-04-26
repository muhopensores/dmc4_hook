@echo off
set /p filename="Enter filename like modName:"
type nul > src\dmc4_hook\mods\%filename%.cpp
type nul > src\dmc4_hook\mods\%filename%.hpp
notepad src\dmc4_hook\CMakeLists.txt