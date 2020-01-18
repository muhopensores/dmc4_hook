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
  config.h
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

SET (MODS 
  mods/modSelectiveCancels.cpp
  mods/modMoveIDs.cpp
  mods/modLimitAdjust.cpp
  mods/modShared.cpp
  mods/modBackgroundRendering.cpp
)
SET (UTILS
  utils/ini.h
)

SET (SOURCES
  hooks.cpp
  main.cpp
  gui_functions.cpp
  config.cpp
)

source_group("imgui", FILES ${IMGUI})
source_group("mods",  FILES ${MODS})
source_group("utils", FILES ${UTILS})

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
