PROJECT(dmc4_hook)

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
SET (MODS
  mods/modSelectiveCancels.hpp
  mods/modSelectiveCancels.cpp
  mods/modMoveIDs.cpp
  mods/modLimitAdjust.cpp
  mods/modShared.cpp
  mods/modBackgroundRendering.cpp
  mods/modNoHBknockback.cpp
)

SET (UTILS
  utils/ini.h
  utils/Config.hpp
  utils/Config.cpp
  utils/String.cpp
  utils/String.hpp
  utils/crash_handler.h
  utils/crash_handler.cpp
)

SET (SOURCES
  hooks.cpp
  main.cpp
  gui_functions.cpp
)

source_group("imgui", FILES ${IMGUI})
source_group("mods",  FILES ${MODS})
source_group("utils", FILES ${UTILS})

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
            ${SOURCES}
)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME dinput8)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} hacklib minhook)
