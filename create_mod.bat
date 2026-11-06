@echo off
set /p filename="Enter filename like ModName:"
type nul > src\mods\%filename%.cpp
type nul > src\mods\%filename%.hpp
start "" notepad src\CMakeLists.txt
echo Adding mods\%filename%.hpp to src\ModsHeaders.hpp
call:AddInclude src\ModsHeaders.hpp %filename%
echo Dont forget to add ADD_MOD(%filename%); into Mods.cpp Mods::Mods() - order matters
exit /b

:AddInclude
echo $f = '%1'>Inc.ps1
echo $inc = '#include "mods/%2.hpp"'>>Inc.ps1
echo $raw = [System.IO.File]::ReadAllText($f)>>Inc.ps1
echo if ($raw.Contains($inc)) { 'include already present: ' + $inc; exit }>>Inc.ps1
echo $nl = "`n"; if ($raw -match "`r`n") { $nl = "`r`n" }>>Inc.ps1
echo $m = [regex]::Match($raw, '(?m)^^#ifndef PUBLIC_BUILD')>>Inc.ps1
echo if ($m.Success) { $raw = $raw.Insert($m.Index, $inc + $nl) } else { $raw = $raw + $inc + $nl }>>Inc.ps1
echo [System.IO.File]::WriteAllText($f, $raw)>>Inc.ps1
echo 'added ' + $inc>>Inc.ps1
Powershell.exe -executionpolicy ByPass -File Inc.ps1
if exist Inc.ps1 del Inc.ps1
exit /b
