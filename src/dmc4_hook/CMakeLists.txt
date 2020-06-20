PROJECT(dmc4_hook)

#add mods here, modName.cpp and modName.hpp
SET (MODS
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
  mods/modFreeCam.cpp
  mods/modFreeCam.hpp
  mods/modForceLucifer.cpp
  mods/modForceLucifer.hpp
  mods/modDisableLastEnemyZoom.cpp
  mods/modDisableLastEnemyZoom.hpp
  mods/modLdkWithDmd.cpp
  mods/modLdkWithDmd.hpp
  mods/modDmdBloodyPalace.cpp
  mods/modDmdBloodyPalace.hpp
  mods/modDmdLevelAi.cpp
  mods/modDmdLevelAi.hpp
  mods/modOneHitKill.cpp
  mods/modOneHitKill.hpp
  mods/modHideTimer.cpp
  mods/modHideTimer.hpp
  mods/modManualTwosomeTime.cpp
  mods/modManualTwosomeTime.hpp
  mods/modHeightRestrictionDante.cpp
  mods/modHeightRestrictionDante.hpp
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
  utils/Hash.hpp
  utils/Config.hpp
  utils/Config.cpp
  utils/String.cpp
  utils/String.hpp
  utils/crash_handler.h
  utils/crash_handler.cpp
)

SET (SDK
  sdk/ReClass_Internal.hpp
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

if(GIT_COMMIT_HASH)
  message("GIT_COMMIT_HASH: ${GIT_COMMIT_HASH}")
  message("GIT_COMMITTER_DATE: ${GIT_COMMITTER_DATE}")
  add_compile_definitions(GIT_COMMIT_HASH=${GIT_COMMIT_HASH})
  add_compile_definitions(GIT_COMMITTER_DATE=${GIT_COMMITTER_DATE})
endif()

ADD_LIBRARY(${PROJECT_NAME} SHARED
            ${HEADERS}
            ${IMGUI}
      	    ${MODS}
            ${UTILS}
	        ${SDK}
            ${SOURCES}
)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME dinput8)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} hacklib minhook)
