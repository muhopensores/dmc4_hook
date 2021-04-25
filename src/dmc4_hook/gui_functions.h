#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_internal.h"

#define GUI_VERSION "DMC4Hook 1.3.4" // Added Rose Removes Pins // Added No Helm Breaker Knockaway

//gui function definitions

//visual theme of the gui, can be changed to change the look
void DarkTheme();

//draw function with parameters for the gui overlay
void DrawWindow();

//imgui::being seperated into function (required to make gui overlay work, see imgui example and documentation
void BeginDrawing();

//function that draws the fps onto the gui
void FPSDrawing();

//function that draws a "misc." text onto the gui, made because misc is used frequently, so to avoid typing extra stuff every time
void Misc();

//function for putting credits specific things in the gui
void CreditsDrawing();

//helpmarker function, to avoid typing it every time in the gui section
void HelpMarker(const char* desc);



