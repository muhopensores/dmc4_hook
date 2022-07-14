# DMC4Hook #

## Info
A trainer for the original DX9 version of Devil May Cry 4 made based on [reframework](https://github.com/praydog/REFramework/), using [minhook](https://github.com/TsudaKageyu/minhook), [glm](https://github.com/g-truc/glm) and [Dear ImGui](https://github.com/ocornut/imgui).

This dll will give you access to training, gameplay, and quality of life options without ever needing to open anything alongside your exe.

Builds are intended for use on the Steam DX9 version of the game and will crash on other versions. Might want to unpack steam drm from `DevilMayCry4_DX9.exe` with [steamless](https://github.com/atom0s/Steamless)

## Download
Releases can be found [here.](https://github.com/muhopensores/dmc4_hook/releases)

## Instructions
For help regarding usage of the trainer, check our [wiki.](https://github.com/muhopensores/dmc4_hook/wiki)

## How to build
Run build_debug.bat/build_release.bat edit bat files according to your visual studio version if needed.

## Linux proton support
Should work fine with `WINEDLLOVERRIDES="dinput8=n,b" %command%` in Steams launch options. Game may freeze up on exit if you quit through menu, might want to `pkill` it or something