@echo off
set /p filename="Enter filename without extension like ModName:"
set "name=%filename%"
for /f %%i in ('powershell -NoProfile -Command "\"%filename:~0,1%\".ToUpper() + \"%filename:~1%\""') do set "name=%%i"

call:DoReplace "ModSample" "%name%" src\mods\ModSample.cpp src\mods\%name%.cpp
call:DoReplace "ModSample" "%name%" src\mods\ModSample.hpp src\mods\%name%.hpp
echo Copied cmakelist addition
(
echo     mods/%name%.hpp
echo     mods/%name%.cpp
)| clip
start "" notepad src\CMakeLists.txt

echo Adding mods\%name%.hpp to src\ModsHeaders.hpp
call:AddInclude src\ModsHeaders.hpp %name%
echo Dont forget to add ADD_MOD(%name%); into Mods.cpp Mods::Mods() - order matters
exit /b

:DoReplace
echo ^(Get-Content "%3"^) ^| ForEach-Object { $_ -replace %1, %2 } ^| Where-Object { $_ -notmatch '^#(if 0^|endif)$' } ^| Set-Content %4>Rep.ps1
Powershell.exe -executionpolicy ByPass -File Rep.ps1
if exist Rep.ps1 del Rep.ps1
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
