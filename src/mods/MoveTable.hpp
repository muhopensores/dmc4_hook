#pragma once

#include "../mod.hpp"

extern uintptr_t HookDanteKADTbl;
extern uintptr_t HookNeroKADTbl;

class MoveTable: public Mod {
public:
    // static bool mod_enabled;
    static uintptr_t jmp_ret0;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static uintptr_t jmp_ret3;

    MoveTable() = default;

    // void toggle(bool enable);

    void display_attack_entry(kAtckDefTbl* TblEntry);

    std::string get_mod_name() override { return "MoveTable"; };
    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;
private:
    std::unique_ptr<FunctionHook> hook0, hook1, hook2, hook3;
    // std::unique_ptr<Patch> patch1;

    static const int COLUMN_SEARCH_BUFFER_SIZE = 32;
    char columnSearchBuffers[18][COLUMN_SEARCH_BUFFER_SIZE] = {0};
    bool columnSearchActive[18] = {false};
    bool columnSearchIsExact[18] = {false};
    bool enableExactMatch = true;
    
    std::string formatHex(uint32_t value) {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%08x", value);
        return std::string(buffer);
    }
};

static const char* buttonMappingNames[] = {
    "0 - None / Lockon + Melee",
    "1 - Lockon + Gun",
    "2 - Lockon + Style",
    "3 - Lockon + Forward + Melee",
    "4 - Lockon + Back + Melee",
    "5 - Lockon + Forward + Gun",
    "6 - Lockon + Back + Gun", 
    "7 - Lockon + Forward + Style",
    "8 - Lockon + Back + Style",
    "9 - Lockon + Forward + Melee2",
    "10 - Lockon + Back + Melee2",
    "11 - Lockon + Forward + Gun2",
    "12 - Lockon + Back + Gun2",
    "13 - Lockon + Forward + Style2",
    "14 - Lockon + Back + Style2",
    "15 - Lockon + Forward + Melee Stick",
    "16 - Lockon + Back + Melee Stick",
    "17 - Lockon + Forward + Gun Stick",
    "18 - Lockon + Back + Gun Stick",
    "19 - Lockon + Forward + Style Stick", 
    "20 - Lockon + Back + Style Stick",
    "21 - Lockon + Any Direction + Melee",
    "22 - Lockon + Any Direction + Gun",
    "23 - Lockon + Any Direction + Style",
    "24 - Any Direction + Melee", 
    "25 - Any Direction + Gun",
    "26 - Any Direction + Style",
    "27 -",
    "28 -",
    "29 -",
    "30 -",
    "31 -",
    "32 - Gun Release",
    "33 - Gun Charged Release",
    "34 - Lockon + Gun Charge",
    "35 - Gun Charged",
    "36 - Gun Charge3",
    "37 - Gun Charge4",
    "38 - Gun Charge3",
    "39 -",
    "40 -",
    "41 -",
    "42 -",
    "43 -",
    "44 -",
    "45 -",
    "46 -",
    "47 -",
    "48 -",
    "49 -",
    "50 -",
    "51 -",
    "52 -",
    "53 -",
    "54 -",
    "55 -", 
    "56 -",
    "57 -",
    "58 -",
    "59 -",
    "60 -",
    "61 -",
    "62 -",
    "63 -",
    "64 -",
    "65 -",
    "66 -",
    "67 -",
    "68 -",
    "69 -",
    "70 -",
    "71 -",
    "72 -",
    "73 -",
    "74 -",
    "75 -",
    "76 -",
    "77 -",
    "78 -",
    "79 -",
    "80 -",
    "81 - Melee + Style",
    "82 - Lockon + Backforward + Melee",
    "83 -",
    "84 - Gun"
};

static const char* attackNames[] = {
    "0 -",
};

static const char* weaponNames[] = {
    "FF - None",
    "0 - Red Queen",      // ID_RED_QUEEN = 0
    "1 - Blue Rose",      // ID_BLUE_ROSE = 1
    "2 - Yamato",         // ID_YAMATO = 2
    "3 - Right Hand",     // ID_RIGHT_HAND = 3
    "4 - Rebellion",      // ID_REVELLION = 4
    "5 - Gilgamesh",      // ID_GILGAMESH = 5
    "6 - Lucifer",        // ID_BASILISK = 6
    "7 - Coyote Ace",     // ID_COYOTE_ACE = 7
    "8 - Pandora",        // ID_PANDORA = 8
    "9 - Ebony & Ivory"   // ID_EBONY_IVORY = 9
    "10 -",
    "11 -",
    "12 -",
    "13 -",
    "14 -",
    "15 -",
    "16 -",
    "17 -",
    "18 -",
    "19 -",
    "20 -",
};

static const char* styleNames[] = {
    "FF - None",
    "0 - Swordmaster",
    "1 - Gunslinger",
    "2 - Trickster",
    "3 - Royal Guard",
    "4 - Darkslayer",
};

static const char* aerialNames[] = {
    "0 - None",
    "1 - Ground",
    "2 - Air",
    "3 - Ground or air",
};

static const char* restrictNames[] = {
    "0 - None?",
    "1 - None?",
    "2 - Disabled?",
    "3 -",
    "4 - Height Restricted",
};

static const char* dtNames[] = {
    "0 - DT or no DT",
    "1 - DT or no DT 2",
    "2 - Disabled?",
    "3 - DT Only",
    "4 -",
    "5 -",
    "6 -",
    "7 -",
    "8 -",
    "9 - DT or Charge",
    "10 -",
    "11 - DT or Charge 2",
    "12 -",
    "13 -",
    "14 -",
    "15 -",
    "16 -",
    "17 -",
    "18 -",
    "19 -",
    "20 -",
    "21 -",
    "22 -",
    "23 -",
    "24 -",
    "25 -",
    "26 -",
    "27 -",
    "28 -",
    "29 -",
    "30 -",
};
