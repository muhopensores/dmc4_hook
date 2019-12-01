PROJECT(dmc4_hook)

SET (HEADERS
  aobs.h
  gui_functions.h
  hooks.h
  imgui_dmc4.h
  main.h
  naked.h
  patches.h
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

SET (INIH
  inih/ini.c
  inih/ini.h
  inih/cpp/INIReader.cpp
  inih/cpp/INIReader.h
)

SET (SOURCES
  hooks.cpp
  main.cpp
)

source_group("imgui", FILES ${IMGUI})
source_group("inih",  FILES ${INIH})

ADD_LIBRARY(${PROJECT_NAME} SHARED
            ${HEADERS}
            ${IMGUI}
            ${INIH}
            ${SOURCES}
)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_NAME})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} hacklib)
