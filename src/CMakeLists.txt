#add mods here, modName.cpp and modName.hpp
set(MODS_SRC
  mods/RoomRespawn.hpp
  mods/RoomRespawn.cpp
  mods/LoadOrder.hpp
  mods/LoadOrder.cpp
  mods/ShaderEditor.hpp
  mods/ShaderEditor.cpp
  mods/TwCmdSuperhot.hpp
  mods/TwCmdSuperhot.cpp
  mods/Sample.cpp
  mods/Sample.hpp
  mods/Borderless.cpp
  mods/Borderless.hpp
  mods/AreaJump.cpp
  mods/AreaJump.hpp
  mods/MoveIDs.cpp
  mods/MoveIDs.hpp
  mods/WorkRate.cpp
  mods/WorkRate.hpp
  mods/SelectiveCancels.hpp
  mods/SelectiveCancels.cpp
  mods/LimitAdjust.cpp
  mods/LimitAdjust.hpp
  mods/BackgroundRendering.cpp
  mods/BackgroundRendering.hpp
  mods/NoHBknockback.cpp
  mods/NoHBknockback.hpp
  mods/EasyJc.cpp
  mods/EasyJc.hpp
  mods/InfDreadnaught.cpp
  mods/InfDreadnaught.hpp
  mods/NoClip.cpp
  mods/NoClip.hpp
  mods/SkipWeapons.cpp
  mods/SkipWeapons.hpp
  mods/RestoreMaxHp.cpp
  mods/RestoreMaxHp.hpp
  mods/InputStates.cpp
  mods/InputStates.hpp
  mods/FpsLimit.cpp
  mods/FpsLimit.hpp
  mods/MoveIDsNero.cpp
  mods/MoveIDsNero.hpp
  mods/InfSkyStars.cpp
  mods/InfSkyStars.hpp
  mods/ForceLucifer.cpp
  mods/ForceLucifer.hpp
  mods/LdkWithDmd.cpp
  mods/LdkWithDmd.hpp
  mods/DmdBloodyPalace.cpp
  mods/DmdBloodyPalace.hpp
  mods/DmdLevelAi.cpp
  mods/DmdLevelAi.hpp
  mods/ManualTwosomeTime.cpp
  mods/ManualTwosomeTime.hpp
  mods/HeightRestrictionDante.cpp
  mods/HeightRestrictionDante.hpp
  mods/InfFaustCloak.cpp
  mods/InfFaustCloak.hpp
  mods/HeightRestrictionNero.cpp
  mods/HeightRestrictionNero.hpp
  mods/InfiniteTime.cpp
  mods/InfiniteTime.hpp
  mods/InfAllHealth.cpp
  mods/InfAllHealth.hpp
  mods/CameraSettings.cpp
  mods/CameraSettings.hpp
  mods/FreezeEnemies.cpp
  mods/FreezeEnemies.hpp
  mods/FreeJc.cpp
  mods/FreeJc.hpp
  mods/OneHitKill.cpp
  mods/OneHitKill.hpp
  mods/EnemySpawn.cpp
  mods/EnemySpawn.hpp
  mods/KnockbackEdits.cpp
  mods/KnockbackEdits.hpp
  mods/NoDtCooldown.cpp
  mods/NoDtCooldown.hpp
  mods/DtEnemiesDontStun.cpp
  mods/DtEnemiesDontStun.hpp
  mods/EnemySlotting.cpp
  mods/EnemySlotting.hpp
  mods/PlayerTracker.cpp
  mods/PlayerTracker.hpp
  mods/TrackingFullHouse.cpp
  mods/TrackingFullHouse.hpp
  mods/InfTrickRange.cpp
  mods/InfTrickRange.hpp
  mods/TimerMem.cpp
  mods/TimerMem.hpp
  mods/TrickDown.cpp
  mods/TrickDown.hpp
  mods/DamageMultiplier.cpp
  mods/DamageMultiplier.hpp
  mods/HpInOrbsDisplay.cpp
  mods/HpInOrbsDisplay.hpp
  mods/InfDT.cpp
  mods/InfDT.hpp
  mods/InfPlayerHealth.cpp
  mods/InfPlayerHealth.hpp
  mods/BerialDaze.cpp
  mods/BerialDaze.hpp
  mods/RoseRemovesPins.cpp
  mods/RoseRemovesPins.hpp
  mods/StunAnything.cpp
  mods/StunAnything.hpp
  mods/DisableCameraEvents.cpp
  mods/DisableCameraEvents.hpp
  mods/HideHud.cpp
  mods/HideHud.hpp
  mods/EnemyDT.cpp
  mods/EnemyDT.hpp
  mods/BpPortal.cpp
  mods/BpPortal.hpp
  mods/FastPandora.cpp
  mods/FastPandora.hpp
  mods/InfAirHikes.cpp
  mods/InfAirHikes.hpp
  mods/InfRevive.cpp
  mods/InfRevive.hpp
  mods/CharacterSwap.cpp
  mods/CharacterSwap.hpp
  mods/DisableDarkslayer.cpp
  mods/DisableDarkslayer.hpp
  mods/RemoveLaunchArmour.cpp
  mods/RemoveLaunchArmour.hpp
  mods/CutsceneSkip.cpp
  mods/CutsceneSkip.hpp
  mods/FastSprint.cpp
  mods/FastSprint.hpp
  mods/InfTableHopper.cpp
  mods/InfTableHopper.hpp
  mods/EnemyAttackOffScreen.cpp
  mods/EnemyAttackOffScreen.hpp
  mods/SlowWalk.cpp
  mods/SlowWalk.hpp
  mods/EnemyReplace.cpp
  mods/EnemyReplace.hpp
  mods/DifficultySelect.cpp
  mods/DifficultySelect.hpp
  mods/BpJumpHook.cpp
  mods/BpJumpHook.hpp
  mods/FastStart.cpp
  mods/FastStart.hpp
  mods/NoAutomaticCharacters.cpp
  mods/NoAutomaticCharacters.hpp
  mods/DisableChimeraBlades.cpp
  mods/DisableChimeraBlades.hpp
  mods/Twitch.hpp
  mods/Twitch.cpp
  mods/Quicksilver.hpp
  mods/Quicksilver.cpp
  mods/TwCmdHolyWater.hpp
  mods/TwCmdHolyWater.cpp
  mods/BigHeadMode.hpp
  mods/BigHeadMode.cpp
  mods/TwCmdPlayerTransforms.hpp
  mods/TwCmdPlayerTransforms.cpp
  mods/InfCalibur.hpp
  mods/InfCalibur.cpp
  mods/RgMultiplier.hpp
  mods/RgMultiplier.cpp
  mods/DeltaTime.hpp
  mods/DeltaTime.cpp
  mods/ActiveBlock.hpp
  mods/ActiveBlock.cpp
  mods/NeroFullHouse.hpp
  mods/NeroFullHouse.cpp
  mods/FrostsDontJump.hpp
  mods/FrostsDontJump.cpp
  mods/NeroSnatchLength.hpp
  mods/NeroSnatchLength.cpp
  mods/ChargeChecker.hpp
  mods/ChargeChecker.cpp
  mods/EnemyStats.hpp
  mods/EnemyStats.cpp
  mods/FasterFastDrive.hpp
  mods/FasterFastDrive.cpp
  mods/RoseOptions.hpp
  mods/RoseOptions.cpp
  mods/InstantTrick.hpp
  mods/InstantTrick.cpp
  mods/EnemyReplaceAgain.hpp
  mods/EnemyReplaceAgain.cpp
  mods/MessageDisplay.hpp
  mods/MessageDisplay.cpp
)

set(HOOKS_SRC
    D3D9Hook.hpp
    D3D9Hook.cpp
    WindowsMessageHook.hpp
    WindowsMessageHook.cpp
)

set(SDK_SRC
    sdk/ReClass.hpp
    sdk/ReClass_Internal.hpp
    sdk/Math.hpp
    sdk/Devil4.cpp
    sdk/Devil4.hpp
)

set(IMGUI_SRC
    fw-imgui/imgui_impl_dx9.h
    fw-imgui/imgui_impl_dx9.cpp
    fw-imgui/imgui_impl_win32.h
    fw-imgui/imgui_impl_win32.cpp
)

set(UTILITY_SRC
  utility/ExceptionHandler.hpp
  utility/ExceptionHandler.cpp
  utility/ChronoAlias.hpp
  utility/Thread.hpp
  utility/Thread.cpp
  utility/Timer.hpp
  utility/Timer.cpp
  utility/MessageDisplay.hpp
  utility/MessageDisplay.cpp
  utility/Hash.hpp
  utility/Config.hpp
  utility/Config.cpp
  utility/String.cpp
  utility/String.hpp
  utility/RingBuffer.hpp
  utility/Address.hpp
  utility/Address.cpp
  utility/FunctionHook.hpp
  utility/FunctionHook.cpp
  utility/Memory.hpp
  utility/Memory.cpp
  utility/Module.hpp
  utility/Module.cpp
  utility/Patch.hpp
  utility/Patch.cpp
  utility/Pattern.hpp
  utility/Pattern.cpp
  utility/Scan.hpp
  utility/Scan.cpp
  utility/Input.hpp
  utility/Input.cpp
)

set(TWITCH_SRC
  twitch/Twitch.hpp
  twitch/Twitch.cpp
  twitch/libirc_errors.h
  twitch/libirc_events.h
  twitch/libirc_options.h
  twitch/libirc_rfcnumeric.h
  twitch/libircclient.h
)

set(FRAMEWORK_SRC
  Mod.hpp
  Mods.hpp
  Mods.cpp
  ModFramework.hpp
  ModFramework.cpp
  Config.hpp
  Main.cpp
  GuiFunctions.hpp
  GuiFunctions.cpp
  Mods.cpp
)

source_group("imgui"   FILES ${IMGUI_SRC})
source_group("Hooks"   FILES ${HOOKS_SRC})
source_group("Mods"    FILES ${MODS_SRC})
source_group("Utility" FILES ${UTILITY_SRC})
source_group("SDK"     FILES ${SDK_SRC})
source_group("Twitch"  FILES ${TWITCH_SRC})

#if(GIT_COMMIT_HASH)
#  message("GIT_COMMIT_HASH: ${GIT_COMMIT_HASH}")
#  message("GIT_COMMITTER_DATE: ${GIT_COMMITTER_DATE}")
#  add_compile_definitions(GIT_COMMIT_HASH=${GIT_COMMIT_HASH})
#  add_compile_definitions(GIT_COMMITTER_DATE=${GIT_COMMITTER_DATE})
#endif()

add_library(${CMAKE_PROJECT_NAME} SHARED
            Main.cpp
            LicenseStrings.hpp
            ${IMGUI_SRC}
            ${MODS_SRC}
            ${HOOKS_SRC}
            ${UTILITY_SRC}
            ${SDK_SRC}
            ${TWITCH_SRC}
            ${FRAMEWORK_SRC}
)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME dinput8)
target_link_libraries(${PROJECT_NAME} imgui minhook glm_static shlwapi D3dx9 d3d9)
target_compile_features(${CMAKE_PROJECT_NAME} PUBLIC cxx_std_17)

include_directories(${CMAKE_PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME dinput8)
target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC DINPUT_HOOK)
if(DEVELOPER_MODE)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC DEVELOPER)
    target_compile_options(${CMAKE_PROJECT_NAME} PUBLIC /EHa)
endif()
