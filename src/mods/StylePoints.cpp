#include "StylePoints.hpp"
#include <Config.hpp>
#include "../sdk/Devil4.hpp"
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
    {"TS-ehsuc",         __("Mustang")},
    // royal guard
    {"RELEASE_A",        __("Release")}, // empty bar
    {"RELEASE_B",        __("Release")}, // any bar or meh attack timing
    {"RELEASE_C",        __("Release")}, // 10k or above
    {"RELEASE_D",        __("Release")}, // releasing an attack at 0-10k gauge
    {"RELEASE_E",        __("Release")}, // releasing an attack at 10k or above gauge

    // rebellion
    {"REBE-combo303",    __("Rebellion Combo A")}, // A1
    {"REBE-combo304",    __("Rebellion Combo A")}, // A2
    {"REBE-combo305",    __("Rebellion Combo A")}, // A3
    {"REBE-stings",      __("Stinger")},
    {"REBE-sting",       __("Stinger Hit")},
    {"REBE-stab",        __("Million Stab")},
	{"REBE-roundt",      __("Round Trip")},
    {"REBE-hightime",    __("High Time")},
    {"REBE-combo306-1",  __("Rebellion Combo B")},
    {"REBE-combo306-2",  __("Rebellion Combo B")},
    {"REBE-combo306-3",  __("Rebellion Combo B")},
    {"REBE-stabf",       __("Million Stab End")},
    {"REBE-helmbrkLV1",  __("Helm Breaker Lv.1")},
    {"REBE-helmbrkLV2",  __("Helm Breaker Lv.2")},
    {"REBE-helmbrkLV3",  __("Helm Breaker Lv.3")},
    {"REBE-helmbrkf",    __("Helm Breaker")},
    {"REBE-DTstg1",      __("DT Stinger")},
    {"REBE-DTstg2",      __("DT Stinger")},
    {"REBE-DTstg3",      __("DT Stinger")},
    {"REBE-DTstg4",      __("DT Stinger")},
    {"REBE-DTstg5",      __("DT Stinger")},

    // rebellion swordmaster
    {"REBE-props1",      __("Prop")},
    {"REBE-props20",     __("Shredder")},
    {"REBE-props21",     __("Shredder")},
    {"REBE-dm363",       __("Dance Macabre")}, // 1
    {"REBE-dm365",       __("Dance Macabre")}, // 2
    {"REBE-dm366",       __("Dance Macabre")}, // 3
    {"REBE-dm367",       __("Dance Macabre")}, // 4
    {"REBE-dm368-1",     __("Dance Macabre")}, // 5-1
    {"REBE-dm368-2",     __("Dance Macabre")}, // 5-2
    {"REBE-dm368-3",     __("Dance Macabre")}, // 5-3
    {"REBE-dm369",       __("Dance Macabre")}, // stab
    {"REBE-dm370",       __("Dance Macabre")}, // 7
    {"REBE-dm371",       __("Dance Macabre")}, // 8
    {"REBE-dm372-1",     __("Dance Macabre")}, // 9
    {"REBE-dm373",       __("Dance Macabre")}, // Million stab end
    {"REBE-dm364",       __("Dance Macabre")}, // End
    {"REBE-arial353",    __("Aerial Rave")}, // 1
    {"REBE-arial354",    __("Aerial Rave")}, // 2
    {"REBE-arial355",    __("Aerial Rave")}, // 3
    {"REBE-arial356",    __("Aerial Rave")}, // 4
	{"REBE-drivesw1",	 __("Drive")},
	{"REBE-drive1",		 __("Drive")},
	{"REBE-drive1-2",	 __("Drive Lv.2")},
	{"REBE-drive1-3",	 __("Drive Lv.3")},
	{"REBE-drivesw2",	 __("Overdrive")},
	{"REBE-drive2",		 __("Overdrive")},
	{"REBE-drive2-2",	 __("Overdrive Lv.2")},
	{"REBE-drive2-3",	 __("Overdrive Lv.3")},
	{"REBE-drivesw3",	 __("Drive")},
	{"REBE-drive3",		 __("Overdrive")},
	{"REBE-drive3-2",	 __("Overdrive Lv.2")},
	{"REBE-drive3-3",	 __("Overdrive Lv.3")},
	{"REBE-qdrive",		 __("Quick Drive")},

    // gilgamesh
	{"GIL-rblaze",       __("Flush")},
	{"GIL-rblaze-f",     __("Flush")},
	{"GIL-rblaze-b",     __("Flush")},
    {"GIL-klbe",         __("Full House")},
	{"GIL-combo403",     __("Gilgamesh Combo A")}, // A1
    {"GIL-combo464",     __("Gilgamesh Combo A Lv.2")}, // A1 Lv.2
    {"GIL-combo465",     __("Gilgamesh Combo A Lv.3")}, // A1 Lv.3
	{"GIL-combo404",     __("Gilgamesh Combo A")}, // A2
    {"GIL-combo466",     __("Gilgamesh Combo A Lv.2")}, // A2 Lv.2
    {"GIL-combo467",     __("Gilgamesh Combo A Lv.3")}, // A2 Lv.3
	{"GIL-combo405",     __("Gilgamesh Combo A")}, // A3
    {"GIL-combo476",     __("Gilgamesh Combo A Lv.2")}, // A3 Lv.2
    {"GIL-combo477",     __("Gilgamesh Combo A Lv.3")}, // A3 Lv.3
	{"GIL-combo406",     __("Gilgamesh Combo A")}, // A4
    {"GIL-combo478",     __("Gilgamesh Combo A Lv.2")}, // A4 Lv.2
    {"GIL-combo479",     __("Gilgamesh Combo A Lv.3")}, // A4 Lv.3
	{"GIL-combo407",     __("Gilgamesh Combo B")}, // B1 (kick spam)
    {"GIL-combo480",     __("Gilgamesh Combo B Lv.2")}, // B1 Lv.2
    {"GIL-combo481",     __("Gilgamesh Combo B Lv.3")}, // B1 Lv.3
	{"GIL-combo409-1",   __("Gilgamesh Combo B")}, // B2-1
	{"GIL-combo409-2",   __("Gilgamesh Combo B")}, // B2-2
    {"GIL-combo482-1",   __("Gilgamesh Combo B Lv.2")}, // B2-1 Lv.2
    {"GIL-combo482-2",   __("Gilgamesh Combo B Lv.2")}, // B2-2 Lv.2
    {"GIL-combo483-1",   __("Gilgamesh Combo B Lv.3")}, // B2-1 Lv.3
    {"GIL-combo483-2",   __("Gilgamesh Combo B Lv.3")}, // B2-2 Lv.3
	{"GIL-straight422",  __("Straight")},
	{"GIL-straight2",    __("Straight Lv.2")},
	{"GIL-straight3",    __("Straight Lv.3")},
	{"GIL-k13r-01",      __("Kick 13")},
	{"GIL-k13r-02",      __("Kick 13")},
	{"GIL-k13r-03",      __("Kick 13")},
	{"GIL-k13r-04",      __("Kick 13")},
	{"GIL-k13r-05",      __("Kick 13")},
	{"GIL-k13rDT-01",    __("DT Kick 13")},
	{"GIL-k13rDT-02",    __("DT Kick 13")},
	{"GIL-k13rDT-03",    __("DT Kick 13")},
	{"GIL-k13rDT-04",    __("DT Kick 13")},
	{"GIL-k13rDT-05",    __("DT Kick 13")},
	{"GIL-k13rDT-06",    __("DT Kick 13")},

    // gilgamesh swordmaster
	{"GIL-mgdv450",  	 __("Beast Uppercut")},
	{"GIL-rgdg460",  	 __("Rising Dragon")},
	{"GIL-dvdg461",  	 __("Divine Dragon")},
	{"GIL-dvdg462",  	 __("Divine Dragon")},
	{"GIL-rlimpct-01",   __("Real Impact")},
	{"GIL-rlimpct-02",   __("Real Impact")},
	{"GIL-rlimpct-03",   __("Real Impact")},
	{"GIL-inferno451",   __("Shock")},
	{"GIL-inferno470",   __("Shock Lv.2")},
	{"GIL-inferno471",   __("Shock Lv.3")},

    // lucifer
    {"NORMAL",           __("Embed")}, // pins and pandora's normal shot
	{"Stand",  			 __("Embed")}, // when the enemy is grounded and uh some other states
    {"Bomb",             __("Pin Explosion")},
    {"ROSE",             __("Rose")},
	{"LUCI-combo503",    __("Lucifer A1")},
	{"LUCI-combo504",    __("Lucifer A2")},
	{"LUCI-combo505",    __("Lucifer A3")},
	{"LUCI-combo506-1",  __("Lucifer A4")},
    {"LUCI-combo506-2",  __("Lucifer A4")},
	{"LUCI-combo519-1",  __("Lucifer B1")},
	{"LUCI-combo519-2",  __("Lucifer B1")},
	{"LUCI-combo508", 	 __("Lucifer E1")},
	{"LUCI-combo509", 	 __("Lucifer E2")},
	{"LUCI-combo510-1",  __("Lucifer E3")},
	{"LUCI-combo510-2",  __("Lucifer E3")},
    {"LUCI-combo514-1",  __("Splash")},
    {"LUCI-combo514-2",  __("Splash")},
    {"LUCI-combo514-3",  __("Splash")},

    // lucifer swordmaster
	{"Funnel", 			 __("Discipline & Bondage")},
	{"BombBariier",  	 __("Climax")},

    // e&i
    {"SHL000",           __("E&I")},
    {"SHL00-LEN2",       __("E&I")},
    {"SHL00_LEN3",       __("E&I")},
    {"CHARGE",           __("Charged E&I")},
    {"CHARGE_LEN2",      __("Charged E&I")},
    {"CHARGE_LEN3",      __("Charged E&I")},

    // e&i gunslinger
    {"RAIN_STORM",       __("Rainstorm")},
    {"RAIN_STORM_LEN2",  __("Rainstorm")},
    {"RAIN_STORM_LEN3",  __("Rainstorm")},
    {"DT_RAIN_STORM",    __("Charged Rainstorm")},
    {"DT_RAIN_STORM_L",  __("Charged Rainstorm")},
	{"TWO_SOMETIME",     __("Twosome Time")},
    {"TWO_SOMETIME_L2",  __("Twosome Time")},
    {"TWO_SOMETIME_L3",  __("Twosome Time")},
	{"DT_TWO_SOMETIME",  __("Charged Twosome Time")},
    {"HONEY_COMB",       __("Honeycomb Fire")},
    {"HONEY_COMB_LEN2",  __("Honeycomb Fire")},
    {"HONEY_COMB_LEN3",  __("Honeycomb Fire")},
    {"DT_HONEY_COMB",    __("Charged Honeycomb Fire")},
    {"DT_HONEY_COMB_L",  __("Charged Honeycomb Fire")},

    // coyote
    {"NRML_ETC_LV1",     __("Coyote")},
    {"NRML_CEN_LV1",     __("Coyote")},
    {"NRML_ETC_LV2",     __("Coyote")},
    {"NRML_CEN_LV2",     __("Coyote")},
    {"NRML_ETC_LV3",     __("Coyote")},
    {"NRML_CEN_LV3",     __("Coyote")},
	{"CHRG_ETC_LV1",     __("Charged Coyote")},
    {"CHRG_CEN_LV1",     __("Charged Coyote")},
    {"CHRG_ETC_LV2",     __("Charged Coyote")},
    {"CHRG_CEN_LV2",     __("Charged Coyote")},
    {"CHRG_ETC_LV3",     __("Charged Coyote")},
    {"CHRG_CEN_LV3",     __("Charged Coyote")},

    // coyote gunslinger
	{"BACK_ETC_LV1",     __("Backslide")},
	{"BACK_CEN_LV1",     __("Backslide")},
	{"BACK_ETC_LV2",     __("Backslide")},
	{"BACK_CEN_LV2",     __("Backslide")},
	{"BACK_ETC_LV3",     __("Backslide")},
	{"BACK_CEN_LV3",     __("Backslide")},
    {"DTBC_ETC_LV1",     __("Charged Backslide")},
    {"DTBC_CEN_LV1",     __("Charged Backslide")},
    {"DTBC_ETC_LV2",     __("Charged Backslide")},
    {"DTBC_CEN_LV2",     __("Charged Backslide")},
    {"DTBC_ETC_LV3",     __("Charged Backslide")},
    {"DTBC_CEN_LV3",     __("Charged Backslide")},
	{"NCHA_STR_LV1",     __("Fireworks")},
	{"NCHA_STR_LV2",     __("Fireworks")},
	{"NCHA_STR_LV3",     __("Fireworks")},
	{"NCHA_WEK_LV1",     __("Fireworks")},
	{"NCHA_WEK_LV2",     __("Fireworks")},
	{"NCHA_WEK_LV3",     __("Fireworks")},
	{"DTNC_STR_LV1",     __("Charged Fireworks")},
	{"DTNC_STR_LV2",     __("Charged Fireworks")},
	{"DTNC_STR_LV3",     __("Charged Fireworks")},
	{"DTNC_WEK_LV1",     __("Charged Fireworks")},
	{"DTNC_WEK_LV2",     __("Charged Fireworks")},
	{"DTNC_WEK_LV3",     __("Charged Fireworks")},
	{"STG-gunstinger",   __("Gun Stinger")},
	{"STNG_ETC_LV1",     __("Gun Stinger")},
	{"STNG_CEN_LV1",     __("Gun Stinger")},
	{"STNG_ETC_LV2",     __("Gun Stinger")},
	{"STNG_CEN_LV2",     __("Gun Stinger")},
	{"STNG_ETC_LV3",     __("Gun Stinger")},
	{"STNG_CEN_LV3",     __("Gun Stinger")},
	{"DTST_ETC_LV1",     __("Charged Gun Stinger")},
	{"DTST_CEN_LV1",     __("Charged Gun Stinger")},
	{"DTST_ETC_LV2",     __("Charged Gun Stinger")},
	{"DTST_CEN_LV2",     __("Charged Gun Stinger")},
	{"DTST_ETC_LV3",     __("Charged Gun Stinger")},
	{"DTST_CEN_LV3",     __("Charged Gun Stinger")},

    // pandora
    {"MACHINEGUN",       __("Jealousy")},
    {"MACHINEGUN_LV2",   __("Jealousy")},
    {"MACHINEGUN_LV3",   __("Jealousy")},
    {"DT_MGUN",          __("Charged Jealousy")},
    {"DT_MGUN_LV2",      __("Charged Jealousy")},
    {"DT_MGUN_LV3",      __("Charged Jealousy")},
	{"BOM_NOR",          __("Epidemic")},
    {"NORMAL02",         __("Hatred")}, // embed
	{"BOM_NOR02",        __("Hatred")},
	{"NORMAL03",         __("Revenge")},
    {"PDR-lanc3tf",      __("Revenge Hit")}, // physical hit
	
    // pandora gunslinger
	{"METEOR",        	 __("Argument")},
	{"BOM_METEOR",       __("Argument")},
	{"PDR-ma666s",  	 __("Omen Hit")},
    {"PDR-ma666e",  	 __("Omen Back Hit")}, // behind physical hit
	{"BOOMERANG",        __("Grief")},

    // yamato
	{"YMT-combo1003",    __("Yamato Combo A")},
	{"YMT-combo1004",    __("Yamato Combo A")},
	{"YMT-combo1005",    __("Yamato Combo A")},
    {"YMT-combo1006-1",  __("Aerial Rave V")},
    {"YMT-combo1006-2",  __("Aerial Rave V")},
    {"YMT-combo1007",    __("Aerial Rave V")},
	{"kuukan",   		 __("Slash Dimension")},
//};

//std::unordered_map<std::string, std::string> neroLookupTable = { // uh some things can go off after a char switch

    // Beyblade
    {"Attack1",          __("Gyro")},

    // scarecrow
    {"Em000",            __("Scarecrow Buster")}, // non dt
    {"Em000_1",          __("Scarecrow Buster")}, // dt ground
    {"Em000_2",          __("Scarecrow Buster")}, // dt air
    {"Grab-Attack",      __("Collateral")},     // enemy hit by bustered scarecrow
    {"Grab-Attack-DT",   __("Collateral")},     // enemy hit by bustered scarecrow
    {"em000_001_dt_th",  __("Collateral")},     // enemy hit by bustered scarecrow (air + dt)

    // mega
    {"Em003",            __("Mega Buster")},
    {"Em003_0",          __("Mega Buster")},
    {"Em003_1",          __("Mega Buster")},
    {"Em003_2",          __("Mega Buster")},
    {"Em003_3",          __("Mega Buster")},
    {"BUS-Em003",        __("Mega Buster")},
    {"Em003-roll_coun",  __("Mega Counter")},
    {"Em003_ground_th",  __("Mega Counter")},
    {"makikomi152",      __("Collateral")},
    {"makikomi158",      __("Collateral")},     // enemy hit by bustered mega
    {"makikomi156",      __("Collateral")}, 

    // bianco
    {"Em005",            __("Bianco Buster")},
    {"Em005_2",          __("Bianco Buster")},
    {"Em005_3",          __("Bianco Buster")},
    {"Em005_4",          __("Bianco Buster")},
    {"BusterThrust",     __("Bianco Buster")},
    {"Em005Majin",       __("Bianco Buster")},
    {"Em005Majin_2",     __("Bianco Buster")},
    {"Em005Majin_3",     __("Bianco Buster")},
    {"Em005Majin_4",     __("Bianco Buster")},
    {"BusterBlown",      __("Collateral")},     // enemy hit by bustered bianco
    {"BusterExplosion",  __("Collateral")},     // enemy hit by bustered bianco
    {"D_Buster",         __("Collateral")},     // enemy hit by bustered bianco

    // alto
    {"Em006",            __("Alto Buster")},
    {"Em006_2",          __("Alto Buster")},
    {"Em006Majin",       __("Alto Buster")},
    {"Em006Majin_2",     __("Alto Buster")},
    {"Em006Majin_3",     __("Alto Buster")},
    {"Em006Flip",        __("Alto Buster")}, // if alto jumps back when you buster
    {"PL_Genki",         __("Energy Ball")},
    {"PL_GenkiExplo",    __("Energy Ball Explosion")},
    {"BusterImpact",     __("Collateral")},     // enemy hit by bustered alto
    {"Blown",            __("Collateral")},     // enemy hit by bustered alto

    // mephisto
    {"Em008",            __("Mephisto Buster")},
    {"Em008_2",          __("Mephisto Buster")},
    {"Em008_3",          __("Mephisto Buster")},
    {"em008-rolled",     __("Collateral")},     // enemy hit by bustered mephisto

    // faust
    {"Em009",            __("Faust Buster")},
    {"Em009_2",          __("Faust Buster")},
    {"Em009_3",          __("Faust Buster")},
    {"em009-rolled",     __("Collateral")},     // enemy hit by bustered faust

    // frost
    {"Em010Release",     __("Frost Buster")},
    {"Em010WallHit",     __("Frost Buster")},
    {"em010Throw",       __("Collateral")},     // enemy hit by bustered frost
    {"Buster0Atk",       __("Collateral")},     // enemy hit by bustered frost
    {"Buster1Atk",       __("Collateral")},     // enemy hit by bustered frost
    {"Shooter1Atk",      __("Friendly Fire")},  // frost hitting enemy being bustered
    {"Shooter2Atk",      __("Friendly Fire")},  // frost hitting enemy being bustered
    {"NailMissileAtk",   __("Friendly Fire")},  // frost hitting enemy being bustered
    {"HighJumpAtk",      __("Friendly Fire")},  // frost hitting enemy being bustered
    {"MegaCrushAtk",     __("Friendly Fire")},  // frost hitting enemy being bustered
    {"WideNailAtk",      __("Friendly Fire")},  // frost hitting enemy being bustered

    // assault
    {"Em011Grab",        __("Assault Buster")},
    {"Em011Grab_Fin",    __("Assault Buster")},
    {"Em011Grab_Count",  __("Assault Buster")},
    {"grabed-attack",    __("Collateral")},     // enemy hit by bustered assault
    {"grabed-at-fin",    __("Collateral")},     // enemy hit by bustered assault
    {"grabed-at-f",      __("Collateral")},     // enemy hit by bustered assault
    {"wind-0",           __("Friendly Fire")},  // enemy being bustered hit by screaming assault
    {"nail-mis",         __("Friendly Fire")},  // enemy being bustered hit by assault projectile
    {"bullet-cn",        __("Friendly Fire")},  // enemy being bustered hit by assault flying swallow
    {"crow-ht",          __("Friendly Fire")},  // enemy being bustered hit by assault 

    // blitz
    {"BUS-Em012_Gr_00",  __("Blitz Buster")}, // ground
    {"BUS-Em012_Gr_01",  __("Blitz Buster")}, // ground
    {"BUS-Em012_Gr_02",  __("Blitz Buster")}, // ground
    {"BUS-Em012_Gr_03",  __("Blitz Buster")}, // ground
    {"BUS-Em012_Gr_04",  __("Blitz Buster")}, // ground
    {"BUS-Em012_Gr_05",  __("Blitz Buster")}, // ground
    {"BUS-Em012_Gr_06",  __("Blitz Buster")}, // ground
    {"BUS-Em012_Gr_07",  __("Blitz Buster")}, // ground
    {"BUS-Em012_Ai_00",  __("Blitz Buster")}, // air
    {"BUS-Em012_Ai_01",  __("Blitz Buster")}, // air
    {"BUS-Em012_Ai_02",  __("Blitz Buster")}, // air
    {"BUS-Em012_Ai_03",  __("Blitz Buster")}, // air
    {"BUS-Em012_Ai_04",  __("Blitz Buster")}, // air
    {"BUS-Em012_Ai_05",  __("Blitz Buster")}, // air
    {"BUS-Em012_Ai_06",  __("Blitz Buster")}, // air
    {"BUS-Em012_Ai_07",  __("Blitz Buster")}, // air

    // chimera seed
    {"Em013",            __("Chimera Seed Buster")},

    // cutlass
    {"Em015",            __("Cutlass Buster")},
    {"em015",            __("Cutlass Buster")},
    {"em015-throw_nml",  __("Collateral")},
    {"em015-throw_dt",   __("Collateral")},
    {"AtkThrowingNml",   __("Collateral")}, // hitting enemies with the spin

    // gladius
    {"Em016",            __("Gladius Buster")},
    {"em016",            __("Gladius Buster")},
    {"D_BusterThrow",    __("Gladius Buster")},
    {"BusterThrow",      __("Collateral")},     // enemy hit by bustered gladius
    {"D_BusterSlash",    __("Collateral")},     // enemy hit by bustered gladius (dt)
    {"Buster_VS_Barri",  __("Collateral")},     // hitting the Agnus window with a gladius
    {"D_Buster_VS_Bar",  __("Collateral")},     // hitting the Agnus window with a gladius (dt)

    // basilisk
    {"Em017",            __("Basilisk Buster")},
    {"ShootNero",        __("Collateral")},     // enemy hit by bustered basilisk
    {"ShootNeroAir",     __("Collateral")},     // enemy hit by bustered basilisk
    {"ShootNero_D",      __("Collateral")},     // enemy hit by bustered basilisk
    {"ShootNeroAir_D",   __("Collateral")},     // enemy hit by bustered basilisk
    {"em017Throw",       __("Collateral")},     // enemy hit by bustered basilisk
    {"em017Throw_Boun",  __("Collateral")},     // enemy hit by bustered basilisk (dt)

    // berial
    {"Em018Down",        __("Berial Buster")},
    {"Em018Down2",       __("Berial Buster")},
    {"Em018Head",        __("Berial Buster")},
    {"Em018Head_maji",   __("Berial Buster")},
    {"Em018Head_maji2",  __("Berial Buster")},
    {"Em018Leg",         __("Berial Buster")},
    {"Em018Leg2",        __("Berial Buster")},
    {"Em018Leg2_maji",   __("Berial Buster")},
    {"Em018Leg3_maji",   __("Berial Buster")},
    {"Em018Leg_maji",    __("Berial Buster")},
    {"Em018Leg_maji2",   __("Berial Buster")},
    {"Em018Down_maji",   __("Berial Buster")},
    {"Em018Down_maji2",  __("Berial Buster")},
    {"Em018Down_maji3",  __("Berial Buster")},
    {"Em018FireLowF",    __("Berial Buster")},
    {"Em018FireLowB",    __("Berial Buster")},
    {"Em018FireUp",      __("Berial Buster")},
    {"Syoryu_Body",      __("Friendly Fire")}, // Berial hit an enemy you were bustering
    {"Sword_Kaze",       __("Friendly Fire")}, // Berial hit an enemy you were bustering
    {"Sword_Base",       __("Friendly Fire")}, // Berial hit an enemy you were bustering
    {"Right_Hand",       __("Friendly Fire")}, // Berial hit an enemy you were bustering
    {"Tategiri_Sword",   __("Friendly Fire")}, // Berial hit an enemy you were bustering
    {"Super_Attack",     __("Friendly Fire")}, // Berial hit an enemy you were bustering
    {"EM018SHL",         __("Friendly Fire")}, // Berial hit an enemy you were bustering

    // frog
    {"Em019Tail",        __("Frog Buster")},
    {"Em019Tail2",       __("Frog Buster")},
    {"Em019Tail3",       __("Frog Buster")},
    {"Em019TailMaj",     __("Frog Buster")},
    {"Em019TailMaj2",    __("Frog Buster")},
    {"Em019TailMaj3",    __("Frog Buster")},
    {"Em019Ranbu",       __("Frog Buster")},
    {"Em019Ranbu1",      __("Frog Buster")},
    {"Em019Ranbu2",      __("Frog Buster")},
    {"Em019Ranbu3",      __("Frog Buster")},
    {"Em019Ranbu4",      __("Frog Buster")},
    {"Em019Ranbu5",      __("Frog Buster")},
    {"Em019Ranbu6",      __("Frog Buster")},
    {"Em019Ranbu7",      __("Frog Buster")},
    {"Em019Ranbu8",      __("Frog Buster")},
    {"Em019Ranbu9",      __("Frog Buster")},
    {"Em019RanbuMaj",    __("Frog Buster")},
    {"Em019RanbuMaj0",   __("Frog Buster")},
    {"Em019RanbuMaj1",   __("Frog Buster")},
    {"Em019RanbuMaj2",   __("Frog Buster")},
    {"Em019RanbuMaj3",   __("Frog Buster")},
    {"Em019RanbuMaj4",   __("Frog Buster")},
    {"Em019RanbuMaj5",   __("Frog Buster")},
    {"Em019RanbuMaj6",   __("Frog Buster")},
    {"Em019RanbuMaj7",   __("Frog Buster")},
    {"Em019RanbuMaj8",   __("Frog Buster")},
    {"Em019RanbuMaj9",   __("Frog Buster")},
    {"Em019RanbuMaj10",  __("Frog Buster")},
    {"Em019RanbuMaj11",  __("Frog Buster")},
    {"Em019RanbuMaj12",  __("Frog Buster")},
    {"Em019RanbuMaj13",  __("Frog Buster")},
    {"Em019RanbuMaj14",  __("Frog Buster")},
    {"Icedmg-rec",       __("Collateral")},     // enemy hit by breaking out of ice

    // echidna
    {"Em021Norm1",       __("Echidna Buster")},
    {"Em021Norm2",       __("Echidna Buster")},
    {"Em021Norm3",       __("Echidna Buster")},
    {"Em021Norm4",       __("Echidna Buster")},
    {"Em021Norm5",       __("Echidna Buster")},
    {"Em021Norm6",       __("Echidna Buster")},
    {"Em021Norm7",       __("Echidna Buster")},
    {"Em021Norm8",       __("Echidna Buster")},
    {"Em021Norm_Ryu1",   __("Echidna Buster")},
    {"Em021Norm_Ryu2",   __("Echidna Buster")},
    {"Em021Norm_Ryu3",   __("Echidna Buster")},
    {"Em021Norm_Ryu4",   __("Echidna Buster")},
    {"Em021Norm_Ryu5",   __("Echidna Buster")},
    {"Em021Norm_Ryu6",   __("Echidna Buster")},
    {"Em021Maji_Ryu1",   __("Echidna Buster")},
    {"Em021Maji_Ryu2",   __("Echidna Buster")},
    {"BUS-Em021_Bl_00",  __("Echidna Buster")},
    {"BUS-Em021_Bl_01",  __("Echidna Buster")},
    {"BUS-Em021_Bl_02",  __("Echidna Buster")},
    {"BUS-Em021_Bl_03",  __("Echidna Buster")},
    {"BUS-Em021_Bl_04",  __("Echidna Buster")},
    {"BUS-Em021_Bl_05",  __("Echidna Buster")},
    {"BUS-Em021_Bl_06",  __("Echidna Buster")},
    {"BUS-Em021_Bl_07",  __("Echidna Buster")},
    {"Em021Maji1",       __("Echidna Buster")},
    {"Em021Maji2",       __("Echidna Buster")},
    {"Em021Maji3",       __("Echidna Buster")},
    {"Em021Maji4",       __("Echidna Buster")},
    {"Em021Maji5",       __("Echidna Buster")},
    {"Em021Maji6",       __("Echidna Buster")},
    {"Em021Maji7",       __("Echidna Buster")},
    {"Em021Maji8",       __("Echidna Buster")},
    {"Em021Other",       __("Echidna Buster")}, // buster fail
    {"SeedPLAtk",        __("Seed Rebound")},

    // credo
    {"SpearBuster_PL",   __("Credo Buster")},
    {"BUS-Em022_Flip",   __("Credo Buster")},
    {"BUS-Em022_00",     __("Credo Buster")},
    {"BUS-Em022_01",     __("Credo Buster")},
    {"BUS-Em022_02",     __("Credo Buster")},
    {"BUS-Em022_03",     __("Credo Buster")},
    {"BUS-Em022_04",     __("Credo Buster")},
    {"BUS-Em022_05",     __("Credo Buster")},
    {"BUS-Em022_Mj_00",  __("Credo Buster")},
    {"BUS-Em022_Mj_01",  __("Credo Buster")},
    {"BUS-Em022_Mj_02",  __("Credo Buster")},
    {"BUS-Em022_Mj_03",  __("Credo Buster")},
    {"BUS-Em022_Mj_04",  __("Credo Buster")},
    {"BUS-Em022_Mj_05",  __("Credo Buster")},
    {"BUS-Em022_Mj_06",  __("Credo Buster")},
    {"SwordCmb1",        __("Friendly Fire")},

    // agnus
    {"BusterLoop",       __("Agnus Buster")},
    {"Sword",            __("Agnus Buster")},
    {"Em023Norm1",       __("Agnus Buster")},
    {"Em023Norm2",       __("Agnus Buster")},
    {"Em023Norm3",       __("Agnus Buster")},
    {"Em023Norm4",       __("Agnus Buster")},
    {"Em023Norm5",       __("Agnus Buster")},
    {"Em023Norm6",       __("Agnus Buster")},
    {"Em023Majin1",      __("Agnus Buster")},
    {"Em023Majin2",      __("Agnus Buster")},
    {"Em023Majin3",      __("Agnus Buster")},
    {"Em023Majin4",      __("Agnus Buster")},
    {"Em023Majin5",      __("Agnus Buster")},
    {"Em023Majin6",      __("Agnus Buster")},
    {"Em023Majin7",      __("Agnus Buster")},
    {"Buster1stImpact",  __("Agnus Buster")},
    {"Buster3rdStrike",  __("Agnus Buster")},

    // m11 sanctus
    {"BUS-Em029_00",     __("Sanctus Buster")},
    {"BUS-Em029_01",     __("Sanctus Buster")},
    {"BUS-Em029_02",     __("Sanctus Buster")},
    {"BUS-Em029_03",     __("Sanctus Buster")},
    {"BUS-Em029_04",     __("Sanctus Buster")},
    {"BUS-Em029_05",     __("Sanctus Buster")},
    {"BUS-Em029_06",     __("Sanctus Buster")},
    {"BUS-Em029_07",     __("Sanctus Buster")},
    {"BUS-Em029_08",     __("Sanctus Buster")},
    {"BUS-Em029_09",     __("Sanctus Buster")},
    {"BUS-Em029_10",     __("Sanctus Buster")},

    // m20 sanctus
    {"BUS-Em030_00",     __("Sanctus Buster")},
    {"BUS-Em030_01",     __("Sanctus Buster")},
    {"BUS-Em030_02",     __("Sanctus Buster")},
    {"BUS-Em030_Mj_00",  __("Sanctus Buster")},
    {"BUS-Em030_Mj_01",  __("Sanctus Buster")},
    {"BUS-Em030_Mj_02",  __("Sanctus Buster")},

    // dante
    {"EmDanteMaj",       __("Dante Buster")},
    {"EmDanteMajAir",    __("Dante Buster")},
    {"EmDanteMajFin",    __("Dante Buster")},
    {"EmDanteNorm",      __("Dante Buster")},
    {"EmDanteThrow",     __("Dante Buster")}, // after sword clashes
    {"em_dante_throw",   __("Collateral")},   // hitting another thing with dante
    {"em_dante_dt_thr",  __("Collateral")},   // hitting another thing with dante (dt)

    // red queen
    {"RED-ComboA_00",    __("Red Queen Combo A")},
    {"RED-ComboA_01",    __("Red Queen Combo A")},
    {"RED-ComboA_02",    __("Red Queen Combo A")},
    {"RED-ComboA_03",    __("Red Queen Combo A")},
    {"RED-ComboB_00",    __("Red Queen Combo B")},
    {"RED-ComboB_01",    __("Red Queen Combo B")},
    {"RED-ComboB_02_A",  __("Red Queen Combo B")},
    {"RED-ComboB_02_B",  __("Red Queen Combo B")},
    {"RED-ComboB_02_C",  __("Red Queen Combo B")},
    {"RED-ComboB_03",    __("Red Queen Combo B")},
    {"RED-ComboB_04",    __("Red Queen Combo B")},
    {"RED-ComboC_00",    __("Red Queen Combo C")},
    {"RED-ComboC_01",    __("Red Queen Combo C")},
    {"RED-ComboC_02",    __("Red Queen Combo C")},
    {"RED-ComboC_03",    __("Red Queen Combo C")},
    {"RED-ComboC_04",    __("Red Queen Combo C")},
    {"RED-ComboD_00",    __("Red Queen Combo D")},
    {"RED-ComboD_01",    __("Red Queen Combo D")},
    {"RED-Streak",       __("Streak")},
    {"RED-StreEX_00",    __("EX Streak")},
    {"RED-StreEX_01",    __("EX Streak")},        // no clarification on ex2 or 3
    {"RED-Highro",       __("High Roller")},      
    {"RED-HighroEX_00",  __("EX High Roller")},   // grounded 1, 2, 3
    {"RED-HighroEX_01",  __("EX High Roller")},   // aerial      2
    {"RED-HighroEX_02",  __("EX High Roller")},   // back hit
    {"RED-HighroEX_03",  __("EX High Roller")},   // grounded    2, 3
    {"RED-HighroEX_04",  __("EX High Roller")},   // grounded       3
    {"RED-HighroEX_05",  __("EX High Roller")},   // aerial      2, 3
    {"RED-HighroEX_06",  __("EX High Roller")},   // aerial      2
    {"RED-Split_00",     __("Split")},            // aerial
    {"RED-Split_01",     __("Split")},            // grounded
    {"RED-SplitEX_00",   __("Double Down")},         // 1, 2, 3
    {"RED-SplitEX_01",   __("Double Down")},         // 1
    {"RED-SplitEX_02",   __("Double Down")},         //    2
    {"RED-SplitEX_03",   __("Double Down")},         //       3
    {"RED-Surren_00",    __("Shuffle")},
    {"RED-Surren_01",    __("Shuffle")},          // weak hit
    {"RED-SureenEX_00",  __("EX Shuffle")},       // EX1
    {"RED-SureenEX_01",  __("EX Shuffle")},       // EX1 // rare, I assume weak hit or back hit
    {"RED-SureenEX_02",  __("EX Shuffle")},       // EX1
    {"RED-SureenEX_03",  __("EX Shuffle")},       // EX1 // rare, I assume weak hit or back hit
    {"RED-SureenEX_04",  __("EX Shuffle")},       // EX2
    {"RED-SureenEX_05",  __("EX Shuffle")},       // EX2 // rare, I assume weak hit or back hit
    {"RED-SureenEX_06",  __("EX Shuffle")},       // EX2
    {"RED-SureenEX_07",  __("EX Shuffle")},       // EX2 // rare, I assume weak hit or back hit
    {"RED-SureenEX_08",  __("EX Shuffle")},       // EX3
    {"RED-SureenEX_09",  __("EX Shuffle")},       // EX3 // rare, I assume weak hit or back hit
    {"RED-SureenEX_10",  __("EX Shuffle")},       // EX3
    {"RED-SureenEX_11",  __("EX Shuffle")},       // EX3
    {"RED-AirStre_00",   __("Calibur")},
    {"RED-AirStre_01",   __("Calibur")},
    {"RED-AirStEX_00",   __("EX Calibur")},       // EX1
    {"RED-AirStEX_01",   __("EX Calibur")},       // EX1
    {"RED-AirStEX_02",   __("EX Calibur")},       // EX2
    {"RED-AirStEX_03",   __("EX Calibur")},       // EX2
    {"RED-AirStEX_04",   __("EX Calibur")},       // EX3
    {"RED-AirStEX_05",   __("EX Calibur")},       // EX3
    {"RED-AircombA_00",  __("Red Queen Aerial")}, // 1
    {"RED-AircombA_01",  __("Red Queen Aerial")}, // 2
    {"RED-AircombA_02",  __("Red Queen Aerial")}, // 3
    {"RED-RouletteSpi",  __("Roulette Spin")},

    // yamato
    {"YAM-ComboA_00",    __("Red Queen Combo A")},
    {"YAM-ComboA_01",    __("Red Queen Combo A")},
    {"YAM-ComboA_02",    __("Red Queen Combo A")},
    {"YAM-ComboA_03",    __("Red Queen Combo A")},
    {"YAM-ComboB_00",    __("Red Queen Combo B")},
    {"YAM-ComboB_01",    __("Red Queen Combo B")},
    {"YAM-ComboB_02_A",  __("Red Queen Combo B")},
    {"YAM-ComboB_02_B",  __("Red Queen Combo B")},
    {"YAM-ComboB_02_C",  __("Red Queen Combo B")},
    {"YAM-ComboB_03",    __("Red Queen Combo B")},
    {"YAM-ComboB_04",    __("Red Queen Combo B")},
    {"YAM-ComboC_00",    __("Red Queen Combo C")},
    {"YAM-ComboC_01",    __("Red Queen Combo C")},
    {"YAM-ComboC_02",    __("Red Queen Combo C")},
    {"YAM-ComboC_03",    __("Red Queen Combo C")},
    {"YAM-ComboC_04",    __("Red Queen Combo C")},
    {"YAM-ComboD_00",    __("Red Queen Combo D")},
    {"YAM-Streak",       __("Streak")},
    {"YAM-StreEX_00",    __("EX Streak")},
    {"YAM-StreEX_01",    __("EX Streak")},
    {"YAM-Highro",       __("High Roller")},
    {"YAM-HighroEX_01",  __("EX High Roller")},
    {"YAM-HighroEX_03",  __("EX High Roller")},
    {"YAM-HighroEX_04",  __("EX High Roller")},
    {"YAM-HighroEX_05",  __("EX High Roller")},
    {"YAM-HighroEX_06",  __("EX High Roller")},
    {"YAM-AircombA_00",  __("Red Queen Aerial")},
    {"YAM-AircombA_01",  __("Red Queen Aerial")},
    {"YAM-AircombA_02",  __("Red Queen Aerial")},
    {"YAM-RouletteSpi",  __("Roulette Spin")},
    {"YAM-AirStre_00",   __("Calibur")},
    {"YAM-AirStre_01",   __("Calibur")},
    {"YAM-AirStEX_00",   __("EX Calibur")},
    {"YAM-AirStEX_01",   __("EX Calibur")},
    {"YAM-AirStEX_02",   __("EX Calibur")},
    {"YAM-AirStEX_03",   __("EX Calibur")},
    {"YAM-AirStEX_04",   __("EX Calibur")},
    {"YAM-AirStEX_05",   __("EX Calibur")},
    {"YAM-Surren_00",    __("Shuffle")},
    {"YAM-Surren_01",    __("Shuffle")},
    {"YAM-SureenEX_00",  __("EX Shuffle")}, // EX1
    {"YAM-SureenEX_01",  __("EX Shuffle")}, // EX1
    {"YAM-SureenEX_02",  __("EX Shuffle")}, // EX1
    {"YAM-SureenEX_03",  __("EX Shuffle")}, // EX2
    {"YAM-SureenEX_04",  __("EX Shuffle")}, // EX2
    {"YAM-SureenEX_05",  __("EX Shuffle")}, // EX2
    {"YAM-SureenEX_06",  __("EX Shuffle")}, // EX3
    {"YAM-SureenEX_08",  __("EX Shuffle")}, // EX3
    {"YAM-SureenEX_10",  __("EX Shuffle")}, // EX3
    {"YAM-Split_00",     __("Split")},
    {"YAM-Split_01",     __("Split")},
    {"YAM-SplitEX_00",   __("Double Down")}, // EX1
    {"YAM-SplitEX_01",   __("Double Down")}, // EX2
    {"YAM-SplitEX_02",   __("Double Down")}, // EX2
    {"YAM-SplitEX_03",   __("Double Down")}, // EX3

    // blue rose
    {"Shoot_1st_Short",  __("Blue Rose")},
    {"Shoot_2nd_short",  __("Blue Rose")},
    {"Shoot_1st_Midd",   __("Blue Rose")},
    {"Shoot_2nd_midd",   __("Blue Rose")},
    {"Shoot_1st_Long",   __("Blue Rose")},
    {"Shoot_2nd_Long",   __("Blue Rose")},
    {"CHARGE_Lv1_Shor",  __("Charge Shot 1")},
    {"CHARGE_Lv1_Midd",  __("Charge Shot 1")},
    {"CHARGE_Lv1_Long",  __("Charge Shot 1")},
    {"CHARGE_Lv2_Shor",  __("Charge Shot 2")},
    {"CHARGE_Lv2_Midd",  __("Charge Shot 2")},
    {"CHARGE_Lv2_Long",  __("Charge Shot 2")},
    {"CHARGE_Lv3_Shor",  __("Charge Shot 3")},
    {"CHARGE_Lv3_Midd",  __("Charge Shot 3")},
    {"CHARGE_Lv3_Long",  __("Charge Shot 3")},
    {"BLU-Bomb_00",      __("Charge Shot Bomb")},
    {"BLU-Bomb_01",      __("Charge Shot Bomb")},

    // dt
    {"D.T-Burst_00",     __("DT Burst")},
    {"D.T-Burst_01",     __("DT Burst")},     // weak hit
    {"D.T-AirBurst_00",  __("Air DT Burst")},
    {"D.T-AirBurst_01",  __("Air DT Burst")}, // weak hit
    {"D.T-Showdown_00",  __("Showdown")},
    {"D.T-Showdown_01",  __("Showdown")},
    {"D.T-Showdown_02",  __("Showdown")},
    {"D.T-Showdown_03",  __("Showdown")},
    {"D.T-Showdown_04",  __("Showdown")},
    {"D.T-Showdown_05",  __("Showdown")},
    {"D.T-Showdown_06",  __("Showdown")},
    {"D.T-Maximum_00",   __("Maximum Bet")},      // hit
    {"Drive",            __("Maximum Bet")},      // projectile
    {"D.T-Maximum_01",   __("Maximum Bet Lv.2")}, // hit
    {"Drive02",          __("Maximum Bet Lv.2")}, // projectile
    {"Genei",            __("Summoned Sword")},
    {"Genei02",          __("Charged Summoned Sword")},

    {"\xA0\x4F\xBE",     __("Hold Block")},     // held enemy hit by scarecrow, reads �O�
    {"\x90\x8B\xBA",     __("Hold Block")},     // held enemy hit by seed, reads ???
};

static const std::map<std::vector<const char*>, const char*> comboNames = {
    {{"High Time", "Aerial Rave", "Aerial Rave", "Aerial Rave", "Aerial Rave"}, "Very Creative!"},
    {{"High Time", "E&I", "E&I", "E&I", "E&I", "E&I"}, "Kamiya's Vision!"},
    {{"Guardfly", "Guardfly"}, "Is That A Plane?"},
    {{"Full House", "Enemy Step", "Full House", "Enemy Step", "Full House", "Enemy Step", "Full House", "Enemy Step", "Full House"}, "When the Lights Are Down"},
    {{"Full House", "Enemy Step", "Rainstorm", "Rainstorm", "Rainstorm", "Rainstorm",                                                     "E&I", "Full House", "Enemy Step", "Rainstorm", "Rainstorm", "Rainstorm"}, "The Forbidden Glitch"},
    {{"Full House", "Enemy Step", "Rainstorm", "Rainstorm", "Rainstorm", "Rainstorm", "Rainstorm",                                        "E&I", "Full House", "Enemy Step", "Rainstorm", "Rainstorm", "Rainstorm"}, "The Forbidden Glitch"},
    {{"Full House", "Enemy Step", "Rainstorm", "Rainstorm", "Rainstorm", "Rainstorm", "Rainstorm", "Rainstorm",                           "E&I", "Full House", "Enemy Step", "Rainstorm", "Rainstorm", "Rainstorm"}, "The Forbidden Glitch"},
    {{"Full House", "Enemy Step", "Rainstorm", "Rainstorm", "Rainstorm", "Rainstorm", "Rainstorm", "Rainstorm", "Rainstorm",              "E&I", "Full House", "Enemy Step", "Rainstorm", "Rainstorm", "Rainstorm"}, "The Forbidden Glitch"},
    {{"Full House", "Enemy Step", "Rainstorm", "Rainstorm", "Rainstorm", "Rainstorm", "Rainstorm", "Rainstorm", "Rainstorm", "Rainstorm", "E&I", "Full House", "Enemy Step", "Rainstorm", "Rainstorm", "Rainstorm"}, "The Forbidden Glitch"},
    {{"Red Queen Combo A", "Red Queen Combo A", "Red Queen Combo A", "High Roller", "Red Queen Aerial", "Red Queen Aerial", "Red Queen Aerial", "Split", "Split"}, "Grassroots"},
    {{"Calibur", "Enemy Step", "Blue Rose", "Blue Rose", "Calibur", "Enemy Step", "Blue Rose", "Blue Rose", "Calibur", "Enemy Step", "Blue Rose", "Blue Rose"}, "Capcom Dev"},
    {{"Red Queen Combo A", "EX High Roller", "EX High Roller", "EX High Roller", "Double Down", "Double Down", "EX Streak", "EX Streak", "EX Streak"}, "Full Throttle"},
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

static std::array<ComboUnlock, 8> unlocked_combos = {
    ComboUnlock {DANTE, 1, __("Very Creative!"), __("Welcome to Dante!"), __("As Dante, Used High Time > Rave x 4"), false},
    ComboUnlock {DANTE, 1, __("Kamiya's Vision!"), __("Did you know DMC1 was going to be RE4?"), __("As Dante, Used High Time > E&I Shot x 10"), false},
    ComboUnlock {DANTE, 3, __("Is That A Plane?"), __("Straight out of the window"), __("As Dante, Used Guardfly > Guardfly"), false},
    ComboUnlock {DANTE, 2, __("When the Lights Are Down"), __("Rules without exceptions last eternally"), __("As Dante, Looped Full House > Enemy Step 5 Times"), false},
    ComboUnlock {DANTE, 2, __("The Forbidden Glitch"), __("Inertia? Let's just make it steerable"), __("As Dante, Looped Full House > Enemy Step > E&I Shot > Rainstorm 2 Times"), false},
    ComboUnlock {NERO,  1, __("Grassroots"), __("Imagine this: It's 2008. You just picked up a new character. Make a combo"), __("As Nero, Used Red Queen Combo A1, A2, A3 > High Roller > Red Queen Aerial 1, 2, 3 > Split"), false},
    ComboUnlock {NERO,  3, __("Capcom Dev"), __("It's time to record a trailer for Devil May Cry 5"), __("As Nero, Looped Calibur > Enemy Step > Blue Rose 3 Times"), false},
    ComboUnlock {NERO,  4, __("Full Throttle"), __("Just press the button at the right time"), __("As Nero, Used Red Queen Combo A1 > EX3 High Roller > Double Down > EX3 Streak"), false},
};

struct TrickScore {
    std::string text;
    float score;
    float enemyMult;
    int styleLetter;
    float styleMult;
    std::chrono::steady_clock::time_point timePerformed;
    bool isAlreadyRenamed;

    TrickScore() 
        : text(""), 
          score(0.0f), 
          enemyMult(1.0f), 
          styleLetter(0), 
          styleMult(1.0f),
          timePerformed(std::chrono::steady_clock::now()),
          isAlreadyRenamed(false) {}

    TrickScore(const std::string& t, float s, int pos, float enemyMult, int styleLetter, float styleMult, bool renamed = false) 
        : text(t), 
          score(s), 
          enemyMult(enemyMult), 
          styleLetter(styleLetter),
          styleMult(styleMult),
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
bool StylePoints::style_points_display = false;
bool StylePoints::tonyHawk = false;
bool StylePoints::moreGrouping = false; 
bool StylePoints::originalNames = false;
// bool StylePoints::showAirTimeDisplay = false;
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
static float displayDuration = 1.0f;
static const uint32_t maxScores = 500;

// tony display
// static std::chrono::steady_clock::time_point lastTrickTime = std::chrono::steady_clock::now();
static float elapsedSinceLastTrick = 500.0f;
static const uint32_t maxTonyScores = UINT32_MAX;
static constexpr float timerBase = 1.0f;
static constexpr float timerComboInfluence = 0.005f;
static constexpr float shakeDuration = 0.99f;
static constexpr float fadeRate = 1.0f;
static constexpr float baseFadeTime = 1.0f / (timerBase * fadeRate);
static int maxPerRow = 7;
static int maxRows = 3;
static float comboScore = 0.0f;

float GetstyleLetterMultiplier(int styleNum) {
    switch (styleNum) {
        case 0: return 1.0f; // E
        case 1: return 1.5f; // D
        case 2: return 2.0f; // C
        case 3: return 2.5f; // B
        case 4: return 3.0f; // A
        case 5: return 4.0f; // S
        case 6: return 4.5f; // SS
        case 7: return 5.0f; // SSS
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
        ImGui::UpdateCurrentFontSize(correctedWindowFontScale * (2.0f * ImGui::GetStyle().FontSizeBase));
        float fontSize = ImGui::GetFontSize();
        
        struct ConsecutiveGroup {
            std::string trickName;
            float score;
            float multiplier;
            int styleLetter;
            int count;
            std::chrono::steady_clock::time_point mostRecentTime;
            int startIndex;
            int endIndex;
        };
        
        std::vector<ConsecutiveGroup> consecutiveGroups;
        
        if (!trickScores.empty()) {
            ConsecutiveGroup currentGroup;
            currentGroup.trickName = trickScores[0].text;
            currentGroup.score = trickScores[0].score;
            currentGroup.multiplier = trickScores[0].styleMult;
            currentGroup.styleLetter = trickScores[0].styleLetter;
            currentGroup.count = 1;
            currentGroup.mostRecentTime = trickScores[0].timePerformed;
            currentGroup.startIndex = 0;
            currentGroup.endIndex = 0;
            
            for (uint32_t i = 1; i < trickScores.size(); i++) {
                if (trickScores[i].text == currentGroup.trickName) {
                    // same trick, extend the current group
                    currentGroup.count++;
                    currentGroup.endIndex = i;
                    if (trickScores[i].timePerformed > currentGroup.mostRecentTime) {
                        currentGroup.mostRecentTime = trickScores[i].timePerformed;
                    }
                } else {
                    // different trick, save current group and start a new one
                    consecutiveGroups.push_back(currentGroup);
                    
                    currentGroup.trickName = trickScores[i].text;
                    currentGroup.score = trickScores[i].score;
                    currentGroup.multiplier = trickScores[i].styleMult;
                    currentGroup.styleLetter = trickScores[i].styleLetter;
                    currentGroup.count = 1;
                    currentGroup.mostRecentTime = trickScores[i].timePerformed;
                    currentGroup.startIndex = i;
                    currentGroup.endIndex = i;
                }
            }
            
            // add the last group
            consecutiveGroups.push_back(currentGroup);
        }
        
        // check which groups have timed out
        std::vector<ConsecutiveGroup> activeGroups;
        std::vector<int> trickIndicesToRemove;
        
        for (const auto& group : consecutiveGroups) {
            float elapsed = std::chrono::duration<float>(now - group.mostRecentTime).count();
            bool timedOut = (elapsed > displayDuration);
            
            if (timedOut) {
                // Mark all tricks in this group for removal
                for (int i = group.startIndex; i <= group.endIndex; i++) {
                    trickIndicesToRemove.push_back(i);
                }
            } else {
                activeGroups.push_back(group);
            }
        }
        
        // sort active groups by how recent they were
        std::sort(activeGroups.begin(), activeGroups.end(), 
            [](const ConsecutiveGroup& a, const ConsecutiveGroup& b) {
                return a.mostRecentTime > b.mostRecentTime;
            });
        
        // display active groups
        for (const auto& group : activeGroups) {
            float elapsed = std::chrono::duration<float>(now - group.mostRecentTime).count();
            float fade = 1.0f - (elapsed / displayDuration);
            fade = std::max(0.0f, std::min(1.0f, fade)); // Clamp fade between 0 and 1
            
            ImVec4 color(1.0f, 1.0f, 1.0f, fade);
            ImVec4 currentStyleColor = GetStyleColor(group.styleLetter);
            ImVec4 color2(currentStyleColor.x, currentStyleColor.y, currentStyleColor.z, fade);
            

            std::string translatedTrickName = utility::text_lookup((char*)group.trickName.c_str());
            std::string displayText = translatedTrickName;
            //std::string displayText = group.trickName;
            if (group.count > 1) {
                displayText += " x" + std::to_string(group.count);
            }
            
            float windowWidth = ImGui::GetContentRegionAvail().x;
            char scoreBuffer[32];
            snprintf(scoreBuffer, sizeof(scoreBuffer), "%.1f", group.score * 0.1 * group.multiplier);
            float textWidth = ImGui::CalcTextSize(displayText.c_str()).x;
            float scoreWidth = ImGui::CalcTextSize(scoreBuffer).x;
            float customSpacing = fontSize * 1.0f;
            float rightAlignX = (windowWidth * 0.8f) - (customSpacing * 0.5f);
            float leftAlignX = (windowWidth * 0.8f) + (customSpacing * 0.5f);
            
            // draw trick name
            ImGui::SetCursorPosX(rightAlignX - textWidth);
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::Text("%s", displayText.c_str());
            ImGui::PopStyleColor();
            
            // draw score
            ImGui::SameLine(leftAlignX);
            ImGui::PushStyleColor(ImGuiCol_Text, color2);
            ImGui::Text("%.1f", group.score * 0.1 * group.multiplier);
            ImGui::PopStyleColor();
        }
        
        // sort to remove in descending order
        std::sort(trickIndicesToRemove.begin(), trickIndicesToRemove.end(), std::greater<int>());
        
        // remove timed-out tricks
        for (uint32_t index : trickIndicesToRemove) {
            if (index >= 0 && index < trickScores.size()) {
                trickScores.erase(trickScores.begin() + index);
            }
        }
        
        ImGui::End();
    }
}

static void DrawTonyScores() {
    auto* player = devil4_sdk::get_local_player();
    if (!player) return;

    ImVec2 screenSize = devil4_sdk::get_sRender()->screenRes;
    //if (!StylePoints::originalNames)
    UpdateTrickNames();
    auto now = std::chrono::steady_clock::now();

    bool gamePaused = false;
    if (sArea* sArea = devil4_sdk::get_sArea()) gamePaused = sArea->aGamePtr->m_paused;

    float deltaTime = player->m_delta_time;
    float realSeconds = deltaTime / 60.0f;
    if (sArea* sArea = devil4_sdk::get_sArea()) {
        if (sArea->aGamePtr) {
            gamePaused = sArea->aGamePtr->m_paused;
        }
    }
    if (!gamePaused)
        elapsedSinceLastTrick += realSeconds;

    // fade
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
    if (fade > shakeDuration && !gamePaused && player->m_delta_time > 0.0f) {
        shakeAmount = dis(gen);
    }

    float scoreFontBaseScale = correctedWindowFontScale * 2.0f;
    float trickFontBaseScale = correctedWindowFontScale * 1.5f;
    float scoreFontScale = scoreFontBaseScale + shakeAmount * 0.1f;
    float trickFontScale = trickFontBaseScale;
    ImGui::UpdateCurrentFontSize(scoreFontScale * ImGui::GetStyle().FontSizeBase);

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
    int effectiveRowSize = 0;

    for (const auto& groupedTrick : groupedTricks) {
        comboScore += groupedTrick.summedScore * 0.1f;

        if (currentRow >= maxRows) continue;

        bool isEnemyStep = (groupedTrick.baseTrick.text == "Enemy Step");
        
        if (!isEnemyStep && effectiveRowSize >= maxPerRow) {
            currentRow++;
            effectiveRowSize = 0;
            
            if (currentRow >= maxRows) continue;
        }

        rowTricks[currentRow].push_back(groupedTrick);
        
        if (!isEnemyStep) {
            effectiveRowSize++;
        }
    }

    int currentStyleTier = devil4_sdk::get_stylish_count()->current_style_tier;
    ImVec4 currentStyleColor = GetStyleColor(currentStyleTier);

    char scoreText[32];
    snprintf(scoreText, sizeof(scoreText), "%.1f", comboScore);

    char styleTierText[32];
    snprintf(styleTierText, sizeof(styleTierText), "x%.1f", GetstyleLetterMultiplier(currentStyleTier));

    float totalWidth = ImGui::CalcTextSize(scoreText).x + ImGui::CalcTextSize(styleTierText).x + ImGui::CalcTextSize(" ").x;
    float scoreShakeAmount = shakeAmount * 1.0f;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f + scoreShakeAmount);
    ImGui::SetCursorPosY(0.0f + scoreShakeAmount);
    ImVec4 scoreColor = GetScoreColor(comboScore, 0.0f, 500.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(scoreColor.x, scoreColor.y, scoreColor.z, fade));
    ImGui::Text("%s", scoreText);
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImVec4 multiplierColor(currentStyleColor.x, currentStyleColor.y, currentStyleColor.z, fade);
    ImGui::PushStyleColor(ImGuiCol_Text, multiplierColor);
    ImGui::Text("%s", styleTierText);
    ImGui::PopStyleColor();

    ImGui::UpdateCurrentFontSize(trickFontScale * ImGui::GetStyle().FontSizeBase);

    float trickShakeAmount = shakeAmount * 5.0f;
    float rowStartY = ImGui::GetFontSize() * 1.5f;

    for (int row = 0; row < maxRows; ++row) {
        if (rowTricks[row].empty()) continue;

        std::vector<std::pair<std::string, ImVec4>> textSegments;
        float totalRowWidth = 0.0f;

        for (size_t i = 0; i < rowTricks[row].size(); ++i) {
            const auto& groupedTrick = rowTricks[row][i];
            
            if (groupedTrick.baseTrick.text == "Enemy Step") {
                continue;
            }
            
            char trickText[64];
            std::string translatedTrickName = utility::text_lookup((char*)groupedTrick.baseTrick.text.c_str());
            if (groupedTrick.repeatCount > 1) {
                snprintf(trickText, sizeof(trickText), "%s (x%d)", translatedTrickName.c_str(), groupedTrick.repeatCount);
            }
            else {
                snprintf(trickText, sizeof(trickText), "%s", translatedTrickName.c_str());
            }

            ImVec4 trickColor = GetTrickColor(groupedTrick.summedScore, fade);
            textSegments.emplace_back(trickText, trickColor);

            if (i < rowTricks[row].size() - 1) {
                bool nextIsEnemyStep = (rowTricks[row][i+1].baseTrick.text == "Enemy Step");
                bool skipToNextAfterEnemyStep = nextIsEnemyStep && (i+2 < rowTricks[row].size());
                
                if (nextIsEnemyStep) {
                    textSegments.emplace_back(" x ", ImVec4(1.0f, 1.0f, 1.0f, fade));
                    i++;
                } else {
                    textSegments.emplace_back(" + ", ImVec4(1.0f, 1.0f, 1.0f, fade));
                }
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
    float comboRecognitionAlpha = 1.0f - (elapsedSinceLastMatch);

    if (!trickScores.empty()) {
        const std::string& latestTrick = trickScores.back().text;
        if (lastProcessedTrick != latestTrick) {
            lastProcessedTrick = latestTrick;
        }
    }

    if (!trickScores.empty()) {
        for (const auto& combo : comboNames) {
            const std::vector<const char*>& comboSequence = combo.first;
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
                
                    detectedCombo =  utility::text_lookup((char*)combo.second); // combo.second;
                    lastMatchTime = std::chrono::steady_clock::now();
                    break;
                }
            }
        }
    }

    ImGui::SetNextWindowPos(ImVec2(screenSize.x * 0.2f, screenSize.y * 0.7f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(screenSize.x * 0.5f, screenSize.y * 0.5f));
    ImGui::Begin("ScoreRecognitionWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    ImGui::UpdateCurrentFontSize(correctedWindowFontScale * (1.5f * ImGui::GetStyle().FontSizeBase));
    if (comboRecognitionAlpha > 0.0f) {
        float textWidth = ImGui::CalcTextSize(detectedCombo.c_str()).x;
        float windowWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImVec4 comboColor(1.0f, 1.0f, 0.0f, comboRecognitionAlpha);
        ImGui::PushStyleColor(ImGuiCol_Text, comboColor);
        ImGui::Text("%s", detectedCombo.c_str());
        ImGui::PopStyleColor();
    }
    ImGui::End();

    // airtime window
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

    bool isInAir = false;
    if (player->collisionSettings)
        isInAir = !player->collisionSettings->mLand;
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
        ImGui::UpdateCurrentFontSize(correctedWindowFontScale * (1.5f * ImGui::GetStyle().FontSizeBase));
        ImVec4 airtimeColor(1.0f, 1.0f, 1.0f, airTimerAlpha);
        ImGui::PushStyleColor(ImGuiCol_Text, airtimeColor);
        float timeToDisplay = isInAir ? airTimer : displayedAirTime;
        ImGui::Text(_("Airtime: %.1f"), timeToDisplay);
        ImGui::PopStyleColor();
        ImGui::End();
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
            heightInertiaTrackerStateHistory.push_back({ realElapsed, player->m_pos.y, player->inertia });
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

static void AddTrickScore(const char* text, float score, float enemyMult, int styleLetter, float styleMult) {
    TrickScore newScore;
    newScore.text = text;
    newScore.score = score * enemyMult;
    newScore.enemyMult = enemyMult;
    newScore.styleLetter = styleLetter;
    newScore.styleMult = styleMult;
    newScore.timePerformed = std::chrono::steady_clock::now();
    newScore.isAlreadyRenamed = false;

    if (StylePoints::tonyHawk) {
        elapsedSinceLastTrick = 0.0f;
        // lastTrickTime = newScore.timePerformed;
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
naked void detour1(void) { // hit instances
    _asm {
        pushfd // 4 bytes
        cmp byte ptr [StylePoints::style_points_display], 1
        je cheatcode
        cmp byte ptr [StylePoints::tonyHawk], 1
        je cheatcode
        jmp originalcode

        cheatcode:
        movss [xmm0backup], xmm0
        movss [xmm1backup], xmm1

        pushad // 0x20 bytes

        sub esp, 4 // arg 5
        movss [esp], xmm2 // style mult
        push [esp+0x24+0x4+0xC] // styleLetter // backup pushes + 1 arg + style letter location // arg 4
        sub esp, 4 // arg 3
        movss [esp], xmm1 // enemyMult
        push [esp+0x24+0xC+0x4] // score // backup pushes + 3 args + score location // arg 2
        push ebx // text // arg 1
        call AddTrickScore // fucks eax, ecx, edx
        add esp, 0x14 // 5 args

        popad

        movss xmm0, [xmm0backup]    
        movss xmm1, [xmm1backup]

        originalcode:
        popfd
        movss xmm3, ds:[0xDECEDC] // [DevilMayCry4_DX9.exe+9ECEDC]
        jmp dword ptr [StylePoints::jmp_ret1]
    }
}

static const char* guardText = __("Guardfly");
static const float slowestGuardfly = 20.0f;
naked void detour2(void) { // called once on air guard start
    _asm {
        cmp byte ptr [StylePoints::tonyHawk], 1
        jne originalcode

        pushad

        movss xmm0,[ebp+0x1e1c]
        comiss xmm0, [slowestGuardfly]
        jb popcode
        push 0x3f800000 // styleMult
        push 0 // style letter
        push 0x3f800000 // enemyMult
        push 0 // score
        push guardText // name
        call AddTrickScore // fucks eax, ecx, edx
        add esp, 0x14 // 5 args

        popcode:
        popad

        originalcode:
        add [ebp+0x00001504],ebx
        jmp dword ptr [StylePoints::jmp_ret2]
    }
}

static const char* GetJustTypeName(int Type) { // return "" if you don't want it displayed
    switch (Type) {
        case 0: return "";
        case 1: return "";                // ADD_COMBO_GUAGE_PROVOKE_BONUS = 0x1, // Taunt
        case 2: return "Enemy Step";   // ADD_COMBO_GUAGE_EMJUMP = 0x2,
        case 3: return _("Snatch");       // ADD_COMBO_GUAGE_GRAB = 0x3,
        case 4: return "";                // ADD_COMBO_GUAGE_GRAB_NEAR = 0x4, // Buster
        case 5: return _("Dodge");        // ADD_COMBO_GUAGE_AVOIDED = 0x5,
        case 6: return "";                // ADD_COMBO_GUAGE_ENEMY_GUARD = 0x6,
        case 7: return "Block";           // ADD_COMBO_GUAGE_BLOCK = 0x7,
        case 8: return "";                // ADD_COMBO_GUAGE_RELEASE = 0x8,
        case 9: return "";                // ADD_COMBO_GUAGE_PANDORABOX = 0x9,
        case 10: return "";               // ADD_COMBO_GUAGE_TS_ESC = 0xa,
        case 11: return "";               // ADD_COMBO_GUAGE_YT_COMBO2 = 0xb,
        case 12: return _("Just Charge"); // ADD_COMBO_GUAGE_GM_JUST_SAVE = 0xc,
        case 13: return "";               // ADD_COMBO_GUAGE_TS_EM_HIKE = 0xd,
        case 14: return "";               // ADD_COMBO_GUAGE_RETURN_SWORD = 0xe,
        default: return "";
    }
}

naked void detour4(void) { // called on just actions and taunts
    _asm {
        cmp byte ptr [StylePoints::tonyHawk], 1
        jne originalcode

        pushad

        push 0x3f800000 // styleMult
        push 0 // style letter
        push 0x3f800000 // enemyMult
        push 0 // score
        push [esp+0x20+0x10+0x8] // just type name arg
        call GetJustTypeName
        add esp, 4 // just type name cleanup
        cmp byte ptr [eax], 0
        je escape
        push eax // name
        call AddTrickScore // fucks eax, ecx, edx
        add esp, 0x14 // 5 args

        popad

        originalcode:
        xor ecx,ecx
        lea eax,[esi+0x28]
        jmp dword ptr [StylePoints::jmp_ret4]

        escape:
        add esp,0x10
        popad
        jmp originalcode
    }
}

std::optional<std::string> StylePoints::on_initialize() {
    if (!install_hook_offset(0x547B2, hook1, &detour1, &jmp_ret1, 8)) { // hit instances
		spdlog::error("Failed to init StylePoints mod\n");
		return "Failed to init StylePoints mod";
	}

    if (!install_hook_offset(0x3CC13B, hook2, &detour2, &jmp_ret2, 6)) { // called once on air guard start
		spdlog::error("Failed to init StylePoints mod 2\n");
		return "Failed to init StylePoints mod 2";
	}

    if (!install_hook_offset(0xA5FFC, hook4, &detour4, &jmp_ret4, 5)) { // called on just actions and taunts
		spdlog::error("Failed to init StylePoints mod 4\n");
		return "Failed to init StylePoints mod 4";
	}

    after_reset();
    return Mod::on_initialize();
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
        bgDrawList->AddImage((ImTextureID)m_texture_handle, windowPos, ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y),
            ImVec2(0, 0), ImVec2(1.0, 1.0), ImColor::ImColor(bg_red_int, bg_green_int, bg_blue_int, bg_alpha_int));
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, windowPadding);
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);
    ImGui::Begin("Hidden Combos Panel", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
    ImVec2 availSpace = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("ScrollingContent", availSpace, false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
    static float fontScale = 1.0f;
    ImGui::UpdateCurrentFontSize(fontScale * ImGui::GetStyle().FontSizeBase);

    static ImVec4 TexCol{ 0.8f, 0.8f, 0.8f, 1.0f };
    ImGui::TextColored(TexCol, _("Hidden Combos"));

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
    
    ImGui::TextColored(TexCol, _("Dante - %i/%i Unlocked"), unlockedDante, totalDante);
    ImGui::NextColumn();

    ImGui::TextColored(TexCol, _("Nero - %i/%i Unlocked"), unlockedNero, totalNero);
    ImGui::NextColumn();

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

        ImGui::TextColored(TexCol, "%s", utility::text_lookup((char*)displayText.c_str()));

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(utility::text_lookup((char*)combo.hint));
            if (combo.unlocked) {
                ImGui::TextColored(TexCol, "(%s)", utility::text_lookup((char*)combo.how));
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
            displayText = utility::text_lookup((char*)combo.name);
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

        ImGui::TextColored(TexCol, "%s", utility::text_lookup((char*)displayText.c_str()));

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(utility::text_lookup((char*)combo.hint));
            if (combo.unlocked) {
                ImGui::TextColored(TexCol, "(%s)", utility::text_lookup((char*)combo.how));
            }
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
    
    ImGui::NextColumn();

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
    // ImGui::SliderInt(_("bg_red_int"),   &bg_red_int, 0, 255, "Red: %d");
    // ImGui::SliderInt(_("bg_green_int"), &bg_green_int, 0, 255, "Green: %d");
    // ImGui::SliderInt(_("bg_blue_int"),  &bg_blue_int, 0, 255, "Blue: %d");
    // ImGui::SliderInt(_("bg_alpha_int"), &bg_alpha_int, 0, 255, "Alpha: %d");
    // ImGui::InputFloat(_("windowPos.x"), &windowPosModifier.x, 0.0f, 1.0f, "%.2f");
    // ImGui::InputFloat(_("windowPos.y"), &windowPosModifier.y, 0.0f, 1.0f, "%.2f");
    // ImGui::InputFloat(_("windowSize.x"), &windowSizeModifier.x, 0.0f, 1.0f, "%.2f");
    // ImGui::InputFloat(_("windowSize.y"), &windowSizeModifier.y, 0.0f, 1.0f, "%.2f");
    // ImGui::InputFloat(_("windowPadding.x"), &windowPaddingModifier.x, 0.0f, 1.0f, "%.2f");
    // ImGui::InputFloat(_("windowPadding.y"), &windowPaddingModifier.y, 0.0f, 1.0f, "%.2f");
    // ImGui::InputFloat(_("Text r"), &TexCol.x);
    // ImGui::InputFloat(_("Text g"), &TexCol.y);
    // ImGui::InputFloat(_("Text b"), &TexCol.z);
    // ImGui::InputFloat(_("Text a"), &TexCol.w);
    // ImGui::InputFloat(_("fontScale"), &fontScale);
    ImGui::SameLine();
    const char* clear_unlocks_label = _("Clear Unlocks");
    ImVec2 btn_size = ImGui::CalcTextSize(clear_unlocks_label);
    btn_size.x += ImGui::GetStyle().FramePadding.x * 2.0f;
    btn_size.y += ImGui::GetStyle().FramePadding.y * 2.0f;
    float rightEdge = ImGui::GetWindowPos().x + ImGui::GetContentRegionAvail().x;
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

void StylePoints::on_gui_frame(int display) {
    ImGui::BeginGroup();
    ImGui::Checkbox(_("Tony"), &tonyHawk);
    ImGui::SameLine();
    help_marker(_("Tony"));
    if (tonyHawk) {
        style_points_display = false;
        originalNames = false;
        ImGui::Indent(lineIndent);
        // ImGui::Checkbox(_("Air Time Display"), &showAirTimeDisplay);
        ImGui::Checkbox(_("Alternate Grouping"), &moreGrouping);
        ImGui::SameLine();
        help_marker(_("Group attacks by the order you originally did them\n"
            "This helps show the variety in a combo over exact input order"));
        ImGui::Checkbox(_("Height Chart"), &showHeightChart);
        ImGui::SameLine();
        help_marker(_("Ever wanted to see a log of your height throughout a combo? Me neither!"));
        ImGui::Checkbox(_("Inertia Chart"), &showInertiaChart);
        ImGui::SameLine();
        help_marker(_("Ever wanted to see a log of your inertia throughout a combo? Me neither!"));
        /*
        ImGui::SliderFloat("timerBase", &timerBase, 0.0f, 2.0f, "%.1f");
        ImGui::SameLine();
        help_marker("DEV PLS REMOVE - The base time for how long you have before breaking a combo\n1.0 default");
        ImGui::SliderFloat("comboInfluence", &timerComboInfluence, 0.0f, 0.1f, "%.4f");
        ImGui::SameLine();
        help_marker("DEV PLS REMOVE - How much influence will the score from your last attack have on the timer\n0.0010 default");
        ImGui::SliderFloat("shakeDuration", &shakeDuration, 0.0f, 1.0f, "%.4f");
        ImGui::SameLine();
        help_marker("DEV PLS REMOVE - At what point in the timer should text shake stop\nRemember a faster timer means less shake, so edit this last\n0.99 default");
        ImGui::PushItemWidth(sameLineItemWidth);
        ImGui::SliderInt(_("maxRows"), &maxRows, 1, 10);
        ImGui::SameLine();
        help_marker("How many rows\n5 default");
        ImGui::SliderInt(_("maxPerRow"), &maxPerRow, 1, 10);
        ImGui::SameLine();
        help_marker("How many attacks per row\n7 default");
        ImGui::PopItemWidth();
        */
        ImGui::Unindent(lineIndent);
    }
    ImGui::EndGroup();
    ImGui::SameLine(sameLineWidth);
    ImGui::Checkbox(_("Style Point Display"), &style_points_display);
    ImGui::SameLine();
    help_marker(_("See exactly what goes into your style points count"));
    if (style_points_display) {
        tonyHawk = false;
        ImGui::Indent(lineIndent + sameLineWidth);
        ImGui::Checkbox(_("Use Original Names"), &originalNames);
        ImGui::SameLine();
        help_marker(_("Instead of using our skill renames, use the developers'. Because there are more unique names, less skills will be grouped"));
        ImGui::Unindent(lineIndent + sameLineWidth);
    }
}

void StylePoints::on_frame(fmilliseconds& dt) {
    if (tonyHawk) {
        ImVec2 screenSize = devil4_sdk::get_sRender()->screenRes;
        correctedWindowFontScale = (screenSize.x / baseWidth);
        DrawTonyScores();
        DrawHiddenCombos();
    }
    else if (style_points_display) {
        ImVec2 screenSize = devil4_sdk::get_sRender()->screenRes;
        correctedWindowFontScale = (screenSize.x / baseWidth);
        DrawTrickScores();
    }
}

void StylePoints::on_config_load(const utility::Config& cfg) {
    style_points_display = cfg.get<bool>("style_points_display").value_or(false);
    tonyHawk = cfg.get<bool>("hawk_points_display").value_or(false);
    moreGrouping = cfg.get<bool>("group_points_display").value_or(false);
    // showAirTimeDisplay = cfg.get<bool>("airtime_points_display").value_or(false);
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
    cfg.set<bool>("style_points_display", style_points_display);
    cfg.set<bool>("hawk_points_display", tonyHawk);
    cfg.set<bool>("group_points_display", moreGrouping);
    // cfg.set<bool>("airtime_points_display", showAirTimeDisplay);
    cfg.set<bool>("height_points_display", showHeightChart);
    cfg.set<bool>("inertia_points_display", showInertiaChart);

    cfg.set<int>("maxPerRow_points_display", maxPerRow);
    cfg.set<int>("maxRows_points_display", maxRows);

    for (const auto& combo : unlocked_combos) {
        std::string configKey = "combo_unlock_" + std::string(combo.name);
        cfg.set<bool>(configKey, combo.unlocked);
    }
}
