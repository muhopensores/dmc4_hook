PROJECT(dmc4_hook)

#add mods here, modName.cpp and modName.hpp
SET (MODS
  mods/modTwCmdSuperhot.hpp
  mods/modTwCmdSuperhot.cpp
  mods/modSample.cpp
  mods/modSample.hpp
  mods/modBorderless.cpp
  mods/modBorderless.hpp
  mods/modAreaJump.cpp
  mods/modAreaJump.hpp
  mods/modMoveIDs.cpp
  mods/modMoveIDs.hpp
  mods/modWorkRate.cpp
  mods/modWorkRate.hpp
  mods/modSelectiveCancels.hpp
  mods/modSelectiveCancels.cpp
  mods/modLimitAdjust.cpp
  mods/modLimitAdjust.hpp
  mods/modBackgroundRendering.cpp
  mods/modBackgroundRendering.hpp
  mods/modNoHBknockback.cpp
  mods/modNoHBknockback.hpp
  mods/modEasyJc.cpp
  mods/modEasyJc.hpp
  mods/modInfDreadnought.cpp
  mods/modInfDreadnought.hpp
  mods/modNoClip.cpp
  mods/modNoClip.hpp
  mods/modSkipPandora.cpp
  mods/modSkipPandora.hpp
  mods/modRestoreMaxHp.cpp
  mods/modRestoreMaxHp.hpp
  mods/modInputStates.cpp
  mods/modInputStates.hpp
  mods/modFpsLimit.cpp
  mods/modFpsLimit.hpp
  mods/modMoveIDsNero.cpp
  mods/modMoveIDsNero.hpp
  mods/modInfSkyStars.cpp
  mods/modInfSkyStars.hpp
  mods/modForceLucifer.cpp
  mods/modForceLucifer.hpp
  mods/modLdkWithDmd.cpp
  mods/modLdkWithDmd.hpp
  mods/modDmdBloodyPalace.cpp
  mods/modDmdBloodyPalace.hpp
  mods/modDmdLevelAi.cpp
  mods/modDmdLevelAi.hpp
  mods/modManualTwosomeTime.cpp
  mods/modManualTwosomeTime.hpp
  mods/modHeightRestrictionDante.cpp
  mods/modHeightRestrictionDante.hpp
  mods/modInfFaustCloak.cpp
  mods/modInfFaustCloak.hpp
  mods/modPassiveEnemies.cpp
  mods/modPassiveEnemies.hpp
  mods/modHeightRestrictionNero.cpp
  mods/modHeightRestrictionNero.hpp
  mods/modInfiniteTime.cpp
  mods/modInfiniteTime.hpp
  mods/modInfAllHealth.cpp
  mods/modInfAllHealth.hpp
  mods/modCameraSettings.cpp
  mods/modCameraSettings.hpp
  mods/modFreezeEnemies.cpp
  mods/modFreezeEnemies.hpp
  mods/modFreeJc.cpp
  mods/modFreeJc.hpp
  mods/modNoDeath.cpp
  mods/modNoDeath.hpp
  mods/modEnemySpawn.cpp
  mods/modEnemySpawn.hpp
  mods/modKnockbackEdits.cpp
  mods/modKnockbackEdits.hpp
  mods/modNoDtCooldown.cpp
  mods/modNoDtCooldown.hpp
  mods/modDtEnemiesDontStun.cpp
  mods/modDtEnemiesDontStun.hpp
  mods/modEnemySlotting.cpp
  mods/modEnemySlotting.hpp
  mods/modPlayerTracker.cpp
  mods/modPlayerTracker.hpp
  mods/modTrackingFullHouse.cpp
  mods/modTrackingFullHouse.hpp
  mods/modInfTrickRange.cpp
  mods/modInfTrickRange.hpp
  mods/modTimerMem.cpp
  mods/modTimerMem.hpp
  mods/modTrickDown.cpp
  mods/modTrickDown.hpp
  mods/modDamageMultiplier.cpp
  mods/modDamageMultiplier.hpp
  mods/modHpInOrbsDisplay.cpp
  mods/modHpInOrbsDisplay.hpp
  mods/modInfDT.cpp
  mods/modInfDT.hpp
  mods/modInfPlayerHealth.cpp
  mods/modInfPlayerHealth.hpp
  mods/modBerialDaze.cpp
  mods/modBerialDaze.hpp
  mods/modRoseRemovesPins.cpp
  mods/modRoseRemovesPins.hpp
  mods/modStunAnything.cpp
  mods/modStunAnything.hpp
  mods/modDisableCameraEvents.cpp
  mods/modDisableCameraEvents.hpp
  mods/modHideHud.cpp
  mods/modHideHud.hpp
  mods/modEnemyDT.cpp
  mods/modEnemyDT.hpp
  mods/modBpPortal.cpp
  mods/modBpPortal.hpp
  mods/modFastPandora.cpp
  mods/modFastPandora.hpp
  mods/modInfAirHikes.cpp
  mods/modInfAirHikes.hpp
  mods/modInfRevive.cpp
  mods/modInfRevive.hpp
  mods/modCharacterSwap.cpp
  mods/modCharacterSwap.hpp
  mods/modDisableDarkslayer.cpp
  mods/modDisableDarkslayer.hpp
  mods/modRemoveLaunchArmour.cpp
  mods/modRemoveLaunchArmour.hpp
  mods/modCutsceneSkip.cpp
  mods/modCutsceneSkip.hpp
  mods/modFastSprint.cpp
  mods/modFastSprint.hpp
  mods/modInfTableHopper.cpp
  mods/modInfTableHopper.hpp
  mods/modEnemyAttackOffScreen.cpp
  mods/modEnemyAttackOffScreen.hpp
  mods/modSlowWalk.cpp
  mods/modSlowWalk.hpp
  mods/modEnemyReplace.cpp
  mods/modEnemyReplace.hpp
  mods/modDifficultySelect.cpp
  mods/modDifficultySelect.hpp
  mods/modBpJumpHook.cpp
  mods/modBpJumpHook.hpp
  mods/modFastStart.cpp
  mods/modFastStart.hpp
  mods/modNoAutomaticCharacters.cpp
  mods/modNoAutomaticCharacters.hpp
  mods/modDisableChimeraBlades.cpp
  mods/modDisableChimeraBlades.hpp
  mods/modTwitch.hpp
  mods/modTwitch.cpp
  mods/modQuicksilver.hpp
  mods/modQuicksilver.cpp
  mods/modTwCmdHolyWater.hpp
  mods/modTwCmdHolyWater.cpp
  mods/modBigHeadMode.hpp
  mods/modBigHeadMode.cpp
  mods/modTwCmdPlayerTransforms.hpp
  mods/modTwCmdPlayerTransforms.cpp
  mods/modInfCalibur.hpp
  mods/modInfCalibur.cpp
  mods/modRgMultiplier.hpp
  mods/modRgMultiplier.cpp
  mods/modDeltaTime.hpp
  mods/modDeltaTime.cpp
  mods/modActiveBlock.hpp
  mods/modActiveBlock.cpp
  mods/modNeroFullHouse.hpp
  mods/modNeroFullHouse.cpp
)

SET (HEADERS
  aobs.h
  gui_functions.h
  hooks.h
  imgui_dmc4.h
  main.h
  naked.h
  patches.h
  mods.h
  mod.hpp
  Mods.hpp
)

SET (IMGUI
  imgui/imconfig.h
  imgui/imgui.cpp
  imgui/imgui.h
  imgui/imgui_draw.cpp
  imgui/imgui_impl_dx9.cpp
  imgui/imgui_impl_dx9.h
  imgui/imgui_impl_win32.cpp
  imgui/imgui_impl_win32.h
  imgui/imgui_internal.h
  imgui/imgui_widgets.cpp
  imgui/imstb_rectpack.h
  imgui/imstb_textedit.h
  imgui/imstb_truetype.h
)

#SET (MODS 
#  mods/modSelectiveCancels.cpp
#  mods/modMoveIDs.cpp
#  mods/modLimitAdjust.cpp
#  mods/modShared.cpp
#  mods/modBackgroundRendering.cpp
#)
# aux_source_directory(<dir> <variable>)
#aux_source_directory(mods MODS)

SET (UTILS
  utils/Thread.hpp
  utils/Thread.cpp
  utils/Timer.hpp
  utils/Timer.cpp
  utils/MessageDisplay.hpp
  utils/MessageDisplay.cpp
  utils/Hash.hpp
  utils/Config.hpp
  utils/Config.cpp
  utils/String.cpp
  utils/String.hpp
  utils/crash_handler.h
  utils/crash_handler.cpp
  utils/RingBuffer.hpp
)

SET (TWITCH
  twitch/twitch.hpp # note to future self pick one fucking capitalisation scheme
  twitch/twitch.cpp
  twitch/libirc_errors.h
  twitch/libirc_events.h
  twitch/libirc_options.h
  twitch/libirc_rfcnumeric.h
  twitch/libircclient.h
)

SET (SDK
  sdk/ReClass_Internal.hpp
  sdk/Devil4.cpp
  sdk/Devil4.hpp
)

SET (SOURCES
  hooks.cpp
  main.cpp
  gui_functions.cpp
  Mods.cpp
)

source_group("imgui" FILES ${IMGUI})
source_group("mods"  FILES ${MODS})
source_group("utils" FILES ${UTILS})
source_group("sdk"   FILES ${SDK})
source_group("twitch" FILES ${TWITCH})

if(GIT_COMMIT_HASH)
  message("GIT_COMMIT_HASH: ${GIT_COMMIT_HASH}")
  message("GIT_COMMITTER_DATE: ${GIT_COMMITTER_DATE}")
  add_compile_definitions(GIT_COMMIT_HASH=${GIT_COMMIT_HASH})
  add_compile_definitions(GIT_COMMITTER_DATE=${GIT_COMMITTER_DATE})
endif()
add_compile_definitions(_WINSOCKAPI_)
add_compile_definitions(TRACY_ENABLE)
ADD_LIBRARY(${PROJECT_NAME} SHARED
            ${HEADERS}
            ${IMGUI}
      	    ${MODS}
            ${UTILS}
	    ${SDK}
            ${SOURCES}
            ${TWITCH}
)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME dinput8)
#TARGET_LINK_LIBRARIES(${PROJECT_NAME} hacklib minhook libircclient.lib tracy)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} hacklib minhook)
