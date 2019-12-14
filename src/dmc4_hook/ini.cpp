#include "inih/cpp/INIReader.h"
#include "ini.h"
#include "bools.h"

void GetIni()
{
    // define ini options
    INIReader reader("dmc4hook.ini");
    checkStyleSwitch = reader.GetBoolean("player", "style_switch_limits_removed", true);
    checkWeaponSwitch = reader.GetBoolean("player", "sword_&_gun_switch_limits_removed", true);
    checkJcCooldown = reader.GetBoolean("player", "jc_limits_removed", true);
    checkMovingTargetChange = reader.GetBoolean("player", "target_change_limit_removed", true);
    checkHeightRestrictionDante = reader.GetBoolean("player", "dante_height_restriction_removed", true);
    checkHeightRestrictionNero = reader.GetBoolean("player", "nero_height_restriction_removed", true);
    checkInfiniteTime = reader.GetBoolean("system", "infinite_time", true);
    checkinfiniteAllHealth = reader.GetBoolean("practice", "infinite_health_all", true);
    checkDisableCameraEvents = reader.GetBoolean("system", "disable_camera_events", true);
    checkHideHUD = reader.GetBoolean("system", "hide_hud", true);
    checkFastPandora = reader.GetBoolean("player", "fast_pandora", true);
    checkInfiniteAirHike = reader.GetBoolean("player", "infinite_air_hike", true);
    checkEnemyNoDT = reader.GetBoolean("system", "enemy_no_DT", true);
    checkEnemyInstantDT = reader.GetBoolean("system", "enemy_instant_DT", true);
    checkBpPortalAutoOpen = reader.GetBoolean("system", "auto_open_doors_and_BP_portal", true);
    checkInfiniteDT = reader.GetBoolean("practice", "infinite_DT", true);
    checkInfinitePlayerHealth = reader.GetBoolean("practice", "infinite_player_health", true);
    checkBerialPractice = reader.GetBoolean("practice", "berial_practice", true);
    checkBossRush = reader.GetBoolean("system", "boss_rush_mode", true);
    checkDisableDarkslayerUp = reader.GetBoolean("practice", "disable_darkslayer_Dpad_up", true);
    checkDisableDarkslayerDown = reader.GetBoolean("practice", "disable_darkslayer_Dpad_down", true);
    checkDisableDarkslayerLeft = reader.GetBoolean("practice", "disable_darkslayer_Dpad_left", true);
    checkDisableDarkslayerRight = reader.GetBoolean("practice", "disable_darkslayer_Dpad_right", true);
    checkSelectiveCancels = reader.GetBoolean("player", "selective_cancels", true);
    checkEcstasyCancel = reader.GetBoolean("player", "ecstasy_cancel", true);
    checkArgumentCancel = reader.GetBoolean("player", "argument_cancel", true);
    checkKickThirteenCancel = reader.GetBoolean("player", "kick_13_cancel", true);
    checkSlashDimensionCancel = reader.GetBoolean("player", "slash_dimension_cancel", true);
    checkPropCancel = reader.GetBoolean("player", "prop_cancel", true);
    checkShockCancel = reader.GetBoolean("player", "shock_cancel", true);
    checkOmenCancel = reader.GetBoolean("player", "omen_cancel", true);
    checkStunAnything = reader.GetBoolean("practice", "stun_anything", true);
    checkRemoveLaunchArmour = reader.GetBoolean("practice", "remove_launch_armour", true);
    checkCharacterChange = reader.GetBoolean("system", "character_change", true);
    checkSprintFasterActivate = reader.GetBoolean("player", "faster_sprint_activation", true);
    checkEnemyAttackOffscreen = reader.GetBoolean("system", "enemies_attack_offscreen", true);
    checkSlowWalk = reader.GetBoolean("system", "enable_slow_walk", true);
    checkHideStyle = reader.GetBoolean("system", "hide_Style_meter_and_orbs", true);
    checkautoSkiptIntro = reader.GetBoolean("system", "auto_skip_mission_intro", true);
    checkLDKWithDMD = reader.GetBoolean("system", "ldk_on_DMD_diff", true);
    checkInfiniteRevive = reader.GetBoolean("practice", "infinite_revive", true);
    checkSetStyle = reader.GetBoolean("config", "set_style", true);
    checkAutoSkipOutro = reader.GetBoolean("system", "auto_skip_mission_outros", true);
    checkInfiniteTableHopper = reader.GetBoolean("player", "infinite_table_hopper", true);
    checkTrackingFullHouse = reader.GetBoolean("player", "tracking_full_house", true);
    checkTrickDown = reader.GetBoolean("player", "trick_down", true);
    checkHoneyComb = reader.GetBoolean("player", "instant_honeycomb", true);
    checkInfiniteTrickRange = reader.GetBoolean("player", "infinite_trick_range", true);
    checkCameraSensitivity = reader.GetBoolean("system", "increased_camera_sensitivity", true);
    checkOrbDisplay = reader.GetBoolean("system", "enemy_hp_red_orb_display", true);
    checkRoseRemovesPins = reader.GetBoolean("player", "rose_removes_pins", true);

    // checking for ini
    if (reader.ParseError() < 0)
    {
        m_con.printf("Can't load 'dmc4hook.ini'. Using default settings\n");
    }
}
