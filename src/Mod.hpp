#pragma once

#define naked static __declspec(naked)
#define _(string) utility::text_lookup(string)
#define __(str) str

// Some mod files contain multiple mods that need to be displayed in different places
// Using int display we can:
// 1: Display character specific mods on character specific pages (e.g. Dante and Nero both have no height restriction)
// 2: Display mods differently in different places (e.g. camera settings appears differently in the gui and in photo mode)
// 3: Display different characters' mods in a different colour in search results
enum DisplayID {
    DISPLAY_SYSTEM_A = 0,
    DISPLAY_SYSTEM_B = 1,
    DISPLAY_SYSTEM_C = 2,
    DISPLAY_NERO_A   = 3,
    DISPLAY_NERO_B   = 4,
    DISPLAY_NERO_C   = 5,
    DISPLAY_DANTE_A  = 6,
    DISPLAY_DANTE_B  = 7,
    DISPLAY_DANTE_C  = 8,
    DISPLAY_MAX      = 9,
};
