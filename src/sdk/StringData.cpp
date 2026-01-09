#include "StringData.hpp"
#include "utility/MoFile.hpp"

#define _(string) utility::text_lookup(string)
#define __(str) str

#define IM_ARRAYSIZE(_ARR) ((int)(sizeof(_ARR) / sizeof(*(_ARR))))

std::vector<std::pair<uint16_t, const char*>> buttonPairs = {
    {0x0000, __("None")},
    {0x0001, __("Select")},
    {0x0002, __("L3")},
    {0x0004, __("R3")},
    {0x0008, __("Start")},
    {0x0010, __("Dpad Up")},
    {0x0020, __("Dpad Right")},
    {0x0040, __("Dpad Down")},
    {0x0080, __("Dpad Left")},
    {0x0100, __("L1")},
    {0x0200, __("R1")},
    {0x0400, __("L2")},
    {0x0800, __("R2")},
    {0x1000, __("Y")},
    {0x2000, __("B")},
    {0x4000, __("A")},
    {0x8000, __("X")}
};


const char* dante_attack_names[109] = {
    "",                                         // 0x00 /   0
    __("Rebellion Combo A1"),                   // 0x01 /   1  ATCK_ID_RV_COMBO0
    __("Rebellion Combo A2"),                   // 0x02 /   2  ATCK_ID_RV_COMBO1
    __("Rebellion Combo A3"),                   // 0x03 /   3  ATCK_ID_RV_COMBO2
    __("Rebellion Combo B1"),                   // 0x04 /   4  ATCK_ID_RV_DELAY_COMBO0
    __("Rebellion Combo B2"),                   // 0x05 /   5  ATCK_ID_RV_DELAY_COMBO1
    __("Million Stab"),                         // 0x06 /   6  ATCK_ID_RV_HYAKURETSU
    __("Helm Breaker"),                         // 0x07 /   7  ATCK_ID_RV_KABUTO
    __("Stinger Level 1"),                      // 0x08 /   8  ATCK_ID_RV_STINGER_LV1
    __("Stinger Level 2"),                      // 0x09 /   9  ATCK_ID_RV_STINGER_LV2
    __("Stinger DT"),                           // 0x0A /  10  ATCK_ID_RV_DT_STINGER
    __("High Time (Ground)"),                   // 0x0B /  11  ATCK_ID_RV_HITIME
    __("High Time (Air)"),                      // 0x0C /  12  ATCK_ID_RV_HITIMEJUMP
    __("Round Trip"),                           // 0x0D /  13  ATCK_ID_RV_ROUND_TRIP
    __("Round Trip (Call Back Ground)"),        // 0x0E /  14  ATCK_ID_RV_RETURN
    __("Round Trip (Call Back Air)"),           // 0x0F /  15  ATCK_ID_RV_RETURN_AIR
    __("Prop"),                                 // 0x10 /  16  ATCK_ID_SRV_ROLLING
    __("Shredder"),                             // 0x11 /  17  ATCK_ID_SRV_ROLLING_SERIES
    __("Aerial Rave 1"),                        // 0x12 /  18  ATCK_ID_SRV_AIR_COMBO0
    __("Aerial Rave 2"),                        // 0x13 /  19  ATCK_ID_SRV_AIR_COMBO1
    __("Aerial Rave 3"),                        // 0x14 /  20  ATCK_ID_SRV_AIR_COMBO2
    __("Aerial Rave 4"),                        // 0x15 /  21  ATCK_ID_SRV_AIR_COMBO3
    __("Dance Macabre"),                        // 0x16 /  22  ATCK_ID_SRV_DANCE
    __("Drive"),                                // 0x17 /  23  ATCK_ID_SRV_DRIVE
    __("Overdrive"),                            // 0x18 /  24  ATCK_ID_SRV_DRIVE_SERIES
    __("Quick Drive"),                          // 0x19 /  25  ATCK_ID_SRV_COMBO0_DRIVE
    __("Gilgamesh Combo A1"),                   // 0x1A /  26  ATCK_ID_GM_L_STRAIGHT
    __("Gilgamesh Combo A2"),                   // 0x1B /  27  ATCK_ID_GM_R_BODY
    __("Gilgamesh Combo A3"),                   // 0x1C /  28  ATCK_ID_GM_HI_KICK
    __("Gilgamesh Combo A4"),                   // 0x1D /  29  ATCK_ID_GM_HEEL
    __("Gilgamesh Combo B1"),                   // 0x1E /  30  ATCK_ID_GM_HYAKURETSU
    __("Gilgamesh Combo B2"),                   // 0x1F /  31  ATCK_ID_GM_WHIRLWIND
    __("Full House"),                           // 0x20 /  32  ATCK_ID_GM_DESCENT_KICK
    __("Kick 13 (Regular)"),                    // 0x21 /  33  ATCK_ID_GM_KICK_13R
    __("Draw"),                                 // 0x22 /  34  ATCK_ID_GM_SWAYING_BACK
    __("Straight"),                             // 0x23 /  35  ATCK_ID_GM_STRAIGHT
    __("Shock (Ground)"),                       // 0x24 /  36  ATCK_ID_SGM_INFERNO
    __("Shocking (Air)"),                       // 0x25 /  37  ATCK_ID_SGM_AIR_INFERNO
    __("Real Impact"),                          // 0x26 /  38  ATCK_ID_SGM_REAL_IMPACT
    __("Beast Uppercut"),                       // 0x27 /  39  ATCK_ID_SGM_MAGMA_DRIVE
    __("Rising Dragon"),                        // 0x28 /  40  ATCK_ID_SGM_RISING_DRAGON
    __("Divine Dragon"),                        // 0x29 /  41  ATCK_ID_SGM_DIVINE_DRAGON
    __("Lucifer Combo A1"),                     // 0x2A /  42  ATCK_ID_LF_WIDE_RANGE_COMBO0
    __("Lucifer Combo A2"),                     // 0x2B /  43  ATCK_ID_LF_WIDE_RANGE_COMBO1
    __("Lucifer Combo A3"),                     // 0x2C /  44  ATCK_ID_LF_WIDE_RANGE_COMBO2
    __("Lucifer Combo A4"),                     // 0x2D /  45  ATCK_ID_LF_WIDE_RANGE_COMBO3
    __("Lucifer Combo D1"),                     // 0x2E /  46  ATCK_ID_LF_CROSS_THROW
    __("Lucifer Combo C1"),                     // 0x2F /  47  ATCK_ID_LF_BOISTEROUS
    __("Lucifer Combo E1"),                     // 0x30 /  48  ATCK_ID_LF_LAUNCH
    __("Lucifer Combo B1"),                     // 0x31 /  49  ATCK_ID_LF_ROLLING
    __("ATCK_ID_LF_ARRANGEMENT_COMBO"),         // 0x32 /  50  ATCK_ID_LF_ARRANGEMENT_COMBO
    __("Splash"),                               // 0x33 /  51  ATCK_ID_LF_AIR_ARRANGEMENT
    __("Pin Up (Regular)"),                     // 0x34 /  52  ATCK_ID_LF_DIO_KNIFE
    __("Pin Up (DT)"),                          // 0x35 /  53  ATCK_ID_LF_ADD_DIO_KNIFE
    __("Ecstasy (Ground)"),                     // 0x36 /  54  ATCK_ID_LF_DETONATE_ALL
    __("Ecstasy (Air)"),                        // 0x37 /  55  ATCK_ID_LF_AIR_DETONATE_ALL
    __("Climax (Ground)"),                      // 0x38 /  56  ATCK_ID_SLF_BOM_BARRIER
    __("Climax (Air)"),                         // 0x39 /  57  ATCK_ID_SLF_AIR_BOM_BARRIER
    __("Discipline"),                           // 0x3A /  58  ATCK_ID_SLF_FUNNEL
    __("Bondage?"),                             // 0x3B /  59  ATCK_ID_SLF_CONCENTRATED_LAUNCH
    __("Ebony & Ivory Shoot (Ground)"),         // 0x3C /  60  ATCK_ID_EI_SHOT
    __("Ebony & Ivory Charged Shot Lv1 (G)"),   // 0x3D /  61  ATCK_ID_EI_CSHOT_LV1
    __("Ebony & Ivory Charged Shot Lv2 (G)"),   // 0x3E /  62  ATCK_ID_EI_CSHOT_LV2
    __("Ebony & Ivory Charged Shot Lv3 (G)"),   // 0x3F /  63  ATCK_ID_EI_CSHOT_LV3
    __("Ebony & Ivory Shoot (Air)"),            // 0x40 /  64  ATCK_ID_EI_AIR_SHOT
    __("Ebony & Ivory Charged Shot Lv1 (A)"),   // 0x41 /  65  ATCK_ID_EI_AIR_CSHOT_LV1
    __("Ebony & Ivory Charged Shot Lv2 (A)"),   // 0x42 /  66  ATCK_ID_EI_AIR_CSHOT_LV2
    __("Ebony & Ivory Charged Shot Lv3 (A)"),   // 0x43 /  67  ATCK_ID_EI_AIR_CSHOT_LV3
    __("Ebony & Ivory Twosome Time"),           // 0x44 /  68  ATCK_ID_SEI_TWO_SOMETIME
    __("Ebony & Ivory Honeycomb Fire"),         // 0x45 /  69  ATCK_ID_SEI_HONEYCOMB
    __("Ebony & Ivory Rainstorm"),              // 0x46 /  70  ATCK_ID_SEI_RAIN_STORM
    __("Coyote Ace Shoot (Ground)"),            // 0x47 /  71  ATCK_ID_CA_SHOT
    __("Coyote Ace Charged Shot Lv1 (G)"),      // 0x48 /  72  ATCK_ID_CA_CSHOT_LV1
    __("Coyote Ace Charged Shot Lv2 (G)"),      // 0x49 /  73  ATCK_ID_CA_CSHOT_LV2
    __("Coyote Ace Charged Shot Lv3 (G)"),      // 0x4A /  74  ATCK_ID_CA_CSHOT_LV3
    __("Coyote Ace Shoot (Air)"),               // 0x4B /  75  ATCK_ID_CA_AIR_SHOT
    __("Coyote Ace Charged Shot Lv1 (A)"),      // 0x4C /  76  ATCK_ID_CA_AIR_CSHOT_LV1
    __("Coyote Ace Charged Shot Lv2 (A)"),      // 0x4D /  77  ATCK_ID_CA_AIR_CSHOT_LV2
    __("Coyote Ace Charged Shot Lv3 (A)"),      // 0x4E /  78  ATCK_ID_CA_AIR_CSHOT_LV3
    __("Coyote Ace Fireworks (Ground)"),        // 0x4F /  79  ATCK_ID_SCA_NUNCHAKU
    __("Coyote Ace Fireworks (Air)"),           // 0x50 /  80  ATCK_ID_SCA_AIR_NUNCHAKU
    __("Coyote Ace Backslide"),                 // 0x51 /  81  ATCK_ID_SCA_BACK_SHOT
    __("Coyote Ace Gun Stinger"),               // 0x52 /  82  ATCK_ID_SCA_STINGER
    __("PF013: Epidemic"),                      // 0x53 /  83  ATCK_ID_PD_LAUNCHER
    __("PF124: Hatred"),                        // 0x54 /  84  ATCK_ID_PD_LAUNCHER2
    __("PF398: Revenge"),                       // 0x55 /  85  ATCK_ID_PD_LAUNCHER3
    __("PF262: Jealousy"),                      // 0x56 /  86  ATCK_ID_PD_AIR_MACHINE_GUN
    __("ATCK_ID_SPD_BATTERY"),                  // 0x57 /  87  ATCK_ID_SPD_BATTERY
    __("PF594: Argument"),                      // 0x58 /  88  ATCK_ID_SPD_METEOR
    __("PF422: Grief"),                         // 0x59 /  89  ATCK_ID_SPD_BOOMERANG
    __("PF666: Omen"),                          // 0x5A /  90  ATCK_ID_SPD_PANDORA_BOX
    __("Trickster Dash"),                       // 0x5B /  91  ATCK_ID_TS_DASH
    __("Sky Star"),                             // 0x5C /  92  ATCK_ID_TS_AIR_DASH
    __("Air Trick"),                            // 0x5D /  93  ATCK_ID_TS_AIRTRICK
    __("Flipper?"),                             // 0x5E /  94  ATCK_ID_TS_BACK_WARP
    __("Release (Bronze No Target)"),           // 0x5F /  95  ATCK_ID_RG_RELEASE
    __("Release (Silver Missed)"),              // 0x60 /  96  ATCK_ID_RG_GUARD_RELEASE
    __("Release (Gold Just)"),                  // 0x61 /  97  ATCK_ID_RG_JUST_RELEASE
    __("Release Air (Bronze)"),                 // 0x62 /  98  ATCK_ID_RG_AIR_RELEASE
    __("Release Air (Silver)"),                 // 0x63 /  99  ATCK_ID_RG_AIR_GUARD_RELEASE
    __("Release Air (Gold)"),                   // 0x64 / 100  ATCK_ID_RG_AIR_JUST_RELEASE
    __("Dreadnaught"),                          // 0x65 / 101  ATCK_ID_RG_INVINCIBLE
    __("Yamato Combo A1"),                      // 0x66 / 102  ATCK_ID_YT_COMBO0
    __("Yamato Combo A2"),                      // 0x67 / 103  ATCK_ID_YT_COMBO1
    __("Yamato Combo A3"),                      // 0x68 / 104  ATCK_ID_YT_COMBO2
    __("Aerial Rave V1"),                       // 0x69 / 105  ATCK_ID_YT_AIR_COMBO0
    __("Aerial Rave V2"),                       // 0x6A / 106  ATCK_ID_YT_AIR_COMBO1
    __("Slash Dimension (Far)"),                // 0x6B / 107  ATCK_ID_YT_SPACE
    __("Slash Dimension (Close)"),              // 0x6C / 108  ATCK_ID_YT_SPACE2
};

const char* directionMappingNames[] = {
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
    "28 -",
    "29 -",
    "30 -",
    "31 -",
    __("32 - Gun Release"),
    __("33 - Gun Charged Release"),
    __("34 - Lockon + Gun Charge"),
    __("35 - Gun Charged"),
    __("36 - Gun Charge3"),
    __("37 - Gun Charge4"),
    __("38 - Gun Charge3"),
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
    __("81 - Melee + Style"),
    __("82 - Lockon + Backforward + Melee"),
    "83 -",
    __("84 - Gun")
};

const char* attackNames[] = {
    "0 -",
};

const char* weaponNames[] = {
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

const char* styleNames[] = {
    __("FF - None"),
    __("0 - Swordmaster"),
    __("1 - Gunslinger"),
    __("2 - Trickster"),
    __("3 - Royal Guard"),
    __("4 - Darkslayer"),
};

const char* aerialNames[] = {
    __("0 - None"),
    __("1 - Ground"),
    __("2 - Air"),
    __("3 - Ground or air"),
};

const char* restrictNames[] = {
    __("0 - None?"),
    __("1 - None?"),
    __("2 - Disabled?"),
    __("3 -"),
    __("4 - Height Restricted"),
};

const char* dtNames[] = {
    __("0 - DT or no DT"),
    __("1 - DT or no DT 2"),
    __("2 - Disabled?"),
    __("3 - DT Only"),
    __("4 -"),
    __("5 -"),
    __("6 -"),
    __("7 -"),
    __("8 -"),
    __("9 - DT or Charge"),
    __("10 -"),
    __("11 - DT or Charge 2"),
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

size_t dante_attack_names_arraysize()   { return IM_ARRAYSIZE(dante_attack_names);    };
size_t directionMappingNamesArraySize() { return IM_ARRAYSIZE(directionMappingNames); };
size_t attackNamesArraySize()           { return IM_ARRAYSIZE(attackNames);           };
size_t weaponNamesArraySize()           { return IM_ARRAYSIZE(weaponNames);           };
size_t styleNamesArraySize()            { return IM_ARRAYSIZE(styleNames);            };
size_t aerialNamesArraySize()           { return IM_ARRAYSIZE(aerialNames);           };
size_t restrictNamesArraySize()         { return IM_ARRAYSIZE(restrictNames);         };
size_t dtNamesArraySize()               { return IM_ARRAYSIZE(dtNames);               };
