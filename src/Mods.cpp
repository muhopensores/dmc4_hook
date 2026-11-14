#include "Mods.hpp"
#include "utility/Hash.hpp"
#include "utility/Locales.hpp"
#include "Config.hpp"
#include "Console.hpp"
// include mod headers in this file
#include "ModsHeaders.hpp"

constexpr size_t MODS_NUM{ 512 };

#define ADD_MOD(name)                                     \
    do {                                                  \
        m_mods.emplace_back(std::make_unique<name>());    \
        m_hash.emplace_back(#name##_hash);                \
        mod_index::id<name> = __COUNTER__ - base - 1;     \
    } while (0)

//#define CRASHHUNTING

//#include "mods/Sample.hpp"
// mods constructor
Mods::Mods() {
    static constexpr int base = __COUNTER__;
    m_mods.reserve(MODS_NUM);
    // slow mods
//#ifndef CRASHHUNTING
    ADD_MOD(MessageDisplayMod);
    ADD_MOD(BackgroundRendering);
    ADD_MOD(Borderless);
    ADD_MOD(TwitchClient);
    ADD_MOD(MutatorSelfAdvertisement);
//#endif
    // regular mods
    ADD_MOD(InfiniteTime);
    ADD_MOD(FastStart);
    ADD_MOD(HealthSettings);
    ADD_MOD(AreaJump);
    ADD_MOD(PinTimer);
    ADD_MOD(EnemySpawn);
    ADD_MOD(CutsceneSkip);
    ADD_MOD(RedOrbCompletion);
    ADD_MOD(PlayerTracker);
    ADD_MOD(LoadOrder);
#ifndef CRASHHUNTING
    ADD_MOD(MoveTable); // before anything that uses it
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
    ADD_MOD(CameraSettings);
    ADD_MOD(FreezeEnemies);
    ADD_MOD(FreeJc);
    ADD_MOD(KnockbackEdits);
    ADD_MOD(DtEnemiesDontStun);
    ADD_MOD(EnemySlotting);
    ADD_MOD(TrackingFullHouse);
    ADD_MOD(InfTrickRange);
    ADD_MOD(LiveMoveTableSwaps);
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
    ADD_MOD(EffectColours);
    ADD_MOD(CharSwitcher);
    ADD_MOD(RisingSun);
    ADD_MOD(LocalizationManager);
    ADD_MOD(AerialStinger);
    // ADD_MOD(AirMustang); // Crashed after 2 mins, removing for now
    ADD_MOD(BackForwardQuickDrive);
    ADD_MOD(AerialDrive); // after backforwardquickdrive
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
    ADD_MOD(TrainerPause);
    ADD_MOD(GunStingerDistance);
    ADD_MOD(AnimationOverlay);
    ADD_MOD(EnemyStepDisplay);
    ADD_MOD(MultiCharFixes);
    ADD_MOD(FastRoundTrip);
    ADD_MOD(FlyingStats);
    // ADD_MOD(FireworksTweaks); // Works but edits the shotty animation on every shotty shot
    ADD_MOD(CrossGunCharge);
    ADD_MOD(PandoraCharge);
    ADD_MOD(TricksterDashDT);
    ADD_MOD(TrackingJealousy);
    ADD_MOD(LongerJealousy);
    ADD_MOD(GhostWalls);
    ADD_MOD(GraphicsSettings);
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
    ADD_MOD(StageCallback);
    ADD_MOD(DarkSoulsStamina);
    ADD_MOD(TestMod);
    ADD_MOD(Macro);
    ADD_MOD(ArcadeMode);
    ADD_MOD(FpsFixes);
    ADD_MOD(WitchTime);
    ADD_MOD(GermanWord);
    ADD_MOD(EnvironmentalHazards);
    ADD_MOD(ExceedLock);
    ADD_MOD(BrainrotPalace);
#ifndef PUBLIC_BUILD
    ADD_MOD(AfterImage);
    ADD_MOD(EnemyAI);
    ADD_MOD(BossDante);
    ADD_MOD(SpiderTwerk);
    ADD_MOD(Coop);
    ADD_MOD(Cock);
    ADD_MOD(RayTest);
    ADD_MOD(ThirdPersonAim); // before WallRun: its pre-check tick turns the left mouse button into the gun press the wall run's matcher reads
    ADD_MOD(WallRun);
    ADD_MOD(NeroStyle);     // before Gerb: Gerb's rows take their style condition from it (config load order)
    ADD_MOD(Gerb);
    ADD_MOD(GuardianDevil); // after NeroStyle: its action sets are per style slot
    ADD_MOD(DanteAirDodge);
    ADD_MOD(FreeRide);      // before MoveDef: its command bit is taken before MoveDef picks free ones
    ADD_MOD(NeroGunslinger);    // before MoveDef, as FreeRide (its command bit); its row's style is NeroStyle's
    ADD_MOD(MoveDef);        // after the hand-coded moves: they install the shared command hook first
    ADD_MOD(ViewportWindow);
    ADD_MOD(ChainFix);       // the game's chains (coats, hair) at any frame rate; optional turbulence fix
    ADD_MOD(ModelExtension); // runtime chains (and attachments) on models, from nativePC\model_ext\*.xml
    ADD_MOD(FileTracker);    // debug: what the game has loaded (files, arcs) and what it loads and drops
    ADD_MOD(UnitPacks);      // stage units' files from their rooms' arcs into the cache, for spawning them anywhere
    ADD_MOD(PbrMaterials);   // GGX shading for Albam's PBR materials (the unused SPECULAR_RIM pixel shaders swapped)
    ADD_MOD(Shadows);        // stage shadow map size and live overrides of its settings (uShadow)
#endif
#endif
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
    bool res = true;
    for (auto& mod : m_mods) {
        if (!mod->on_message(wnd, message, w_param, l_param)) {
            res = false;
        }
    }
    return res;
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
