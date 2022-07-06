#pragma once

#include "Mods.hpp"

namespace gui {
    //gui function definitions

    //visual theme of the gui, can be changed to change the look
    void dark_theme();

    //draw function with parameters for the gui overlay
    void set_window_props();

    //imgui::being seperated into function (required to make gui overlay work, see imgui example and documentation
    void begin_drawing();

    //function that draws the fps onto the gui
    void fps_drawing();

    //function for putting credits specific things in the gui
    void credits_drawing();

    //helpmarker function, to avoid typing it every time in the gui section
    void help_marker(const char* desc);

    void im_gui_main_window_proc(Mods* pmods);
}