#pragma once

#include "../mod.hpp"
#include "misc/kAtckDefTbl.cpp"

extern uintptr_t HookDanteKADTbl;
extern uintptr_t HookNeroKADTbl;

class MoveTable: public Mod {
public:
    // static bool mod_enabled;
    static uintptr_t jmp_ret0;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static uintptr_t jmp_ret3;

    static int extra_nero_moves;
    static int extra_dante_moves;

    static int AirThrow;

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
    __("0 - None / Lockon + Melee"),
    __("1 - Lockon + Gun"),
    __("2 - Lockon + Style"),
    __("3 - Lockon + Forward + Melee"),
    __("4 - Lockon + Back + Melee"),
    __("5 - Lockon + Forward + Gun"),
    __("6 - Lockon + Back + Gun"), 
    __("7 - Lockon + Forward + Style"),
    __("8 - Lockon + Back + Style"),
    __("9 - Lockon + Forward + Melee2"),
    __("10 - Lockon + Back + Melee2"),
    __("11 - Lockon + Forward + Gun2"),
    __("12 - Lockon + Back + Gun2"),
    __("13 - Lockon + Forward + Style2"),
    __("14 - Lockon + Back + Style2"),
    __("15 - Lockon + Forward + Melee Stick"),
    __("16 - Lockon + Back + Melee Stick"),
    __("17 - Lockon + Forward + Gun Stick"),
    __("18 - Lockon + Back + Gun Stick"),
    __("19 - Lockon + Forward + Style Stick"), 
    __("20 - Lockon + Back + Style Stick"),
    __("21 - Lockon + Any Direction + Melee"),
    __("22 - Lockon + Any Direction + Gun"),
    __("23 - Lockon + Any Direction + Style"),
    __("24 - Any Direction + Melee"), 
    __("25 - Any Direction + Gun"),
    __("26 - Any Direction + Style"),
    __("27 - Melee"),
    __("28 -"),
    __("29 -"),
    __("30 -"),
    __("31 -"),
    __("32 - Gun Release"),
    __("33 - Gun Charged Release"),
    __("34 - Lockon + Gun Charge"),
    __("35 - Gun Charged"),
    __("36 - Gun Charge3"),
    __("37 - Gun Charge4"),
    __("38 - Gun Charge3"),
    __("39 -"),
    __("40 -"),
    __("41 -"),
    __("42 -"),
    __("43 -"),
    __("44 -"),
    __("45 -"),
    __("46 -"),
    __("47 -"),
    __("48 -"),
    __("49 -"),
    __("50 -"),
    __("51 -"),
    __("52 -"),
    __("53 -"),
    __("54 -"),
    __("55 -"), 
    __("56 -"),
    __("57 -"),
    __("58 -"),
    __("59 -"),
    __("60 -"),
    __("61 -"),
    __("62 -"),
    __("63 -"),
    __("64 -"),
    __("65 -"),
    __("66 -"),
    __("67 -"),
    __("68 -"),
    __("69 -"),
    __("70 -"),
    __("71 -"),
    __("72 -"),
    __("73 -"),
    __("74 -"),
    __("75 -"),
    __("76 -"),
    __("77 -"),
    __("78 -"),
    __("79 -"),
    __("80 -"),
    __("81 - Melee + Style"),
    __("82 - Lockon + Backforward + Melee"),
    __("83 -"),
    __("84 - Gun")
};

static const char* attackNames[] = {
    "0 -",
};

static const char* weaponNames[] = {
    __("FF - None"),
    __("0 - Red Queen"),      // ID_RED_QUEEN = 0
    __("1 - Blue Rose"),      // ID_BLUE_ROSE = 1
    __("2 - Yamato"),         // ID_YAMATO = 2
    __("3 - Right Hand"),     // ID_RIGHT_HAND = 3
    __("4 - Rebellion"),      // ID_REVELLION = 4
    __("5 - Gilgamesh"),      // ID_GILGAMESH = 5
    __("6 - Lucifer"),        // ID_BASILISK = 6
    __("7 - Coyote Ace"),     // ID_COYOTE_ACE = 7
    __("8 - Pandora"),        // ID_PANDORA = 8
    __("9 - Ebony & Ivory"),  // ID_EBONY_IVORY = 9
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
