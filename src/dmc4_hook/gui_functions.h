#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_internal.h"

#define GUI_VERSION "DMC4Hook 1.4.2"

//gui function definitions

//visual theme of the gui, can be changed to change the look
void DarkTheme();

//function for putting credits specific things in the gui
void CreditsDrawing(const char* page);

//helpmarker function, to avoid typing it every time in the gui section
void HelpMarker(const char* desc);

// enable docking on imgui windows
void DockingSetup(float gui_split);

// make space to the right of the trainer to see into game
void RightWindow(void);

// make space below the trainer to see into game
void LowerWindow(void);
