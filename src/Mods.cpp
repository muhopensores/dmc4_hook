#include "Mods.hpp"
#include "utility/Hash.hpp"
#include "utility/Locales.hpp"
#include "Config.hpp"
#include "mods/LocalizationManager.hpp"
// include mod headers here
#include "mods/AreaJump.hpp"
#include "mods/BackgroundRendering.hpp"
#include "mods/Borderless.hpp"
#include "mods/CameraSettings.hpp"
#include "mods/DmdBloodyPalace.hpp"
#include "mods/DmdLevelAi.hpp"
#include "mods/FrenziedBossAI.hpp"
#include "mods/DtEnemiesDontStun.hpp"
#include "mods/EasyJc.hpp"
#include "mods/EnemySlotting.hpp"
#include "mods/EnemySpawn.hpp"
#include "mods/FastStart.hpp"
#include "mods/ForceLucifer.hpp"
#include "mods/FpsLimit.hpp"
#include "mods/FreeJc.hpp"
#include "mods/FreezeEnemies.hpp"
#include "mods/HeightRestriction.hpp"
#include "mods/HpInOrbsDisplay.hpp"
#include "mods/InfAllHealth.hpp"
#include "mods/InfDT.hpp"
#include "mods/InfDreadnaught.hpp"
#include "mods/InfFaustCloak.hpp"
#include "mods/InfSkyStars.hpp"
#include "mods/InfTrickRange.hpp"
#include "mods/InfiniteTime.hpp"
#include "mods/InputStates.hpp"
#include "mods/KnockbackEdits.hpp"
#include "mods/LdkWithDmd.hpp"
#include "mods/LimitAdjust.hpp"
#include "mods/ManualTwosomeTime.hpp"
#include "mods/NoClip.hpp"
#include "mods/NoHBknockback.hpp"
#include "mods/OneHitKill.hpp"
#include "mods/PinTimer.hpp"
#include "mods/PlayerTracker.hpp"
#include "mods/RestoreMaxHp.hpp"
#include "mods/SelectiveCancels.hpp"
#include "mods/SkipWeapons.hpp"
#include "mods/TimerMem.hpp"
#include "mods/TrackingFullHouse.hpp"
#include "mods/TrickDown.hpp"
#include "mods/WorkRate.hpp"
#include "mods/BerialDaze.hpp"
#include "mods/BpPortal.hpp"
#include "mods/CharacterSwap.hpp"
#include "mods/CutsceneSkip.hpp"
#include "mods/DifficultySelect.hpp"
#include "mods/DisableCameraEvents.hpp"
#include "mods/DisableDarkslayer.hpp"
#include "mods/EnemyAttackOffScreen.hpp"
#include "mods/EnemyDT.hpp"
#include "mods/FastPandora.hpp"
#include "mods/FastSprint.hpp"
#include "mods/HideHud.hpp"
#include "mods/InfAirHikes.hpp"
#include "mods/InfRevive.hpp"
#include "mods/InfTableHopper.hpp"
#include "mods/RemoveLaunchArmour.hpp"
#include "mods/RoseRemovesPins.hpp" // taunt ecstasy relies on this
#include "mods/InfinitePinTimer.hpp"
#include "mods/SlowWalk.hpp"
#include "mods/StunAnything.hpp"
#include "mods/ActiveBlock.hpp"
#include "mods/BigHeadMode.hpp"
#include "mods/ChargeChecker.hpp"
#include "mods/DisableChimeraBlades.hpp"
#include "mods/EnemyReplace.hpp"
#include "mods/EnemyTracker.hpp"
#include "mods/FasterFastDrive.hpp"
#include "mods/FrostsDontJump.hpp"
#include "mods/InfCalibur.hpp"
#include "mods/InstantTrick.hpp"
#include "mods/LoadOrder.hpp"
#include "mods/Payline.hpp"
#include "mods/NeroSnatchLength.hpp"
#include "mods/NoAutomaticCharacters.hpp"
#include "mods/Quicksilver.hpp"
#include "mods/RgMultiplier.hpp"
#include "mods/RoomRespawn.hpp"
#include "mods/RoseOptions.hpp"
#include "mods/TurnSpeedEdits.hpp"
#include "mods/TrackingSkyStar.hpp"
#include "mods/DisableDTStinger.hpp"
#include "mods/HighTimeWeaponSwitch.hpp"
#include "mods/AlwaysRoyalRelease.hpp"
#include "mods/DisableKeyboard.hpp"
#include "mods/ShaderEditor.hpp"
#include "mods/MutatorHolyWater.hpp"
#include "mods/MutatorPlayerTransforms.hpp"
#include "mods/MutatorSuperhot.hpp"
#include "mods/Twitch.hpp"
#include "mods/MessageDisplay.hpp"
#include "mods/MutatorSelfAdvertisement.hpp"
#include "mods/DoubleTapDarkslayer.hpp"
#include "mods/TargetChangeDarkslayer.hpp"
#include "mods/CustomModelFix.hpp"
#include "mods/PsychoMantis.hpp"
#include "mods/GuardSteer.hpp"
#include "mods/PlayerRotation.hpp"
#include "mods/AssaultsDontJump.hpp"
#include "mods/DanteJdc.hpp"
#include "mods/RevFlying.hpp"
#include "mods/AngelSnatch.hpp"
#include "mods/DtKnuckle.hpp"
#include "mods/EffectColours.hpp"
#include "mods/CharSwitcher.hpp"
#include "mods/RisingSun.hpp"
#include "mods/AerialStinger.hpp"
#include "mods/AirMustang.hpp"
#include "mods/AerialDrive.hpp"
#include "mods/PinTrick.hpp"
#include "mods/LMTSlotFix.hpp"
#include "mods/MoveTable.hpp"
#include "mods/PinProperties.hpp"
#include "mods/LuciAirThrow.hpp"
#include "mods/CustomAllocatorMem.hpp"
// #include "mods/uPlayerParamsEdit.hpp"
#include "mods/PullKnockback.hpp" // no gui
#include "mods/SwordSpin.hpp"
#include "mods/CustomProjectile.hpp"
#include "mods/RedOrbCompletion.hpp"
#include "mods/TauntSelect.hpp"
#include "mods/StylePoints.hpp"
#include "mods/GuardTimer.hpp"
#include "mods/ExceedLimiter.hpp"
#include "mods/VisualizeHitbox.hpp"
#include "mods/NoLockonRestriction.hpp"
#include "mods/DebugCam.hpp"
#include "mods/NoclipCam.hpp"
#include "mods/CutscenePause.hpp"
#include "mods/AfterImage.hpp"
#include "mods/TrainerPause.hpp"
#include "mods/GunStingerDistance.hpp"
#include "mods/EnemyStepDisplay.hpp"
#include "mods/MultiCharFixes.hpp"
#include "mods/FastRoundTrip.hpp"
#include "mods/FlyingStats.hpp"
#include "mods/FireworksTweaks.hpp"
#include "mods/CrossGunCharge.hpp"
#include "mods/PandoraCharge.hpp"
#include "mods/TricksterDashDT.hpp"
#include "mods/TrackingJealousy.hpp"
#include "mods/LongerJealousy.hpp"
#include "mods/GhostWalls.hpp"
#include "mods/ShadowResolution.hpp"
#include "mods/PowerUpSystem.hpp"
#include "mods/SpawnedEnemiesAttack.hpp"
#include "mods/Survival.hpp"
#include "mods/ManualLuciferAim.hpp"
#include "mods/ReversableDraw.hpp"
#include "mods/InfFrostHeal.hpp"
#include "mods/MustangWalls.hpp"
#include "mods/ExceedDisplay.hpp"
#include "mods/RunWhileHolding.hpp"
#include "mods/GroundedGunCancels.hpp"
#include "mods/FloorIsLava.hpp"
#include "mods/JointDisplay.hpp"
#include "mods/ForceInCombat.hpp"
#include "mods/PhotoMode.hpp"
#include "mods/Coop.hpp"
#include "mods/StageCallback.hpp"
constexpr size_t MODS_NUM{ 256 };

#define ADD_MOD(name)                                  \
    do {                                               \
        m_mods.emplace_back(std::make_unique<name>()); \
        m_hash.emplace_back(#name##_hash);             \
    } while (0)

//#include "mods/Sample.hpp"
// mods constructor
Mods::Mods() {
    m_mods.reserve(MODS_NUM);
    m_hash.reserve(MODS_NUM);
    // slow mods
    ADD_MOD(MessageDisplayMod);
    ADD_MOD(BackgroundRendering);
    ADD_MOD(Borderless);
    ADD_MOD(TwitchClient);
    ADD_MOD(MutatorSelfAdvertisement);
    ADD_MOD(PinTimer);
    // regular mods
    ADD_MOD(RedOrbCompletion);
    ADD_MOD(PlayerTracker);
    ADD_MOD(LoadOrder);
    ADD_MOD(MoveTable); // before anything that uses it
    ADD_MOD(FastStart);
    ADD_MOD(AreaJump);
    ADD_MOD(LimitAdjust);
    ADD_MOD(NoHbKnockback);
    ADD_MOD(EasyJc);
    ADD_MOD(SelectiveCancels);
    ADD_MOD(WorkRate);
    ADD_MOD(InfDreadnaught);
    ADD_MOD(NoClip);
    ADD_MOD(SkipWeapons);
    ADD_MOD(RestoreMaxHp);
    ADD_MOD(InputStates);
    ADD_MOD(FpsLimit);
    ADD_MOD(InfSkyStars);
    ADD_MOD(ForceLucifer);
    ADD_MOD(LdkWithDmd);
    ADD_MOD(DmdBloodyPalace);
    ADD_MOD(DmdLevelAi);
    ADD_MOD(FrenziedBossAI);
    ADD_MOD(ManualTwosomeTime);
    ADD_MOD(HeightRestriction);
    ADD_MOD(InfFaustCloak);
    ADD_MOD(InfiniteTime);
    ADD_MOD(InfAllHealth);
    ADD_MOD(CameraSettings);
    ADD_MOD(FreezeEnemies);
    ADD_MOD(FreeJc);
    ADD_MOD(OneHitKill);
    ADD_MOD(EnemySpawn);
    ADD_MOD(KnockbackEdits);
    ADD_MOD(DtEnemiesDontStun);
    ADD_MOD(EnemySlotting);
    ADD_MOD(TrackingFullHouse);
    ADD_MOD(InfTrickRange);
    ADD_MOD(TimerMem);
    ADD_MOD(TrickDown);
    ADD_MOD(HpInOrbsDisplay);
    ADD_MOD(InfDT);
    ADD_MOD(BerialDaze);
    ADD_MOD(RoseRemovesPins);
    ADD_MOD(InfinitePinTimer);
    ADD_MOD(StunAnything);
    ADD_MOD(DisableCameraEvents);
    ADD_MOD(HideHud);
    ADD_MOD(EnemyDT);
    ADD_MOD(BpPortal);
    ADD_MOD(FastPandora);
    ADD_MOD(InfAirHikes);
    ADD_MOD(InfRevive);
    ADD_MOD(CharacterSwap);
    ADD_MOD(DisableDarkslayer);
    ADD_MOD(RemoveLaunchArmour);
    ADD_MOD(CutsceneSkip);
    ADD_MOD(FastSprint);
    ADD_MOD(InfTableHopper);
    ADD_MOD(EnemyAttackOffScreen);
    ADD_MOD(SlowWalk);
    ADD_MOD(DifficultySelect);
    ADD_MOD(EnemyReplace);
    ADD_MOD(NoAutomaticCharacters);
    ADD_MOD(DisableChimeraBlades);
    ADD_MOD(Quicksilver);
    ADD_MOD(MutatorHolyWater);
    ADD_MOD(BigHeadMode);
    // ADD_MOD(MutatorPlayerTransforms); // BROKEN
    ADD_MOD(MutatorSuperhot);
    ADD_MOD(InfCalibur);
    ADD_MOD(RgMultiplier);
    ADD_MOD(ActiveBlock);
    ADD_MOD(Payline);
    ADD_MOD(FrostsDontJump);
    ADD_MOD(NeroSnatchLength);
    ADD_MOD(ChargeChecker);
    ADD_MOD(EnemyTracker);
    ADD_MOD(FasterFastDrive);
    ADD_MOD(RoseOptions);
    ADD_MOD(TurnSpeedEdits);
    ADD_MOD(TrackingSkyStar);
    ADD_MOD(DisableDTStinger);
    ADD_MOD(HighTimeWeaponSwitch);
    ADD_MOD(AlwaysRoyalRelease);
    ADD_MOD(DisableKeyboard);
    ADD_MOD(InstantTrick);
    // ADD_MOD(ShaderEditor); // unfinished
    ADD_MOD(RoomRespawn);
    ADD_MOD(DoubleTapDarkslayer);
    ADD_MOD(TargetChangeDarkslayer);
    ADD_MOD(CustomModelFix);
    ADD_MOD(PsychoMantis);
    ADD_MOD(GuardSteer);
    ADD_MOD(PlayerRotation);
    ADD_MOD(AssaultsDontJump);
    ADD_MOD(DanteJdc);
    ADD_MOD(RevFlying);
    ADD_MOD(AngelSnatch);
    ADD_MOD(DtKnuckle);
    ADD_MOD(EffectColours);
    ADD_MOD(CharSwitcher);
    ADD_MOD(RisingSun);
    ADD_MOD(LocalizationManager);
    ADD_MOD(AerialStinger);
    // ADD_MOD(AirMustang); // Crashed after 2 mins, removing for now
    ADD_MOD(AerialDrive);
    ADD_MOD(PinTrick);
    ADD_MOD(LMTSlotFix);
    ADD_MOD(PinProperties);
    ADD_MOD(LuciAirThrow);
    // ADD_MOD(uPlayerParamsEdit);
    ADD_MOD(PullKnockback); // no gui
    ADD_MOD(SwordSpin);
    ADD_MOD(CustomProjectile);
    ADD_MOD(TauntSelect);
    ADD_MOD(StylePoints);
    ADD_MOD(GuardTimer);
    ADD_MOD(ExceedLimiter);
    ADD_MOD(VisualizeHitbox);
    ADD_MOD(NoLockonRestriction);
    ADD_MOD(DebugCam);
    ADD_MOD(NoclipCam);
    ADD_MOD(CutscenePause);
    ADD_MOD(AfterImage);
    ADD_MOD(TrainerPause);
    ADD_MOD(GunStingerDistance);
    ADD_MOD(EnemyStepDisplay);
    ADD_MOD(MultiCharFixes);
    ADD_MOD(FastRoundTrip);
    ADD_MOD(FlyingStats);
    ADD_MOD(FireworksTweaks);
    ADD_MOD(CrossGunCharge);
    ADD_MOD(PandoraCharge);
    ADD_MOD(TricksterDashDT);
    ADD_MOD(TrackingJealousy);
    ADD_MOD(LongerJealousy);
    ADD_MOD(GhostWalls);
    ADD_MOD(ShadowResolution);
    ADD_MOD(SpawnedEnemiesAttack); // before Survival
    ADD_MOD(Survival);
    ADD_MOD(ManualLuciferAim);
    ADD_MOD(ReversableDraw);
    ADD_MOD(InfFrostHeal);
    ADD_MOD(MustangWalls);
    ADD_MOD(ExceedDisplay);
    ADD_MOD(RunWhileHolding);
    ADD_MOD(GroundedGunCancels);
    ADD_MOD(FloorIsLava);
    ADD_MOD(JointDisplay);
    ADD_MOD(ForceInCombat);
    ADD_MOD(PhotoMode);
    ADD_MOD(Coop);
    ADD_MOD(StageCallback);
}

// Initializes mods, checks for errors
std::optional<std::string> Mods::on_initialize(Mod::ModType type) const {
    for (auto& mod : m_mods) {
        if (mod->get_mod_type() != type) {
            continue;
        }
        //spdlog::info("%s::onInitialize()\n", mod->getModName().data());

        if (auto e = mod->on_initialize(); e != std::nullopt) {
            // spdlog::info("%s::onInitialize() has failed: %s\n", mod->getModName().data(), *e);
            return e;
        }
    }

    utility::Config cfg{CONFIG_FILENAME};
    for (auto& mod : m_mods) {
        if (mod->get_mod_type() != type) {
            continue;
        }
        // spdlog::info("%s::onConfigLoad()\n", mod->getModName().data());
        mod->on_config_load(cfg);
        auto& hks = mod->m_hotkeys;
        if (hks.empty()) { continue; }
        for (const auto& key : hks) {
            key->on_config_load(cfg);
        }
    }

    if (type == Mod::ModType::SLOW) {
        ModFramework* framework = g_framework.get();
        framework->get_menu_key_struct()->on_config_load(cfg);
        framework->get_menu_button_struct()->on_config_load(cfg);
    }

    MutatorRegistry::inst().load_config(cfg);

    return std::nullopt;
}

void Mods::on_config_save() {

    spdlog::info("Saving config to file");

    utility::Config cfg{};

    for (auto& mod : m_mods) {
        spdlog::info("%s::onConfigSave()\n", mod->get_mod_name().data());
        mod->on_config_save(cfg);

        auto& hks = mod->m_hotkeys;
        if (hks.empty()) { continue; }
        for (const auto& key : hks) {
            key->on_config_save(cfg);
        }
    }

    ModFramework* framework = g_framework.get();
    framework->get_menu_key_struct()->on_config_save(cfg);
    framework->get_menu_button_struct()->on_config_save(cfg);
    

    MutatorRegistry::inst().save_config(cfg);

    if (!cfg.save(CONFIG_FILENAME)) {
        spdlog::info("Failed to save config");
        return;
    }

    spdlog::info("Saved config");
}

// Called when showing ui, currently only WorkRate uses this to freeze time.
void Mods::on_game_pause(bool toggle) {
    for (auto& mod : m_mods) {
        mod->on_game_pause(toggle);
    }
}

void Mods::on_stage_start() {
    for (auto& mod: m_mods)
        mod->on_stage_start();
}

void Mods::on_stage_end() {
    for (auto& mod : m_mods)
        mod->on_stage_end();
}


bool Mods::on_message(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param) {
    for (auto& mod : m_mods) {
        if (!mod->on_message(wnd, message, w_param, l_param)) {
            return false;
        }
    }
    return true;
}

// Called on every frame for mods that override this method
void Mods::on_frame(fmilliseconds& dt) {
    for (auto& mod : m_mods) {
        mod->on_frame(dt);
    }
}

// Called when drawing the gui
void Mods::on_draw_ui(uint32_t hash, int display) {
    auto itr = std::find(m_hash.begin(), m_hash.end(), hash);
    if (itr != m_hash.cend()) {
        m_mods[std::distance(m_hash.begin(), itr)]->on_gui_frame(display);
    }
    else {
        spdlog::error("Mods::onDrawUI not found in hashindex!");
        __debugbreak();
    }
}
#if 0
// this is terrible atm
void Mods::on_chat_command(const std::string& message) {
    std::size_t hash = std::hash<std::string>{}(message);
    spdlog::debug("[TwitchCommand] hash:%d message:%s", hash, message.c_str());
    for (auto& mod : m_mods) {
        mod->on_twitch_command(hash);
    }
}
#endif

void Mods::on_update_input(utility::Input& input) {
    if (g_framework->get_window_handle() != GetForegroundWindow()) {
        return;
    }
    if (input.went_down(VK_OEM_3, false) || input.went_down(VK_OEM_8, false)) { // for bri'ish, 3 is ' and 8 is `
        g_framework->m_draw_console = !g_framework->m_draw_console;
        if (g_framework->m_draw_console) {
            console->set_display_fraction(0.5f);
        }
        else {
            console->set_display_fraction(0.0f);
        }
        
    }
    for (auto& mod : m_mods) {
        mod->on_update_input(input);
    }
}

void Mods::on_hotkey_tab(utility::Input& input) {
    ImGui::Text(_("Keys will probably show up only on US keyboard layouts."));
    ImGui::Text(_("We use MapVirtualKeyA();"));
    ImGui::Text(_("Making sure dear imgui renders unicode properly is complicated"));
    ImGui::Text(_("please understand. ;_;"));

    ImGui::Spacing();

    auto& menu = g_framework->get_menu_key_struct();
    { menu->draw(input); ImGui::Separator(); }
    auto& menu_gamepad = g_framework->get_menu_button_struct();
    { menu_gamepad->draw(input); ImGui::Separator(); }

    for (auto& mod : m_mods) {
        // it was too late i realized some shit binds multiple hotkeys
        auto& hks = mod->m_hotkeys;
        if (hks.empty()) { continue; }
        for (const auto& key : hks) {
            key->draw(input); ImGui::Separator();
        }
    }
}
