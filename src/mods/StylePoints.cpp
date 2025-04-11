#include "StylePoints.hpp"
#include <Config.hpp>

// texture
#include "utility/Dx9Utils.hpp"
#include "utility/Compressed.hpp"
#include "misc/TonyBg.cpp"
static int bg_red_int   = 255;
static int bg_green_int = 255;
static int bg_blue_int  = 255;
static int bg_alpha_int = 255;
void StylePoints::on_reset() {
    spdlog::info("StylePoints::on_reset()");
    if (m_texture_handle != nullptr) {
        m_texture_handle->Release();
        spdlog::info("StylePoints::m_texture_handle->Release()");
        m_texture_handle = nullptr;
    }
}
void StylePoints::after_reset() {
    auto [data, size] = utility::decompress_file_from_memory_with_size(tony_bg_compressed_data, tony_bg_compressed_size);

    if (!utility::dx9::load_texture_from_file(data, size, &m_texture_handle, &m_texture_width, &m_texture_height)) {
        spdlog::error("Failed to unpack and load compressed texture");
    }
    free(data);
}

static std::unordered_map<std::string, std::string> textLookupTable = {
    // trickster
    {"TS-ehsuc",         "Mustang"},
    // royal guard
    {"RELEASE_A",        "Release"}, // empty bar
    {"RELEASE_B",        "Release"}, // any bar
    {"RELEASE_C",        "Release"}, // 10k or above
    {"RELEASE_D",        "Release"}, // releasing an attack at 0-10k gauge
    {"RELEASE_E",        "Release"}, // releasing an attack at 10k or above gauge

    // rebellion
    {"REBE-combo303",    "Rebellion Combo A"}, // A1
    {"REBE-combo304",    "Rebellion Combo A"}, // A2
    {"REBE-combo305",    "Rebellion Combo A"}, // A3
    {"REBE-stings",      "Stinger"},
    {"REBE-sting",       "Stinger Hit"},
    {"REBE-stab",        "Million Stab"},
	{"REBE-roundt",      "Round Trip"},
    {"REBE-hightime",    "High Time"},
    {"REBE-combo306-1",  "Rebellion Combo B"},
    {"REBE-combo306-2",  "Rebellion Combo B"},
    {"REBE-combo306-3",  "Rebellion Combo B"},
    {"REBE-stabf",       "Million Stab End"},
    {"REBE-helmbrkLV1",  "Helm Breaker Lv.1"},
    {"REBE-helmbrkLV2",  "Helm Breaker Lv.2"},
    {"REBE-helmbrkLV3",  "Helm Breaker Lv.3"},
    {"REBE-helmbrkf",    "Helm Breaker"},
    {"REBE-DTstg1",      "DT Stinger"},
    {"REBE-DTstg2",      "DT Stinger"},
    {"REBE-DTstg3",      "DT Stinger"},
    {"REBE-DTstg4",      "DT Stinger"},
    {"REBE-DTstg5",      "DT Stinger"},

    // rebellion swordmaster
    {"REBE-props1",      "Prop"},
    {"REBE-props20",     "Shredder"},
    {"REBE-props21",     "Shredder"},
    {"REBE-dm363",       "Dance Macabre"}, // 1
    {"REBE-dm365",       "Dance Macabre"}, // 2
    {"REBE-dm366",       "Dance Macabre"}, // 3
    {"REBE-dm367",       "Dance Macabre"}, // 4
    {"REBE-dm368-1",     "Dance Macabre"}, // 5-1
    {"REBE-dm368-2",     "Dance Macabre"}, // 5-2
    {"REBE-dm368-3",     "Dance Macabre"}, // 5-3
    {"REBE-dm369",       "Dance Macabre"}, // stab
    {"REBE-dm370",       "Dance Macabre"}, // 7
    {"REBE-dm371",       "Dance Macabre"}, // 8
    {"REBE-dm372-1",     "Dance Macabre"}, // 9
    {"REBE-dm373",       "Dance Macabre"}, // Million stab end
    {"REBE-dm364",       "Dance Macabre"}, // End
    {"REBE-arial353",    "Aerial Rave"}, // 1
    {"REBE-arial354",    "Aerial Rave"}, // 2
    {"REBE-arial355",    "Aerial Rave"}, // 3
    {"REBE-arial356",    "Aerial Rave"}, // 4
	{"REBE-drivesw1",	 "Drive"},
	{"REBE-drive1",		 "Drive"},
	{"REBE-drive1-2",	 "Drive Lv.2"},
	{"REBE-drive1-3",	 "Drive Lv.3"},
	{"REBE-drivesw2",	 "Overdrive"},
	{"REBE-drive2",		 "Overdrive"},
	{"REBE-drive2-2",	 "Overdrive Lv.2"},
	{"REBE-drive2-3",	 "Overdrive Lv.3"},
	{"REBE-drivesw3",	 "Drive"},
	{"REBE-drive3",		 "Overdrive"},
	{"REBE-drive3-2",	 "Overdrive Lv.2"},
	{"REBE-drive3-3",	 "Overdrive Lv.3"},
	{"REBE-qdrive",		 "Quick Drive"},

    // gilgamesh
	{"GIL-rblaze",       "Flush"},
	{"GIL-rblaze-f",     "Flush"},
	{"GIL-rblaze-b",     "Flush"},
    {"GIL-klbe",         "Full House"},
	{"GIL-combo403",     "Gilgamesh Combo A"}, // A1
    {"GIL-combo464",     "Gilgamesh Combo A Lv.2"}, // A1 Lv.2
    {"GIL-combo465",     "Gilgamesh Combo A Lv.3"}, // A1 Lv.3
	{"GIL-combo404",     "Gilgamesh Combo A"}, // A2
    {"GIL-combo466",     "Gilgamesh Combo A Lv.2"}, // A2 Lv.2
    {"GIL-combo467",     "Gilgamesh Combo A Lv.3"}, // A2 Lv.3
	{"GIL-combo405",     "Gilgamesh Combo A"}, // A3
    {"GIL-combo476",     "Gilgamesh Combo A Lv.2"}, // A3 Lv.2
    {"GIL-combo477",     "Gilgamesh Combo A Lv.3"}, // A3 Lv.3
	{"GIL-combo406",     "Gilgamesh Combo A"}, // A4
    {"GIL-combo478",     "Gilgamesh Combo A Lv.2"}, // A4 Lv.2
    {"GIL-combo479",     "Gilgamesh Combo A Lv.3"}, // A4 Lv.3
	{"GIL-combo407",     "Gilgamesh Combo B"}, // B1 (kick spam)
    {"GIL-combo480",     "Gilgamesh Combo B Lv.2"}, // B1 Lv.2
    {"GIL-combo481",     "Gilgamesh Combo B Lv.3"}, // B1 Lv.3
	{"GIL-combo409-1",   "Gilgamesh Combo B"}, // B2-1
	{"GIL-combo409-2",   "Gilgamesh Combo B"}, // B2-2
    {"GIL-combo482-1",   "Gilgamesh Combo B Lv.2"}, // B2-1 Lv.2
    {"GIL-combo482-2",   "Gilgamesh Combo B Lv.2"}, // B2-2 Lv.2
    {"GIL-combo483-1",   "Gilgamesh Combo B Lv.3"}, // B2-1 Lv.3
    {"GIL-combo483-2",   "Gilgamesh Combo B Lv.3"}, // B2-2 Lv.3
	{"GIL-straight422",  "Straight"},
	{"GIL-straight2",    "Straight Lv.2"},
	{"GIL-straight3",    "Straight Lv.3"},
	{"GIL-k13r-01",      "Kick 13"},
	{"GIL-k13r-02",      "Kick 13"},
	{"GIL-k13r-03",      "Kick 13"},
	{"GIL-k13r-04",      "Kick 13"},
	{"GIL-k13r-05",      "Kick 13"},
	{"GIL-k13rDT-01",    "DT Kick 13"},
	{"GIL-k13rDT-02",    "DT Kick 13"},
	{"GIL-k13rDT-03",    "DT Kick 13"},
	{"GIL-k13rDT-04",    "DT Kick 13"},
	{"GIL-k13rDT-05",    "DT Kick 13"},
	{"GIL-k13rDT-06",    "DT Kick 13"},

    // gilgamesh swordmaster
	{"GIL-mgdv450",  	 "Beast Uppercut"},
	{"GIL-rgdg460",  	 "Rising Dragon"},
	{"GIL-dvdg461",  	 "Divine Dragon"},
	{"GIL-dvdg462",  	 "Divine Dragon"},
	{"GIL-rlimpct-01",   "Real Impact"},
	{"GIL-rlimpct-02",   "Real Impact"},
	{"GIL-rlimpct-03",   "Real Impact"},
	{"GIL-inferno451",   "Shock"},
	{"GIL-inferno470",   "Shock Lv.2"},
	{"GIL-inferno471",   "Shock Lv.3"},

    // lucifer
    {"NORMAL",           "Embed"}, // pins and pandora's normal shot
    {"Bomb",             "Pin Explosion"},
    {"ROSE",             "Rose"},
	{"LUCI-combo503",    "Lucifer A1"},
	{"LUCI-combo504",    "Lucifer A2"},
	{"LUCI-combo505",    "Lucifer A3"},
	{"LUCI-combo506-1",  "Lucifer A4"},
    {"LUCI-combo506-2",  "Lucifer A4"},
	{"LUCI-combo519-1",  "Lucifer B1"},
	{"LUCI-combo519-2",  "Lucifer B1"},
	{"LUCI-combo508", 	 "Lucifer E1"},
	{"LUCI-combo509", 	 "Lucifer E2"},
	{"LUCI-combo510-1",  "Lucifer E3"},
	{"LUCI-combo510-2",  "Lucifer E3"},
    {"LUCI-combo514-1",  "Splash"},
    {"LUCI-combo514-2",  "Splash"},
    {"LUCI-combo514-3",  "Splash"},

    // lucifer swordmaster
	{"Funnel", 			 "Discipline & Bondage"},
	{"Stand",  			 "Climax"},
	{"BombBariier",  	 "Climax"},

    // e&i
    {"SHL000",           "E&I"},
    {"SHL00-LEN2",       "E&I"},
    {"SHL00_LEN3",       "E&I"},
    {"CHARGE",           "Charged E&I"},
    {"CHARGE_LEN2",      "Charged E&I"},
    {"CHARGE_LEN3",      "Charged E&I"},

    // e&i gunslinger
    {"RAIN_STORM",       "Rainstorm"},
    {"RAIN_STORM_LEN2",  "Rainstorm"},
    {"RAIN_STORM_LEN3",  "Rainstorm"},
    {"DT_RAIN_STORM",    "DT Rainstorm"},
    {"DT_RAIN_STORM_L",  "DT Rainstorm"},
	{"TWO_SOMETIME",     "Twosome Time"},
    {"TWO_SOMETIME_L2",  "Twosome Time"},
    {"TWO_SOMETIME_L3",  "Twosome Time"},
	{"DT_TWO_SOMETIME",  "DT Twosome Time"},
    {"HONEY_COMB",       "Honeycomb Fire"},
    {"HONEY_COMB_LEN2",  "Honeycomb Fire"},
    {"HONEY_COMB_LEN3",  "Honeycomb Fire"},
    {"DT_HONEY_COMB",    "DT Honeycomb Fire"},
    {"DT_HONEY_COMB_L",  "DT Honeycomb Fire"},

    // coyote
    {"NRML_ETC_LV1",     "Coyote"},
    {"NRML_CEN_LV1",     "Coyote"},
    {"NRML_ETC_LV2",     "Coyote"},
    {"NRML_CEN_LV2",     "Coyote"},
    {"NRML_ETC_LV3",     "Coyote"},
    {"NRML_CEN_LV3",     "Coyote"},
	{"CHRG_ETC_LV1",     "DT Coyote"},
    {"CHRG_CEN_LV1",     "DT Coyote"},
    {"CHRG_ETC_LV2",     "DT Coyote"},
    {"CHRG_CEN_LV2",     "DT Coyote"},
    {"CHRG_ETC_LV3",     "DT Coyote"},
    {"CHRG_CEN_LV3",     "DT Coyote"},

    // coyote gunslinger
	{"BACK_ETC_LV1",     "Backslide"},
	{"BACK_CEN_LV1",     "Backslide"},
	{"BACK_ETC_LV2",     "Backslide"},
	{"BACK_CEN_LV2",     "Backslide"},
	{"BACK_ETC_LV3",     "Backslide"},
	{"BACK_CEN_LV3",     "Backslide"},
    {"DTBC_ETC_LV1",     "DT Backslide"},
    {"DTBC_CEN_LV1",     "DT Backslide"},
    {"DTBC_ETC_LV2",     "DT Backslide"},
    {"DTBC_CEN_LV2",     "DT Backslide"},
    {"DTBC_ETC_LV3",     "DT Backslide"},
    {"DTBC_CEN_LV3",     "DT Backslide"},
	{"NCHA_STR_LV1",     "Fireworks"},
	{"NCHA_STR_LV2",     "Fireworks"},
	{"NCHA_STR_LV3",     "Fireworks"},
	{"NCHA_WEK_LV1",     "Fireworks"},
	{"NCHA_WEK_LV2",     "Fireworks"},
	{"NCHA_WEK_LV3",     "Fireworks"},
	{"DTNC_STR_LV1",     "DT Fireworks"},
	{"DTNC_STR_LV2",     "DT Fireworks"},
	{"DTNC_STR_LV3",     "DT Fireworks"},
	{"DTNC_WEK_LV1",     "DT Fireworks"},
	{"DTNC_WEK_LV2",     "DT Fireworks"},
	{"DTNC_WEK_LV3",     "DT Fireworks"},
	{"STG-gunstinger",   "Gun Stinger"},
	{"STNG_ETC_LV1",     "Gun Stinger"},
	{"STNG_CEN_LV1",     "Gun Stinger"},
	{"STNG_ETC_LV2",     "Gun Stinger"},
	{"STNG_CEN_LV2",     "Gun Stinger"},
	{"STNG_ETC_LV3",     "Gun Stinger"},
	{"STNG_CEN_LV3",     "Gun Stinger"},
	{"DTST_ETC_LV1",     "DT Gun Stinger"},
	{"DTST_CEN_LV1",     "DT Gun Stinger"},
	{"DTST_ETC_LV2",     "DT Gun Stinger"},
	{"DTST_CEN_LV2",     "DT Gun Stinger"},
	{"DTST_ETC_LV3",     "DT Gun Stinger"},
	{"DTST_CEN_LV3",     "DT Gun Stinger"},

    // pandora
    {"MACHINEGUN",       "Jealousy"},
    {"MACHINEGUN_LV2",   "Jealousy"},
    {"MACHINEGUN_LV3",   "Jealousy"},
    {"DT_MGUN",          "DT Jealousy"},
    {"DT_MGUN_LV2",      "DT Jealousy"},
    {"DT_MGUN_LV3",      "DT Jealousy"},
	{"BOM_NOR",          "Epidemic"},
    {"NORMAL02",         "Hatred"}, // embed
	{"BOM_NOR02",        "Hatred"},
	{"NORMAL03",         "Revenge"},
    {"PDR-lanc3tf",      "Revenge Hit"}, // physical hit
	
    // pandora gunslinger
	{"METEOR",        	 "Argument"},
	{"BOM_METEOR",       "Argument"},
	{"PDR-ma666s",  	 "Omen Hit"},
    {"PDR-ma666e",  	 "Omen Back Hit"}, // behind physical hit
	{"BOOMERANG",        "Grief"},

    // yamato
	{"YMT-combo1003",    "Yamato Combo A"},
	{"YMT-combo1004",    "Yamato Combo A"},
	{"YMT-combo1005",    "Yamato Combo A"},
    {"YMT-combo1006-1",  "Yamato Aerial Rave V"},
    {"YMT-combo1006-2",  "Yamato Aerial Rave V"},
    {"YMT-combo1007",    "Yamato Aerial Rave V"},
	{"kuukan",   		 "Slash Dimension"},
//};

//std::unordered_map<std::string, std::string> neroLookupTable = { // uh some things can go off after a char switch
    {"Grab-Attack",      "Collateral"},     // enemy hit by bustered scarecrow
    {"Grab-Attack-DT",   "Collateral"},     // enemy hit by bustered scarecrow
    {"em000_001_dt_th",  "Collateral"},     // enemy hit by bustered scarecrow (air + dt)
    {"makikomi158",      "Collateral"},     // enemy hit by bustered mega
    {"BusterBlown",      "Collateral"},     // enemy hit by bustered bianco
    {"BusterExplosion",  "Collateral"},     // enemy hit by bustered bianco
    {"D_Buster",         "Collateral"},     // enemy hit by bustered bianco
    {"BusterImpact",     "Collateral"},     // enemy hit by bustered alto
    {"Blown",            "Collateral"},     // enemy hit by bustered alto
    {"Em008-rolled",     "Collateral"},     // enemy hit by bustered mephisto
    {"Em009-rolled",     "Collateral"},     // enemy hit by bustered faust
    {"em010Throw",       "Collateral"},     // enemy hit by bustered frost
    {"Buster0Atk",       "Collateral"},     // enemy hit by bustered frost
    {"Buster1Atk",       "Collateral"},     // enemy hit by bustered frost
    {"grabed-attack",    "Collateral"},     // enemy hit by bustered assault
    {"grabed-at-fin",    "Collateral"},     // enemy hit by bustered assault
    {"ShootNero",        "Collateral"},     // enemy hit by bustered basilisk
    {"ShootNeroAir",     "Collateral"},     // enemy hit by bustered basilisk
    {"BusterThrow",      "Collateral"},     // enemy hit by bustered gladius
    {"D_BusterSlash",    "Collateral"},     // enemy hit by bustered gladius (dt)
    {"\xA0\x4F\xBE",     "Hold Block"},     // held enemy hit by scarecrow, reads �O�
    {"\x90\x8B\xBA",     "Hold Block"},     // held enemy hit by seed, reads ???

    // scarecrow
    {"Em000",            "Scarecrow Buster"}, // non dt
    {"Em000_1",          "Scarecrow Buster"}, // dt ground
    {"Em000_2",          "Scarecrow Buster"}, // dt air

    // mega
    {"Em003",            "Mega Buster"},
    {"Em003_0",          "Mega Buster"},
    {"Em003_1",          "Mega Buster"},
    {"Em003_2",          "Mega Buster"},
    {"Em003_3",          "Mega Buster"},
    {"BUS-Em003",        "Mega Buster"},
    {"Em003-roll_coun",  "Mega Counter"},

    // bianco
    {"Em005",            "Bianco Buster"},
    {"Em005_2",          "Bianco Buster"},
    {"Em005_3",          "Bianco Buster"},
    {"Em005_4",          "Bianco Buster"},
    {"BusterThrust",     "Bianco Buster"},
    {"Em005Majin",       "Bianco Buster"},
    {"Em005Majin_2",     "Bianco Buster"},
    {"Em005Majin_3",     "Bianco Buster"},
    {"Em005Majin_4",     "Bianco Buster"},

    // alto
    {"Em006",            "Alto Buster"},
    {"Em006_2",          "Alto Buster"},
    {"Em006Majin",       "Alto Buster"},
    {"Em006Majin_2",     "Alto Buster"},
    {"Em006Majin_3",     "Alto Buster"},
    {"Em006Flip",        "Alto Buster"}, // if alto jumps back when you buster

    // mephisto
    {"Em008",            "Mephisto Buster"},
    {"Em008_2",          "Mephisto Buster"},
    {"Em008_3",          "Mephisto Buster"},

    // faust
    {"Em009",            "Faust Buster"},
    {"Em009_2",          "Faust Buster"},
    {"Em009_3",          "Faust Buster"},

    // frost
    {"Em010Release",     "Frost Buster"},
    {"Em010WallHit",     "Frost Buster"},

    // assault
    {"Em011Grab",        "Assault Buster"},
    {"Em011Grab_Fin",    "Assault Buster"},

    // blitz
    {"BUS-Em012_Gr_00",  "Blitz Buster"}, // ground
    {"BUS-Em012_Gr_01",  "Blitz Buster"}, // ground
    {"BUS-Em012_Gr_02",  "Blitz Buster"}, // ground
    {"BUS-Em012_Gr_03",  "Blitz Buster"}, // ground
    {"BUS-Em012_Gr_04",  "Blitz Buster"}, // ground
    {"BUS-Em012_Gr_05",  "Blitz Buster"}, // ground
    {"BUS-Em012_Gr_06",  "Blitz Buster"}, // ground
    {"BUS-Em012_Gr_07",  "Blitz Buster"}, // ground
    {"BUS-Em012_Ai_00",  "Blitz Buster"}, // air
    {"BUS-Em012_Ai_01",  "Blitz Buster"}, // air
    {"BUS-Em012_Ai_02",  "Blitz Buster"}, // air
    {"BUS-Em012_Ai_03",  "Blitz Buster"}, // air
    {"BUS-Em012_Ai_04",  "Blitz Buster"}, // air
    {"BUS-Em012_Ai_05",  "Blitz Buster"}, // air
    {"BUS-Em012_Ai_06",  "Blitz Buster"}, // air
    {"BUS-Em012_Ai_07",  "Blitz Buster"}, // air

    // chimera seed
    {"Em013",            "Chimera Seed Buster"},

    // cutlass
    {"Em015",            "Cutlass Buster"},

    // gladius
    {"Em016",            "Gladius Buster"},

    // basilisk
    {"Em017",            "Basilisk Buster"},

    // berial
    {"Em018Head",        "Berial Buster"},
    {"Em018Leg",         "Berial Buster"},
    {"Em018Leg2",        "Berial Buster"},
    {"Em018Head_maji",   "Berial Buster"},
    {"Em018Head_maji2",  "Berial Buster"},
    {"Em018Down_maji",   "Berial Buster"},
    {"Em018Down_maji2",  "Berial Buster"},

    // frog
    {"Em019Tail",        "Frog Buster"},
    {"Em019Tail2",       "Frog Buster"},
    {"Em019TailMaj",     "Frog Buster"},
    {"Em019TailMaj2",    "Frog Buster"},
    {"Em019Ranbu",       "Frog Buster"},
    {"Em019Ranbu2",      "Frog Buster"},
    {"Em019Ranbu3",      "Frog Buster"},
    {"Em019Ranbu4",      "Frog Buster"},
    {"Em019Ranbu5",      "Frog Buster"},
    {"Em019Ranbu6",      "Frog Buster"},
    {"Em019Ranbu7",      "Frog Buster"},
    {"Em019Ranbu8",      "Frog Buster"},
    {"Em019Ranbu9",      "Frog Buster"},
    {"Em019RanbuMaj",    "Frog Buster"},
    {"Em019RanbuMaj2",   "Frog Buster"},
    {"Em019RanbuMaj3",   "Frog Buster"},
    {"Em019RanbuMaj4",   "Frog Buster"},
    {"Em019RanbuMaj5",   "Frog Buster"},
    {"Em019RanbuMaj6",   "Frog Buster"},
    {"Em019RanbuMaj7",   "Frog Buster"},
    {"Em019RanbuMaj8",   "Frog Buster"},
    {"Em019RanbuMaj9",   "Frog Buster"},
    {"Em019RanbuMaj10",  "Frog Buster"},
    {"Em019RanbuMaj12",  "Frog Buster"},
    {"Em019RanbuMaj13",  "Frog Buster"},
    {"Em019RanbuMaj14",  "Frog Buster"},

    // echidna
    {"Em021Norm1",       "Echidna Buster"},
    {"Em021Norm2",       "Echidna Buster"},
    {"Em021Norm3",       "Echidna Buster"},
    {"Em021Norm4",       "Echidna Buster"},
    {"Em021Norm6",       "Echidna Buster"},
    {"Em021Norm8",       "Echidna Buster"},
    {"Em021Norm_Ryu1",   "Echidna Buster"},
    {"Em021Norm_Ryu2",   "Echidna Buster"},
    {"Em021Norm_Ryu3",   "Echidna Buster"},
    {"Em021Norm_Ryu4",   "Echidna Buster"},
    {"Em021Norm_Ryu6",   "Echidna Buster"},
    {"Em021Maji_Ryu1",   "Echidna Buster"},
    {"Em021Maji_Ryu2",   "Echidna Buster"},
    {"BUS-Em021_Bl_00",  "Echidna Buster"},
    {"BUS-Em021_Bl_01",  "Echidna Buster"},
    {"BUS-Em021_Bl_02",  "Echidna Buster"},
    {"BUS-Em021_Bl_03",  "Echidna Buster"},
    {"BUS-Em021_Bl_04",  "Echidna Buster"},
    {"BUS-Em021_Bl_05",  "Echidna Buster"},
    {"BUS-Em021_Bl_06",  "Echidna Buster"},
    {"BUS-Em021_Bl_07",  "Echidna Buster"},
    {"Em021Maji1",       "Echidna Buster"},
    {"Em021Maji2",       "Echidna Buster"},
    {"Em021Maji3",       "Echidna Buster"},
    {"Em021Maji4",       "Echidna Buster"},
    {"Em021Maji5",       "Echidna Buster"},
    {"Em021Maji6",       "Echidna Buster"},
    {"Em021Maji7",       "Echidna Buster"},
    {"Em021Maji8",       "Echidna Buster"},
    {"SeedPLAtk",        "Seed Rebound"},

    // credo
    {"SpearBuster_PL",   "Credo Buster"},
    {"BUS-Em022_Flip",   "Credo Buster"},
    {"BUS-Em022_00",     "Credo Buster"},
    {"BUS-Em022_01",     "Credo Buster"},
    {"BUS-Em022_02",     "Credo Buster"},
    {"BUS-Em022_03",     "Credo Buster"},
    {"BUS-Em022_04",     "Credo Buster"},
    {"BUS-Em022_Mj_00",  "Credo Buster"},
    {"BUS-Em022_Mj_01",  "Credo Buster"},
    {"BUS-Em022_Mj_02",  "Credo Buster"},
    {"BUS-Em022_Mj_03",  "Credo Buster"},
    {"BUS-Em022_Mj_04",  "Credo Buster"},

    // agnus
    {"BusterLoop",       "Agnus Buster"},
    {"Sword",            "Agnus Buster"},
    {"EM023Norm1",       "Agnus Buster"},
    {"EM023Norm2",       "Agnus Buster"},
    {"EM023Norm3",       "Agnus Buster"},
    {"EM023Norm4",       "Agnus Buster"},
    {"EM023Norm5",       "Agnus Buster"},
    {"EM023Norm6",       "Agnus Buster"},
    {"Em023Majin1",      "Agnus Buster"},
    {"Em023Majin2",      "Agnus Buster"},
    {"Em023Majin3",      "Agnus Buster"},
    {"Em023Majin4",      "Agnus Buster"},
    {"Em023Majin5",      "Agnus Buster"},
    {"Em023Majin6",      "Agnus Buster"},
    {"Em023Majin7",      "Agnus Buster"},

    // dante
    {"EmDanteMaj",       "Dante Buster"},
    {"EmDanteMajAir",    "Dante Buster"},
    {"EmDanteMajFin",    "Dante Buster"},
    {"EmDanteNorm",      "Dante Buster"},
    {"EmDanteThrow",     "Dante Buster"}, // after sword clashes

    // red queen
    {"RED-ComboA_00",    "Red Queen Combo A"},
    {"RED-ComboA_01",    "Red Queen Combo A"},
    {"RED-ComboA_02",    "Red Queen Combo A"},
    {"RED-ComboA_03",    "Red Queen Combo A"},
    {"RED-ComboB_00",    "Red Queen Combo B"},
    {"RED-ComboB_01",    "Red Queen Combo B"},
    {"RED-ComboB_02_A",  "Red Queen Combo B"},
    {"RED-ComboB_02_B",  "Red Queen Combo B"},
    {"RED-ComboB_02_C",  "Red Queen Combo B"},
    {"RED-ComboB_03",    "Red Queen Combo B"},
    {"RED-ComboB_04",    "Red Queen Combo B"},
    {"RED-ComboC_00",    "Red Queen Combo C"},
    {"RED-ComboC_01",    "Red Queen Combo C"},
    {"RED-ComboC_02",    "Red Queen Combo C"},
    {"RED-ComboC_03",    "Red Queen Combo C"},
    {"RED-ComboC_04",    "Red Queen Combo C"},
    {"RED-ComboD_00",    "Red Queen Combo D"},
    {"RED-ComboD_01",    "Red Queen Combo D"},
    {"RED-Streak",       "Streak"},
    {"RED-StreEX_00",    "EX Streak"},
    {"RED-StreEX_01",    "EX Streak"},        // no clarification on ex2 or 3
    {"RED-Highro",       "High Roller"},      
    {"RED-HighroEX_00",  "EX High Roller"},   // grounded 1, 2, 3
    {"RED-HighroEX_01",  "EX High Roller"},   // aerial      2
    {"RED-HighroEX_03",  "EX High Roller"},   // grounded    2, 3
    {"RED-HighroEX_04",  "EX High Roller"},   // grounded       3
    {"RED-HighroEX_05",  "EX High Roller"},   // aerial      2, 3
    {"RED-HighroEX_06",  "EX High Roller"},   // aerial      2
    {"RED-Split_00",     "Split"},            // aerial
    {"RED-Split_01",     "Split"},            // grounded
    {"RED-SplitEX_00",   "EX Split"},         // 1, 2, 3
    {"RED-SplitEX_01",   "EX Split"},         // 1
    {"RED-SplitEX_02",   "EX Split"},         //    2
    {"RED-SplitEX_03",   "EX Split"},         //       3
    {"RED-Surren_00",    "Shuffle"},
    {"RED-Surren_01",    "Shuffle"},          // weak hit
    {"RED-SureenEX_00",  "EX Shuffle"},       // EX1
    {"RED-SureenEX_01",  "EX Shuffle"},       // EX1 // rare, I assume weak hit or back hit
    {"RED-SureenEX_02",  "EX Shuffle"},       // EX1
    {"RED-SureenEX_03",  "EX Shuffle"},       // EX1 // rare, I assume weak hit or back hit
    {"RED-SureenEX_04",  "EX Shuffle"},       // EX2
    {"RED-SureenEX_05",  "EX Shuffle"},       // EX2 // rare, I assume weak hit or back hit
    {"RED-SureenEX_06",  "EX Shuffle"},       // EX2
    {"RED-SureenEX_07",  "EX Shuffle"},       // EX2 // rare, I assume weak hit or back hit
    {"RED-SureenEX_08",  "EX Shuffle"},       // EX3
    {"RED-SureenEX_09",  "EX Shuffle"},       // EX3 // rare, I assume weak hit or back hit
    {"RED-SureenEX_10",  "EX Shuffle"},       // EX3
    {"RED-AirStre_00",   "Calibur"},         
    {"RED-AirStEX_00",   "EX Calibur"},       // EX1
    {"RED-AirStEX_01",   "EX Calibur"},       // EX1
    {"RED-AirStEX_02",   "EX Calibur"},       // EX2
    {"RED-AirStEX_03",   "EX Calibur"},       // EX2
    {"RED-AirStEX_04",   "EX Calibur"},       // EX3
    {"RED-AirStEX_05",   "EX Calibur"},       // EX3
    {"RED-AircombA_00",  "Red Queen Aerial"}, // 1
    {"RED-AircombA_01",  "Red Queen Aerial"}, // 2
    {"RED-AircombA_02",  "Red Queen Aerial"}, // 3
    {"RED-RouletteSpi",  "Roulette Spin"},

    // yamato
    {"YAM-ComboA_00",    "Red Queen Combo A"},
    {"YAM-ComboA_01",    "Red Queen Combo A"},
    {"YAM-ComboA_02",    "Red Queen Combo A"},
    {"YAM-ComboA_03",    "Red Queen Combo A"},
    {"YAM-ComboB_00",    "Red Queen Combo B"},
    {"YAM-ComboB_01",    "Red Queen Combo B"},
    {"YAM-ComboB_02_A",  "Red Queen Combo B"},
    {"YAM-ComboB_02_B",  "Red Queen Combo B"},
    {"YAM-ComboB_02_C",  "Red Queen Combo B"},
    {"YAM-ComboB_03",    "Red Queen Combo B"},
    {"YAM-ComboB_04",    "Red Queen Combo B"},
    {"YAM-ComboC_00",    "Red Queen Combo C"},
    {"YAM-ComboC_01",    "Red Queen Combo C"},
    {"YAM-ComboC_02",    "Red Queen Combo C"},
    {"YAM-ComboC_03",    "Red Queen Combo C"},
    {"YAM-ComboC_04",    "Red Queen Combo C"},
    {"YAM-ComboD_00",    "Red Queen Combo D"},
    {"YAM-Streak",       "Streak"},
    {"YAM-StreEX_00",    "EX Streak"},
    {"YAM-StreEX_01",    "EX Streak"},
    {"YAM-Highro",       "High Roller"},
    {"YAM-HighroEX_01",  "EX High Roller"},
    {"YAM-HighroEX_03",  "EX High Roller"},
    {"YAM-HighroEX_04",  "EX High Roller"},
    {"YAM-HighroEX_05",  "EX High Roller"},
    {"YAM-HighroEX_06",  "EX High Roller"},
    {"YAM-AircombA_00",  "Red Queen Aerial"},
    {"YAM-AircombA_01",  "Red Queen Aerial"},
    {"YAM-AircombA_02",  "Red Queen Aerial"},
    {"YAM-RouletteSpi",  "Roulette Spin"},
    {"YAM-AirStre_00",   "Calibur"},
    {"YAM-AirStEX_00",   "EX Calibur"},
    {"YAM-AirStEX_01",   "EX Calibur"},
    {"YAM-AirStEX_02",   "EX Calibur"},
    {"YAM-AirStEX_03",   "EX Calibur"},
    {"YAM-AirStEX_04",   "EX Calibur"},
    {"YAM-AirStEX_05",   "EX Calibur"},
    {"YAM-Surren_00",    "Shuffle"},
    {"YAM-SureenEX_00",  "EX Shuffle"}, // EX1
    {"YAM-SureenEX_01",  "EX Shuffle"}, // EX1
    {"YAM-SureenEX_02",  "EX Shuffle"}, // EX1
    {"YAM-SureenEX_03",  "EX Shuffle"}, // EX2
    {"YAM-SureenEX_04",  "EX Shuffle"}, // EX2
    {"YAM-SureenEX_05",  "EX Shuffle"}, // EX2
    {"YAM-SureenEX_06",  "EX Shuffle"}, // EX3
    {"YAM-SureenEX_08",  "EX Shuffle"}, // EX3
    {"YAM-SureenEX_10",  "EX Shuffle"}, // EX3
    {"YAM-Split_00",     "Split"},
    {"YAM-SplitEX_00",   "EX Split"}, // EX1
    {"YAM-SplitEX_01",   "EX Split"}, // EX2
    {"YAM-SplitEX_03",   "EX Split"}, // EX3

    // blue rose
    {"Shoot_1st_Short",  "Blue Rose"},
    {"Shoot_2nd_short",  "Blue Rose"},
    {"Shoot_1st_Midd",   "Blue Rose"},
    {"Shoot_2nd_midd",   "Blue Rose"},
    {"Shoot_1st_Long",   "Blue Rose"},
    {"Shoot_2nd_Long",   "Blue Rose"},
    {"CHARGE_Lv1_Shor",  "Charge Shot 1"},
    {"CHARGE_Lv1_Midd",  "Charge Shot 1"},
    {"CHARGE_Lv1_Long",  "Charge Shot 1"},
    {"CHARGE_Lv2_Shor",  "Charge Shot 2"},
    {"CHARGE_Lv2_Midd",  "Charge Shot 2"},
    {"CHARGE_Lv2_Long",  "Charge Shot 2"},
    {"CHARGE_Lv3_Shor",  "Charge Shot 3"},
    {"CHARGE_Lv3_Midd",  "Charge Shot 3"},
    {"CHARGE_Lv3_Long",  "Charge Shot 3"},
    {"BLU-Bomb_00",      "Charge Shot Bomb"},

    // dt
    {"D.T-Burst_00",     "DT Burst"},
    {"D.T-Burst_01",     "DT Burst"},     // weak hit
    {"D.T-AirBurst_00",  "Air DT Burst"},
    {"D.T-AirBurst_01",  "Air DT Burst"}, // weak hit
    {"D.T-Showdown_00",  "Showdown"},
    {"D.T-Showdown_01",  "Showdown"},
    {"D.T-Showdown_02",  "Showdown"},
    {"D.T-Showdown_03",  "Showdown"},
    {"D.T-Showdown_04",  "Showdown"},
    {"D.T-Showdown_05",  "Showdown"},
    {"D.T-Showdown_06",  "Showdown"},
    {"D.T-Maximum_00",   "Maximum Bet"},      // hit
    {"Drive",            "Maximum Bet"},      // projectile
    {"D.T-Maximum_01",   "Maximum Bet Lv.2"}, // hit
    {"Drive02",          "Maximum Bet Lv.2"}, // projectile
    {"Genei",            "Summoned Sword"},
    {"Genei02",          "Charged Summoned Sword"},
};

static const std::map<std::vector<std::string>, std::string> comboNames = {
    {{"High Time", "Aerial Rave", "Aerial Rave", "Aerial Rave", "Aerial Rave"}, "Very Creative!"},
    {{"High Time", "E&I", "E&I", "E&I", "E&I", "E&I", "E&I", "E&I", "E&I", "E&I", "E&I"}, "Kamiya's Vision!"},
    {{"Guardfly", "Guardfly"}, "Is That A Plane?"},
};

enum characterID {
    NERO  = 0,
    DANTE = 1,
};

struct ComboUnlock {
  characterID character;
  int difficultyLevel;
  const char* name;
  const char* hint;
  const char* how;
  bool unlocked;
};

static std::array<ComboUnlock, 3> unlocked_combos = {
    ComboUnlock {DANTE, 1, "Very Creative!", "Welcome to Dante!", "As Dante, Used High Time > Rave 1-4", false},
    ComboUnlock {DANTE, 1, "Kamiya's Vision!", "Did you know DMC1 was going to be RE4?", "As Dante, Used High Time > E+I Shot x 10", false},
    ComboUnlock {DANTE, 3, "Is That A Plane?", "I saw a combo video on YouTube once...", "As Dante, Used Guardfly > Guardfly", false},
};

struct TrickScore {
    std::string text;
    float score;
    float multiplier;
    char styleLetter;
    std::chrono::steady_clock::time_point timePerformed;
    bool isAlreadyRenamed;

    TrickScore() 
        : text(""), 
          score(0.0f), 
          multiplier(1.0f), 
          styleLetter(' '), 
          timePerformed(std::chrono::steady_clock::now()),
          isAlreadyRenamed(false) {}

    TrickScore(const std::string& t, float s, int pos, float mult, char letter, bool renamed = false) 
        : text(t), 
          score(s), 
          multiplier(mult), 
          styleLetter(letter),
          timePerformed(std::chrono::steady_clock::now()),
          isAlreadyRenamed(renamed) {}
};

struct GroupedTrick {
    TrickScore baseTrick;
    int repeatCount;
    float summedScore;

    GroupedTrick() : baseTrick(), repeatCount(0), summedScore(0.0f) {}

    GroupedTrick(const TrickScore& trick, int count, float totalScore)
        : baseTrick(trick), repeatCount(count), summedScore(totalScore) {}
};

struct StateSample {
    float time;
    float state;
};

// system
bool StylePoints::mod_enabled = false;
bool StylePoints::tonyHawk = false;
bool StylePoints::moreGrouping = false; 
bool StylePoints::originalNames = false;
bool StylePoints::showAirTimeDisplay = false;
bool StylePoints::showHeightChart = false;
bool StylePoints::showInertiaChart = false;

uintptr_t StylePoints::jmp_ret1 = NULL;
uintptr_t StylePoints::jmp_ret2 = NULL;
uintptr_t StylePoints::jmp_ret3 = NULL;
uintptr_t StylePoints::jmp_ret4 = NULL;

// non tony display
std::vector<TrickScore> trickScores;
static const float baseWidth = 1920.0f;
static float correctedWindowFontScale = 0.0f;
static const float displayDuration = 1.0f;
static const uint32_t maxScores = 5;

// tony display
static std::chrono::steady_clock::time_point lastTrickTime = std::chrono::steady_clock::now();
static const uint32_t maxTonyScores = UINT32_MAX;
static int maxPerRow = 7;
static int maxRows = 3;
static float comboScore = 0.0f;
static float timerBase = 1.0f;
static float timerComboInfluence = 0.005f;
static float shakeDuration = 0.99f;

const char* GetStyleChar(int styleNum) {
    switch (styleNum) {
        case 0: return "E";
        case 1: return "D";
        case 2: return "C";
        case 3: return "B";
        case 4: return "A";
        case 5: return "S";
        case 6: return "SS";
        case 7: return "SSS";
        default: return "";
    }
}

float GetStyleMultiplier(int styleNum) {
    switch (styleNum) {
        case 0: return 1.0f; // E
        case 1: return 1.5f; // D
        case 2: return 2.0f; // C
        case 3: return 2.5f; // B
        case 4: return 3.0f; // A
        case 5: return 4.0f; // S
        case 6: return 4.0f; // SS
        case 7: return 4.0f; // SSS
        default: return 0.0f;
    }
}

ImVec4 GetStyleColor(int styleNum) {
    switch (styleNum) {
    case 0: return { 242 / 255.0f, 242 / 255.0f, 242 / 255.0f, 0.0f }; // E, same as D
    case 1: return { 242 / 255.0f, 242 / 255.0f, 242 / 255.0f, 0.0f }; // D
    case 2: return {  53 / 255.0f, 203 / 255.0f, 244 / 255.0f, 0.0f }; // C
    case 3: return { 213 / 255.0f,  92 / 255.0f,  15 / 255.0f, 0.0f }; // B
    case 4: return { 245 / 255.0f,  68 / 255.0f,  46 / 255.0f, 0.0f }; // A
    case 5: return { 234 / 255.0f,  56 / 255.0f,  58 / 255.0f, 0.0f }; // S
    case 6: return { 234 / 255.0f,  56 / 255.0f,  58 / 255.0f, 0.0f }; // SS
    case 7: return { 234 / 255.0f,  56 / 255.0f,  58 / 255.0f, 0.0f }; // SSS
    default: return { 0.0f, 0.0f, 0.0f, 0.0f };
    }
}

static void UpdateTrickNames() {
    for (auto& score : trickScores) {
        if (!score.isAlreadyRenamed) {
            auto it = textLookupTable.find(score.text);
            if (it != textLookupTable.end()) {
                score.text = it->second;
            }
            score.isAlreadyRenamed = true;
        }
    }
}

// from white to gold
ImVec4 GetScoreColor(float score, float min, float max) {
    float normalizedScore = (score - min) / (max - min);
    normalizedScore = glm::clamp(normalizedScore, 0.0f, 1.0f);
    float red = 1.0f;
    float green = 1.0f - 0.157f * normalizedScore;
    float blue = 1.0f - 1.0f * normalizedScore;
    return ImVec4(red, green, blue, 1.0f);
}

ImVec4 GetTrickColor(float totalScore, float fade) {
    StylePoints::moreGrouping ? totalScore *= 0.05f : totalScore *= 0.1f;
    if (totalScore >= 50.0f) {
        return ImVec4(1.0f, 0.0f, 0.0f, fade); // Red for high scores
    } else if (totalScore >= 30.0f) {
        return ImVec4(1.0f, 0.5f, 0.0f, fade); // Orange for medium-high scores
    } else if (totalScore >= 20.0f) {
        return ImVec4(1.0f, 1.0f, 0.0f, fade); // Yellow for medium scores
    } else {
        return ImVec4(1.0f, 1.0f, 1.0f, fade); // White for low scores
    }
}

void SaveUnlockToConfig() {
    ModFramework* framework = g_framework.get();
    utility::Config cfg{};
    cfg.load(CONFIG_FILENAME);
    for (const auto& combo : unlocked_combos) {
        std::string configKey = "combo_unlock_" + std::string(combo.name);
        cfg.set<bool>(configKey, combo.unlocked);
    }
    cfg.save(CONFIG_FILENAME);
}

static void DrawTrickScores() {
    if (devil4_sdk::get_local_player()) {
        if (!StylePoints::originalNames) UpdateTrickNames();
        auto now = std::chrono::steady_clock::now();
        ImVec2 screenSize = devil4_sdk::get_sRender()->screenRes;
        ImGui::SetNextWindowPos(ImVec2(screenSize.x * 0.6f, screenSize.y * 0.4f));
        ImGui::SetNextWindowSize(ImVec2(screenSize.x * 0.3f, screenSize.y * 0.3f));
        ImGui::Begin("TrickScoresWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
        ImGui::SetWindowFontScale(correctedWindowFontScale * 2.0f);
        float fontSize = ImGui::GetFontSize();

        for (uint16_t i = 0; i < trickScores.size(); ++i) {
            TrickScore& score = trickScores[i];
            float elapsed = std::chrono::duration<float>(now - score.timePerformed).count();

            if (elapsed > displayDuration) {
                trickScores.erase(trickScores.begin() + i);
                --i;
                continue;
            }

            float fade = 1.0f - (elapsed / displayDuration);
            ImVec4 color(1.0f, 1.0f, 1.0f, fade);
            ImVec4 currentStyleColor = GetStyleColor(score.styleLetter);
            ImVec4 color2(currentStyleColor.x, currentStyleColor.y, currentStyleColor.z, fade);

            if (score.styleLetter != 0) {
                float windowWidth = ImGui::GetContentRegionAvail().x;
                char scoreBuffer[32];
                snprintf(scoreBuffer, sizeof(scoreBuffer), "%.1f", score.score * 0.1 * score.multiplier);

                float textWidth = ImGui::CalcTextSize(score.text.c_str()).x;
                float scoreWidth = ImGui::CalcTextSize(scoreBuffer).x;
                float customSpacing = fontSize * 1.0f;
                float rightAlignX = (windowWidth * 0.8f) - (customSpacing * 0.5f);
                float leftAlignX = (windowWidth * 0.8f) + (customSpacing * 0.5f);

                ImGui::SetCursorPosX(rightAlignX - textWidth);
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::Text(_("%s"), score.text.c_str());
                ImGui::PopStyleColor();
                ImGui::SameLine(leftAlignX);
                ImGui::PushStyleColor(ImGuiCol_Text, color2);
                ImGui::Text(_("%.1f"), score.score * 0.1 * score.multiplier);
                ImGui::PopStyleColor();
            }
        }
        ImGui::End();
    }
}

static void DrawTonyScores() {
    auto* player = devil4_sdk::get_local_player();
    if (!player) return;

    ImVec2 screenSize = devil4_sdk::get_sRender()->screenRes;
    if (!StylePoints::originalNames) UpdateTrickNames();
    auto now = std::chrono::steady_clock::now();

    // trick recognition
    float elapsedSinceLastTrick = std::chrono::duration<float>(now - lastTrickTime).count();
    float turboSpeed = devil4_sdk::get_sMediator()->turboEnabled ? devil4_sdk::get_work_rate()->turbo_speed : devil4_sdk::get_work_rate()->game_speed;

    // fade
    float fadeRate = 1.0f * turboSpeed; // Base decay rate
    float baseFadeTime = 1.0f / (timerBase * fadeRate); // Inversely proportional to timerBase
    float comboBonus = trickScores.empty() ? 0.0f : trickScores.back().score * timerComboInfluence;
    float fadeMaxMultiplier = baseFadeTime + comboBonus;
    float fade = std::max(0.0f, 1.0f - (elapsedSinceLastTrick / fadeMaxMultiplier));

    if (fade <= 0.0f) {
        trickScores.clear();
        comboScore = 0.0f;
    }

    ImGui::SetNextWindowPos(ImVec2(screenSize.x * 0.5f, screenSize.y * 0.9f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(screenSize.x * 1.0f, screenSize.y * 0.2f));
    ImGui::Begin("TrickScoresWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

    float shakeAmount = 0.0f;
    if (fade > shakeDuration) {
        shakeAmount = dis(gen);
    }

    float scoreFontBaseScale = correctedWindowFontScale * 2.0f;
    float trickFontBaseScale = correctedWindowFontScale * 1.5f;
    float scoreFontScale = scoreFontBaseScale + shakeAmount * 0.1f;
    float trickFontScale = trickFontBaseScale;
    ImGui::SetWindowFontScale(scoreFontScale);

    std::vector<TrickScore> latestTricks;
    if (trickScores.size() > maxTonyScores) {
        latestTricks = std::vector<TrickScore>(
            trickScores.end() - maxTonyScores,
            trickScores.end()
        );
    }
    else {
        latestTricks = trickScores;
    }

    std::vector<GroupedTrick> groupedTricks;

    if (StylePoints::moreGrouping) {
        std::vector<std::string> firstAppearanceOrder;
        std::map<std::string, GroupedTrick> trickMap;

        for (const auto& trick : latestTricks) {
            auto it = trickMap.find(trick.text);
            if (it == trickMap.end()) {
                trickMap[trick.text] = GroupedTrick(trick, 1, trick.score);
                firstAppearanceOrder.push_back(trick.text);
            }
            else {
                it->second.repeatCount++;
                it->second.summedScore += trick.score;
            }
        }

        groupedTricks.clear();
        for (const auto& name : firstAppearanceOrder) {
            groupedTricks.push_back(trickMap[name]);
        }
    }
    else {
        for (const auto& trick : latestTricks) {
            if (groupedTricks.empty() || groupedTricks.back().baseTrick.text != trick.text) {
                groupedTricks.emplace_back(trick, 1, trick.score);
            }
            else {
                auto& lastGroup = groupedTricks.back();
                lastGroup.repeatCount++;
                lastGroup.summedScore += trick.score;
            }
        }
    }

    std::vector<std::vector<GroupedTrick>> rowTricks(maxRows);
    comboScore = 0.0f;
    int currentRow = 0;

    for (const auto& groupedTrick : groupedTricks) {
        comboScore += groupedTrick.summedScore * 0.1f;

        if (currentRow >= maxRows) continue;

        if (rowTricks[currentRow].size() >= (uint32_t)maxPerRow) {
            currentRow++;
        }

        if (currentRow < maxRows) {
            rowTricks[currentRow].push_back(groupedTrick);
        }
    }

    int currentStyleTier = devil4_sdk::get_stylish_count()->current_style_tier;
    ImVec4 currentStyleColor = GetStyleColor(currentStyleTier);

    char scoreText[32];
    snprintf(scoreText, sizeof(scoreText), "%.1f", comboScore);

    char styleTierText[32];
    snprintf(styleTierText, sizeof(styleTierText), "x%.1f", GetStyleMultiplier(currentStyleTier));

    float totalWidth = ImGui::CalcTextSize(scoreText).x + ImGui::CalcTextSize(styleTierText).x + ImGui::CalcTextSize(" ").x;
    float scoreShakeAmount = shakeAmount * 1.0f;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f + scoreShakeAmount);
    ImGui::SetCursorPosY(0.0f + scoreShakeAmount);
    ImVec4 scoreColor = GetScoreColor(comboScore, 0.0f, 500.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(scoreColor.x, scoreColor.y, scoreColor.z, fade));
    ImGui::Text(_("%s"), scoreText);
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImVec4 multiplierColor(currentStyleColor.x, currentStyleColor.y, currentStyleColor.z, fade);
    ImGui::PushStyleColor(ImGuiCol_Text, multiplierColor);
    ImGui::Text(_("%s"), styleTierText);
    ImGui::PopStyleColor();

    ImGui::SetWindowFontScale(trickFontScale);

    float trickShakeAmount = shakeAmount * 5.0f;
    float rowStartY = ImGui::GetFontSize() * 1.5f;

    for (int row = 0; row < maxRows; ++row) {
        if (rowTricks[row].empty()) continue;

        std::vector<std::pair<std::string, ImVec4>> textSegments;
        float totalRowWidth = 0.0f;

        for (size_t i = 0; i < rowTricks[row].size(); ++i) {
            const auto& groupedTrick = rowTricks[row][i];
            char trickText[64];
            if (groupedTrick.repeatCount > 1) {
                snprintf(trickText, sizeof(trickText), "%s (x%d)",
                    groupedTrick.baseTrick.text.c_str(),
                    groupedTrick.repeatCount
                );
            }
            else {
                snprintf(trickText, sizeof(trickText), "%s",
                    groupedTrick.baseTrick.text.c_str()
                );
            }

            ImVec4 trickColor = GetTrickColor(groupedTrick.summedScore, fade);
            textSegments.emplace_back(trickText, trickColor);

            if (i < rowTricks[row].size() - 1) {
                textSegments.emplace_back(" + ", ImVec4(1.0f, 1.0f, 1.0f, fade));
            }
        }

        for (const auto& [text, color] : textSegments) {
            totalRowWidth += ImGui::CalcTextSize(text.c_str()).x;
        }

        float posY = rowStartY + row * ImGui::GetFontSize() * 1.0f + trickShakeAmount;
        float cursorX = (ImGui::GetContentRegionAvail().x - totalRowWidth) * 0.5f + trickShakeAmount;

        for (const auto& [text, color] : textSegments) {
            ImGui::SetCursorPos(ImVec2(cursorX, posY));
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(text.c_str());
            ImGui::PopStyleColor();
            cursorX += ImGui::CalcTextSize(text.c_str()).x;
        }
    }

    ImGui::End();

    // Combo recognition
    static std::string lastProcessedTrick = "";
    static std::string detectedCombo = "";
    static std::chrono::steady_clock::time_point lastMatchTime = std::chrono::steady_clock::now();
    float elapsedSinceLastMatch = std::chrono::duration<float>(now - lastMatchTime).count();
    float comboRecognitionAlpha = 1.0f - (elapsedSinceLastMatch * turboSpeed);

    if (!trickScores.empty()) {
        const std::string& latestTrick = trickScores.back().text;
        if (lastProcessedTrick != latestTrick) {
            lastProcessedTrick = latestTrick;
        }
    }

    if (!trickScores.empty()) {
        for (const auto& combo : comboNames) {
            const std::vector<std::string>& comboSequence = combo.first;
            size_t comboLength = comboSequence.size();
            size_t trickScoresSize = trickScores.size();
            if (trickScoresSize >= comboLength) {
                bool match = true;
                for (size_t i = 0; i < comboLength; ++i) {
                    if (trickScores[trickScoresSize - comboLength + i].text != comboSequence[i]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    const std::string& comboName = combo.second;
                    bool newUnlock = false;
                    for (auto& unlockCombo : unlocked_combos) {
                        if (comboName == unlockCombo.name && !unlockCombo.unlocked) {
                            unlockCombo.unlocked = true;
                            newUnlock = true;
                            break;
                        }
                    }
                
                    if (newUnlock) {
                        SaveUnlockToConfig();
                    }
                
                    detectedCombo = combo.second;
                    lastMatchTime = std::chrono::steady_clock::now();
                    break;
                }
            }
        }
    }

    ImGui::SetNextWindowPos(ImVec2(screenSize.x * 0.2f, screenSize.y * 0.7f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(screenSize.x * 0.5f, screenSize.y * 0.5f));
    ImGui::Begin("ScoreRecognitionWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    ImGui::SetWindowFontScale(correctedWindowFontScale * 1.5f);
    if (comboRecognitionAlpha > 0.0f) {
        float textWidth = ImGui::CalcTextSize(detectedCombo.c_str()).x;
        float windowWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImVec4 comboColor(1.0f, 1.0f, 0.0f, comboRecognitionAlpha);
        ImGui::PushStyleColor(ImGuiCol_Text, comboColor);
        ImGui::Text(_("%s"), detectedCombo.c_str());
        ImGui::PopStyleColor();
    }
    ImGui::End();

    // airtime window
    if (StylePoints::showAirTimeDisplay) {
        static float airTimer = 0.0f;
        static float airTimerAlpha = 0.0f;
        static float displayedAirTime = 0.0f;
        static bool wasInAir = false;
        static float timeOnGround = 0.0f;
        static const float displayStartAirTime = 3.0f;
        static const float fadeDuration = 1.0f;
        static bool wasGrounded = true;
        static bool fadingUp = false;
        static float fadeUpTimer = 0.0f;
        static float alphaWhenLanded = 0.0f;

        float deltaTime = player->m_delta_time;
        float realSeconds = deltaTime / 60.0f;
        bool isInAir = false;
        if (player->characterSettingsOne)
            isInAir = player->characterSettingsOne->groundedActual;
        bool justBecameAirborne = wasGrounded && isInAir;
        wasGrounded = !isInAir;

        if (isInAir) {
            if (!wasInAir) {
                airTimer = 0.0f;
                wasInAir = true;
                if (airTimerAlpha > 0.0f) {
                    fadingUp = true;
                    fadeUpTimer = 0.0f;
                }
            }
            else if (justBecameAirborne && airTimerAlpha > 0.0f) {
                fadingUp = true;
                fadeUpTimer = 0.0f;
            }
            if (sArea* sArea = devil4_sdk::get_sArea()) {
                if (!sArea->aGamePtr->m_paused) {
                    airTimer += realSeconds;
                }
            }
            if (fadingUp) {
                fadeUpTimer += realSeconds;
                float fadeProgress = fadeUpTimer / fadeDuration;
                if (fadeProgress >= 1.0f) {
                    airTimerAlpha = 1.0f;
                    fadingUp = false;
                }
                else {
                    airTimerAlpha = std::min(1.0f, airTimerAlpha + fadeProgress * (1.0f - airTimerAlpha));
                }
            }
            else if (airTimer >= displayStartAirTime && airTimerAlpha < 1.0f) {
                float fadeInProgress = (airTimer - displayStartAirTime) / fadeDuration;
                airTimerAlpha = std::max(airTimerAlpha, std::min(1.0f, fadeInProgress));
            }
        }
        else {
            if (wasInAir) {
                displayedAirTime = airTimer;
                timeOnGround = 0.0f;
                wasInAir = false;
                fadingUp = false;
                alphaWhenLanded = airTimerAlpha;
            }

            if (airTimerAlpha > 0.0f) {
                timeOnGround += realSeconds;
                if (timeOnGround > 1.0f) {
                    float fadeOutProgress = std::min(1.0f, (timeOnGround - 1.0f) / fadeDuration);
                    airTimerAlpha = std::max(0.0f, alphaWhenLanded * (1.0f - fadeOutProgress));
                }
            }
        }

        if (airTimerAlpha > 0.0f) {
            ImGui::SetNextWindowPos(ImVec2(screenSize.x * 0.8f, screenSize.y * 0.7f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
            ImGui::Begin("AirtimeWindow", nullptr, ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
            ImGui::SetWindowFontScale(correctedWindowFontScale * 1.5f);
            ImVec4 airtimeColor(1.0f, 1.0f, 1.0f, airTimerAlpha);
            ImGui::PushStyleColor(ImGuiCol_Text, airtimeColor);
            float timeToDisplay = isInAir ? airTimer : displayedAirTime;
            ImGui::Text(_("Airtime: %.1f"), timeToDisplay);
            ImGui::PopStyleColor();
            ImGui::End();
        }
    }

    // Height & Inertia Chart
    if (StylePoints::showHeightChart || StylePoints::showInertiaChart) {
        static std::vector<std::tuple<float, float, float>> heightInertiaTrackerStateHistory;
        static std::chrono::steady_clock::time_point heightInertiaTrackerStartTime;
        static bool heightInertiaTrackerTimerActive = false;

        if (!trickScores.empty()) {
            if (!heightInertiaTrackerTimerActive) {
                heightInertiaTrackerStartTime = std::chrono::steady_clock::now();
                heightInertiaTrackerStateHistory.clear();
                heightInertiaTrackerTimerActive = true;
            }

            auto now = std::chrono::steady_clock::now();
            float realElapsed = std::chrono::duration<float>(now - heightInertiaTrackerStartTime).count();
            float gameElapsed = realElapsed / turboSpeed;
            heightInertiaTrackerStateHistory.push_back({ gameElapsed, player->m_pos.y, player->inertia });
            ImVec2 chartSize = ImVec2(screenSize.x * 0.1f, screenSize.y * 0.1f);
            ImGui::SetNextWindowPos(ImVec2(screenSize.x * 0.01f, screenSize.y * 0.5f), ImGuiCond_Always, ImVec2(0.0f, 0.5f));
            ImGui::Begin("Height & Inertia Chart", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration);
            static std::vector<float> times, heights, inertias;
            times.clear();
            heights.clear();
            inertias.clear();
            for (const auto& sample : heightInertiaTrackerStateHistory) {
                times.push_back(std::get<0>(sample));
                heights.push_back(std::get<1>(sample));
                inertias.push_back(std::get<2>(sample));
            }

            float minTime = times.front();
            float maxTime = times.back();
            float minHeight = *std::min_element(heights.begin(), heights.end());
            float maxHeight = *std::max_element(heights.begin(), heights.end());
            float minInertia = *std::min_element(inertias.begin(), inertias.end());
            float maxInertia = *std::max_element(inertias.begin(), inertias.end());
            float overallMin = std::min(minHeight, minInertia);
            float overallMax = std::max(maxHeight, maxInertia);

            ImGui::Text(_("Combo: %.2f"), maxTime - minTime);
            if (StylePoints::showHeightChart) {
                ImGui::PlotLines("## Height", heights.data(), heights.size(), 0, "Height", minHeight, maxHeight, chartSize);
                ImGui::Text(_("Height: %.2f"), heights.back());
            }
            if (StylePoints::showInertiaChart) {
                ImGui::PlotLines("## Inertia", inertias.data(), inertias.size(), 0, "Inertia", minInertia, maxInertia, chartSize);
                ImGui::Text(_("Inertia: %.2f"), inertias.back());
            }
            ImGui::End();
        }
        else {
            heightInertiaTrackerTimerActive = false;
        }
    }
}

static void AddTrickScore(const char* text, float score, float multiplier, int styleLetter) {
    TrickScore newScore;
    newScore.text = text;
    newScore.score = score;
    newScore.multiplier = multiplier;
    newScore.styleLetter = styleLetter;
    newScore.timePerformed = std::chrono::steady_clock::now();
    newScore.isAlreadyRenamed = false;

    if (StylePoints::tonyHawk) {
        lastTrickTime = newScore.timePerformed;
        if (trickScores.size() >= maxTonyScores) {
            trickScores.erase(trickScores.begin());
        }
        trickScores.push_back(newScore);
    } else {
        if (trickScores.size() >= maxScores) {
            trickScores.pop_back();
        }
        trickScores.insert(trickScores.begin(), newScore);
    }
}

static float xmm0backup = 0.0f;
static float xmm1backup = 0.0f;
naked void detour1(void) {
    _asm {
        cmp byte ptr [StylePoints::mod_enabled], 1
        jne originalcode

        movss [xmm0backup], xmm0
        movss [xmm1backup], xmm1

        push eax // 0x4
        push ebx // 0x8
        push ecx // 0xC
        push edx // 0x10
        push esi // 0x14
        push edi // 0x18

        // backup pushes, 2 args
        push [esp+0x18+0xC] // style letter
        sub esp, 0x4
        movss [esp], xmm2 // multiplier
        push [esp+0x18+0x8+0x4] // score
        push ebx // text
        call AddTrickScore // fucks eax, ecx, edx
        add esp,0x10 // 4 args

        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        pop eax

        movss xmm0, [xmm0backup]
        movss xmm1, [xmm1backup]

        originalcode:
        comiss xmm0,xmm1
        movss [edx+0x04], xmm0
        jmp dword ptr [StylePoints::jmp_ret1]
    }
}
static const char* guardText = "Guardfly";
static const float slowestGuardfly = 20.0f;
naked void detour2(void) {
    _asm {
        cmp byte ptr [StylePoints::mod_enabled], 1
        jne originalcode

        push eax
        push ebx
        push ecx
        push edx
        push esi
        push edi

        movss xmm0,[ebp+0x1e1c]
        comiss xmm0, [slowestGuardfly]
        jb popcode
        push 0 // style letter
        push 0x3f800000 // multiplier
        push 0 // score
        push guardText // name
        call AddTrickScore // fucks eax, ecx, edx
        add esp,0x10 // 4 args

        popcode:
        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        pop eax

        originalcode:
        add [ebp+0x00001504],ebx
        jmp dword ptr [StylePoints::jmp_ret2]
    }
}

static const char* snatchText = "Snatch";
naked void detour3(void) {
    _asm {
        cmp byte ptr [StylePoints::mod_enabled], 1
        jne originalcode

        push eax
        push ebx
        push ecx
        push edx
        push esi
        push edi

        push 0 // style letter
        push 0x3f800000 // multiplier
        push 0 // score
        push snatchText // name
        call AddTrickScore // fucks eax, ecx, edx
        add esp,0x10 // 4 args

        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        pop eax

        originalcode:
        mov dword ptr [ebp+0x00001504],00000002
        jmp dword ptr [StylePoints::jmp_ret3]
    }
}

naked void detour4(void) {
    _asm {
        cmp byte ptr [StylePoints::mod_enabled], 1
        jne originalcode

        push eax
        push ebx
        push ecx
        push edx
        push esi
        push edi

        push 0 // style letter
        push 0x3f800000 // multiplier
        push 0 // score
        push snatchText // name
        call AddTrickScore // fucks eax, ecx, edx
        add esp,0x10 // 4 args

        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        pop eax

        originalcode:
        mov dword ptr [ebp+0x00001504],00000002
        jmp dword ptr [StylePoints::jmp_ret4]
    }
}

std::optional<std::string> StylePoints::on_initialize() {
    if (!install_hook_offset(0x5480F, hook1, &detour1, &jmp_ret1, 5)) {
		spdlog::error("Failed to init StylePoints mod\n");
		return "Failed to init StylePoints mod";
	}

    if (!install_hook_offset(0x3CC13B, hook2, &detour2, &jmp_ret2, 6)) { // called once on air guard start
		spdlog::error("Failed to init StylePoints mod 2\n");
		return "Failed to init StylePoints mod 2";
	}

    // would be nice to swap these out with "snatch was successful" call so you can't use it outside of combat
    if (!install_hook_offset(0x3FA0C9, hook3, &detour3, &jmp_ret3, 10)) { // called once on snatch ground
		spdlog::error("Failed to init StylePoints mod 3\n");
		return "Failed to init StylePoints mod 3";
	}
    if (!install_hook_offset(0x3FA319, hook4, &detour4, &jmp_ret4, 10)) { // called once on snatch air
		spdlog::error("Failed to init StylePoints mod 4\n");
		return "Failed to init StylePoints mod 4";
	}

    after_reset();
    return Mod::on_initialize();
}

std::string CensorText(const std::string& text) {
    return std::string(text.length(), '*');
}
   
void StylePoints::DrawHiddenCombos() {
    if (!devil4_sdk::get_local_player()) return;
    sArea* sArea = devil4_sdk::get_sArea();
    if (sArea == nullptr || sArea->aGamePtr == nullptr || sArea->aGamePtr->m_paused == false)
        return;
    if (sArea->currentRoomPtr == nullptr || sArea->currentRoomPtr->pauseMenuPtr1 == nullptr || sArea->currentRoomPtr->pauseMenuPtr1->draw != 1)
        return;

    ImVec2 screenSize = devil4_sdk::get_sRender()->screenRes;
    static ImVec2 windowPosModifier{ 0.475f, 0.20f };
    ImVec2 windowPos = ImVec2(screenSize.x * windowPosModifier.x, screenSize.y * windowPosModifier.y);
    static ImVec2 windowSizeModifier{ 0.35f, 0.25f };
    ImVec2 windowSize = ImVec2(screenSize.x * windowSizeModifier.x, screenSize.y * windowSizeModifier.y);
    static ImVec2 windowPaddingModifier{ 0.1f, 0.2f };
    ImVec2 windowPadding = ImVec2(windowSize.x * windowPaddingModifier.x, windowSize.y * windowPaddingModifier.y);
    if (m_texture_handle != nullptr) {
        auto* bgDrawList = ImGui::GetBackgroundDrawList();
        bgDrawList->AddImage(m_texture_handle, windowPos, ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y),
            ImVec2(0, 0), ImVec2(1.0, 1.0), ImColor::ImColor(bg_red_int, bg_green_int, bg_blue_int, bg_alpha_int));
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, windowPadding);
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);
    ImGui::Begin("Hidden Combos Panel", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
    ImVec2 availSpace = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("ScrollingContent", availSpace, false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
    static float fontScale = 1.0f;
    ImGui::SetWindowFontScale(fontScale);
    static bool splitIntoDifficulties = false;

    static ImVec4 TexCol{ 0.8f, 0.8f, 0.8f, 1.0f };
    ImGui::TextColored(TexCol, "Hidden Combos");

    ImGui::Columns(2, "CharacterColumns", true);

    ImVec4 danteColor = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
    ImVec4 neroColor = ImVec4(0.4f, 0.6f, 1.0f, 1.0f);

    int totalDante = 0;
    int unlockedDante = 0;
    for (const ComboUnlock& combo : unlocked_combos) {
        if (combo.character == DANTE) {
            totalDante++;
            if (combo.unlocked) unlockedDante++;
        }
    }
    
    int totalNero = 0;
    int unlockedNero = 0;
    for (const ComboUnlock& combo : unlocked_combos) {
        if (combo.character == NERO) {
            totalNero++;
            if (combo.unlocked) unlockedNero++;
        }
    }
    
    ImGui::TextColored(TexCol, "Dante - %i/%i Unlocked", unlockedDante, totalDante);
    ImGui::NextColumn();

    ImGui::TextColored(TexCol, "Nero - %i/%i Unlocked", unlockedNero, totalNero);
    ImGui::NextColumn();

    if (splitIntoDifficulties) {
        int maxDifficulty = 0;
        for (ComboUnlock& combo : unlocked_combos) {
            if (combo.difficultyLevel > maxDifficulty)
                maxDifficulty = combo.difficultyLevel;
        }
    
        for (int difficulty = 1; difficulty <= maxDifficulty; ++difficulty) {
            ImVec4 difficultyColor;
            if (difficulty == 1) {
                difficultyColor = ImVec4(0.5f, 1.0f, 0.5f, 1.0f); // Green
            } else if (difficulty == 2) {
                difficultyColor = ImVec4(1.0f, 1.0f, 0.5f, 1.0f); // Yellow
            } else if (difficulty == 3) {
                difficultyColor = ImVec4(1.0f, 0.6f, 0.2f, 1.0f); // Orange
            } else if (difficulty == 4) {
                difficultyColor = ImVec4(1.0f, 0.5f, 0.5f, 1.0f); // Red
            } else if (difficulty == 5) {
                difficultyColor = ImVec4(0.8f, 0.5f, 1.0f, 1.0f); // Purple
            }

            ImGui::Separator();
            int totalDanteDiff = 0;
            int unlockedDanteDiff = 0;
            for (const ComboUnlock& combo : unlocked_combos) {
                if (combo.difficultyLevel == difficulty && combo.character == DANTE) {
                    totalDanteDiff++;
                    if (combo.unlocked) unlockedDanteDiff++;
                }
            }
        
            ImGui::TextColored(TexCol, "Difficulty %i - %i/%i Unlocked", difficulty, unlockedDanteDiff, totalDanteDiff); // difficultyColor
        
            for (size_t i = 0; i < unlocked_combos.size(); ++i) {
                ComboUnlock& combo = unlocked_combos[i];
                if (combo.difficultyLevel != difficulty || combo.character != DANTE)
                    continue;

                std::string displayText;
                if (combo.unlocked) {
                    displayText = combo.name;
                } else {
                    for (const char* p = combo.name; *p != '\0'; ++p) {
                        char c = *p;
                        if (c == ' ') {
                            displayText += c;
                        } else {
                            displayText += '?';
                        }
                    }
                }

                ImGui::TextColored(TexCol, "%s", displayText.c_str());

                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                    ImGui::TextUnformatted(combo.hint);
                    if (combo.unlocked) {
                        ImGui::TextColored(TexCol, "(%s)", combo.how);
                    }
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
            }
        
            ImGui::NextColumn();
        
            int totalNeroDiff = 0;
            int unlockedNeroDiff = 0;
            for (const ComboUnlock& combo : unlocked_combos) {
                if (combo.difficultyLevel == difficulty && combo.character == NERO) {
                    totalNeroDiff++;
                    if (combo.unlocked) unlockedNeroDiff++;
                }
            }
        
            ImGui::TextColored(TexCol, "Difficulty %i - %i/%i Unlocked", difficulty, unlockedNeroDiff, totalNeroDiff); // difficultyColor
        
            for (size_t i = 0; i < unlocked_combos.size(); ++i) {
                ComboUnlock& combo = unlocked_combos[i];
                if (combo.difficultyLevel != difficulty || combo.character != NERO)
                    continue;

                std::string displayText;
                if (combo.unlocked) {
                    displayText = combo.name;
                } else {
                    for (const char* p = combo.name; *p != '\0'; ++p) {
                        char c = *p;
                        if (c == ' ') {
                            displayText += c;
                        } else {
                            displayText += '?';
                        }
                    }
                }

                ImGui::TextColored(TexCol, "%s", displayText.c_str());

                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                    ImGui::TextUnformatted(combo.hint);
                    if (combo.unlocked) {
                        ImGui::TextColored(TexCol, "(%s)", combo.how);
                    }
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
            }
        
            ImGui::NextColumn();
        }
    } else {
        ImGui::Separator();
    
        for (size_t i = 0; i < unlocked_combos.size(); ++i) {
            ComboUnlock& combo = unlocked_combos[i];
            if (combo.character != DANTE)
                continue;

            std::string displayText;
            if (combo.unlocked) {
                displayText = combo.name;
            } else {
                for (const char* p = combo.name; *p != '\0'; ++p) {
                    char c = *p;
                    if (c == ' ') {
                        displayText += c;
                    } else {
                        displayText += '?';
                    }
                }
            }

            ImGui::TextColored(TexCol, "%s", displayText.c_str());

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted(combo.hint);
                if (combo.unlocked) {
                    ImGui::TextColored(TexCol, "(%s)", combo.how);
                }
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }
    
        ImGui::NextColumn();
    
        for (size_t i = 0; i < unlocked_combos.size(); ++i) {
            ComboUnlock& combo = unlocked_combos[i];
            if (combo.character != NERO)
                continue;

            std::string displayText;
            if (combo.unlocked) {
                displayText = combo.name;
            } else {
                for (const char* p = combo.name; *p != '\0'; ++p) {
                    char c = *p;
                    if (c == ' ') {
                        displayText += c;
                    } else {
                        displayText += '?';
                    }
                }
            }

            ImGui::TextColored(TexCol, "%s", displayText.c_str());

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted(combo.hint);
                if (combo.unlocked) {
                    ImGui::TextColored(TexCol, "(%s)", combo.how);
                }
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }
    
        ImGui::NextColumn();
    }

    ImGui::Columns(1);
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::SliderInt(_("bg_red_int"),   &bg_red_int, 0, 255, "Red: %d");
    ImGui::SliderInt(_("bg_green_int"), &bg_green_int, 0, 255, "Green: %d");
    ImGui::SliderInt(_("bg_blue_int"),  &bg_blue_int, 0, 255, "Blue: %d");
    ImGui::SliderInt(_("bg_alpha_int"), &bg_alpha_int, 0, 255, "Alpha: %d");
    ImGui::InputFloat(_("windowPos.x"), &windowPosModifier.x, 0.0f, 1.0f, "%.2f");
    ImGui::InputFloat(_("windowPos.y"), &windowPosModifier.y, 0.0f, 1.0f, "%.2f");
    ImGui::InputFloat(_("windowSize.x"), &windowSizeModifier.x, 0.0f, 1.0f, "%.2f");
    ImGui::InputFloat(_("windowSize.y"), &windowSizeModifier.y, 0.0f, 1.0f, "%.2f");
    ImGui::InputFloat(_("windowPadding.x"), &windowPaddingModifier.x, 0.0f, 1.0f, "%.2f");
    ImGui::InputFloat(_("windowPadding.y"), &windowPaddingModifier.y, 0.0f, 1.0f, "%.2f");
    ImGui::InputFloat(_("Text r"), &TexCol.x);
    ImGui::InputFloat(_("Text g"), &TexCol.y);
    ImGui::InputFloat(_("Text b"), &TexCol.z);
    ImGui::InputFloat(_("Text a"), &TexCol.w);
    ImGui::InputFloat(_("fontScale"), &fontScale);
    ImGui::Checkbox(_("Split Into Difficulties"), &splitIntoDifficulties);
    ImGui::SameLine();
    const char* clear_unlocks_label = _("Clear Unlocks");
    ImVec2 btn_size = ImGui::CalcTextSize(clear_unlocks_label);
    btn_size.x += ImGui::GetStyle().FramePadding.x * 2.0f;
    btn_size.y += ImGui::GetStyle().FramePadding.y * 2.0f;
    float rightEdge = ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x;
    ImGui::SetCursorPosX(rightEdge - ImGui::GetWindowPos().x - btn_size.x);
    if (ImGui::Button(clear_unlocks_label, btn_size)) {
        for (ComboUnlock& combo : unlocked_combos) {
            combo.unlocked = false;
        }
        SaveUnlockToConfig();
    }
    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleVar();
}

void StylePoints::on_gui_frame() {
    ImGui::BeginGroup();
    ImGui::Checkbox(_("Style Point Display"), &mod_enabled);
    if (mod_enabled) {
        ImGui::Indent(lineIndent);
        ImGui::Checkbox(_("Use Original Names"), &originalNames);
        ImGui::SameLine();
        help_marker(_("Instead of using our skill renames, use the developers'. Because there are more unique names, less skills will be grouped"));
        ImGui::Checkbox(_("Tony"), &tonyHawk);
        ImGui::SameLine();
        help_marker(_("Tony"));
        if (tonyHawk) {
            ImGui::Indent(lineIndent);
            ImGui::Checkbox(_("Air Time Display"), &showAirTimeDisplay);
            ImGui::Checkbox(_("Alternate Grouping"), &moreGrouping);
            ImGui::SameLine();
            help_marker(_("Group attacks by the order you originally did them\n"
                "This helps show the variety in a combo over exact input order"));
            // ImGui::Checkbox("Height Display", &showHeightChart);
            // ImGui::Checkbox("Inertia Display", &showInertiaChart);
            // ImGui::SliderFloat("timerBase", &timerBase, 0.0f, 2.0f, "%.1f");
            // ImGui::SameLine();
            // help_marker("DEV PLS REMOVE - The base time for how long you have before breaking a combo\n1.0 default");
            // ImGui::SliderFloat("comboInfluence", &timerComboInfluence, 0.0f, 0.1f, "%.4f");
            // ImGui::SameLine();
            // help_marker("DEV PLS REMOVE - How much influence will the score from your last attack have on the timer\n0.0010 default");
            // ImGui::SliderFloat("shakeDuration", &shakeDuration, 0.0f, 1.0f, "%.4f");
            // ImGui::SameLine();
            // help_marker("DEV PLS REMOVE - At what point in the timer should text shake stop\nRemember a faster timer means less shake, so edit this last\n0.99 default");
            // ImGui::PushItemWidth(sameLineItemWidth);
            // ImGui::SliderInt(_("maxRows"), &maxRows, 1, 10);
            // ImGui::SameLine();
            // help_marker("How many rows\n5 default");
            // ImGui::SliderInt(_("maxPerRow"), &maxPerRow, 1, 10);
            // ImGui::SameLine();
            // help_marker("How many attacks per row\n7 default");
            // ImGui::PopItemWidth();
            ImGui::Unindent(lineIndent);
        }
        ImGui::Unindent(lineIndent);
    }
    ImGui::EndGroup();
}

void StylePoints::on_frame(fmilliseconds& dt) {
    if (mod_enabled) {
        ImVec2 screenSize = devil4_sdk::get_sRender()->screenRes;
        correctedWindowFontScale = (screenSize.x / baseWidth);
        if (tonyHawk) {
            DrawTonyScores();
            DrawHiddenCombos();
        }
        else {
            DrawTrickScores();
        }
    }
}

void StylePoints::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("style_points_display").value_or(true);
    tonyHawk = cfg.get<bool>("hawk_points_display").value_or(true);
    moreGrouping = cfg.get<bool>("group_points_display").value_or(false);
    showAirTimeDisplay = cfg.get<bool>("airtime_points_display").value_or(true);
    showHeightChart = cfg.get<bool>("height_points_display").value_or(false);
    showInertiaChart = cfg.get<bool>("inertia_points_display").value_or(false);

    maxPerRow = cfg.get<int>("maxPerRow_points_display").value_or(7);
    maxRows = cfg.get<int>("maxRows_points_display").value_or(3);

    for (auto& combo : unlocked_combos) {
        std::string configKey = "combo_unlock_" + std::string(combo.name);
        combo.unlocked = cfg.get<bool>(configKey).value_or(false);
    }
}

void StylePoints::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("style_points_display", mod_enabled);
    cfg.set<bool>("hawk_points_display", tonyHawk);
    cfg.set<bool>("group_points_display", moreGrouping);
    cfg.set<bool>("airtime_points_display", showAirTimeDisplay);
    cfg.set<bool>("height_points_display", showHeightChart);
    cfg.set<bool>("inertia_points_display", showInertiaChart);

    cfg.set<int>("maxPerRow_points_display", maxPerRow);
    cfg.set<int>("maxRows_points_display", maxRows);

    // done automatically now
    /*for (size_t i = 0; i < unlocked_combos.size(); ++i) {
        std::string configKey = "combo_unlock_" + std::to_string(i);
        cfg.set<bool>(configKey, unlocked_combos[i].unlocked);
    }*/
}
