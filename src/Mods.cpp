#include "Mods.hpp"
#include "utility/Hash.hpp"
#include "Config.hpp"
// include mod headers here
#include "mods/AreaJump.hpp"
#include "mods/BackgroundRendering.hpp"
#include "mods/Borderless.hpp"
#include "mods/CameraSettings.hpp"
#include "mods/DamageMultiplier.hpp"
#include "mods/DmdBloodyPalace.hpp"
#include "mods/DmdLevelAi.hpp"
#include "mods/DtEnemiesDontStun.hpp"
#include "mods/EasyJc.hpp"
#include "mods/EnemySlotting.hpp"
#include "mods/EnemySpawn.hpp"
#include "mods/FastStart.hpp"
#include "mods/ForceLucifer.hpp"
#include "mods/FpsLimit.hpp"
#include "mods/FreeJc.hpp"
#include "mods/FreezeEnemies.hpp"
#include "mods/HeightRestrictionDante.hpp"
#include "mods/HeightRestrictionNero.hpp"
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
#include "mods/MoveIDs.hpp"
#include "mods/MoveIDsNero.hpp"
#include "mods/NoClip.hpp"
#include "mods/NoDtCooldown.hpp"
#include "mods/NoHBknockback.hpp"
#include "mods/OneHitKill.hpp"
#include "mods/PlayerTracker.hpp"
#include "mods/RestoreMaxHp.hpp"
#include "mods/SelectiveCancels.hpp"
#include "mods/SkipWeapons.hpp"
#include "mods/TimerMem.hpp"
#include "mods/TrackingFullHouse.hpp"
#include "mods/TrickDown.hpp"
#include "mods/WorkRate.hpp"
// #include "mods/InfPlayerHealth.hpp"
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
#include "mods/SlowWalk.hpp"
#include "mods/StunAnything.hpp"
//#include "mods/EnemyReplace.hpp"
#include "mods/ActiveBlock.hpp"
#include "mods/BigHeadMode.hpp"
#include "mods/BpJumpHook.hpp"
#include "mods/ChargeChecker.hpp"
#include "mods/DeltaTime.hpp"
#include "mods/DisableChimeraBlades.hpp"
#include "mods/EnemyReplaceAgain.hpp"
#include "mods/EnemyStats.hpp"
#include "mods/FasterFastDrive.hpp"
#include "mods/FrostsDontJump.hpp"
#include "mods/InfCalibur.hpp"
#include "mods/InstantTrick.hpp"
#include "mods/LoadOrder.hpp"
#include "mods/NeroFullHouse.hpp"
#include "mods/NeroSnatchLength.hpp"
#include "mods/NoAutomaticCharacters.hpp"
#include "mods/Quicksilver.hpp"
#include "mods/RgMultiplier.hpp"
#include "mods/RoomRespawn.hpp"
#include "mods/RoseOptions.hpp"
#include "mods/ShaderEditor.hpp"
#include "mods/TwCmdHolyWater.hpp"
#include "mods/TwCmdPlayerTransforms.hpp"
#include "mods/TwCmdSuperhot.hpp"
#include "mods/Twitch.hpp"
#include "mods/MessageDisplay.hpp"

#define ADD_MOD(name)                                  \
    do {                                               \
        m_mods.emplace_back(std::make_unique<name>()); \
        m_hash.emplace_back(#name##_hash);             \
    } while (0)

//#include "mods/Sample.hpp"
// mods constructor
Mods::Mods() {
    // slow mods
    ADD_MOD(MessageDisplayMod); // keep this one at the 0 index for overlay;
    ADD_MOD(BackgroundRendering);
    ADD_MOD(Borderless);
    ADD_MOD(TwitchClient);
    // regular mods
    // add mods here
    // m_mods["ModName"_hash] = std::make_unique<ModName>();
    ADD_MOD(FastStart);
    ADD_MOD(AreaJump);
    ADD_MOD(LimitAdjust);
    ADD_MOD(MoveIds);
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
    ADD_MOD(MoveIdsNero);
    ADD_MOD(InfSkyStars);
    ADD_MOD(ForceLucifer);
    ADD_MOD(LdkWithDmd);
    ADD_MOD(DmdBloodyPalace);
    ADD_MOD(DmdLevelAi);
    ADD_MOD(ManualTwosomeTime);
    ADD_MOD(HeightRestrictionDante);
    ADD_MOD(InfFaustCloak);
    ADD_MOD(HeightRestrictionNero);
    ADD_MOD(InfiniteTime);
    ADD_MOD(InfAllHealth);
    ADD_MOD(CameraSettings);
    ADD_MOD(FreezeEnemies);
    ADD_MOD(FreeJc);
    ADD_MOD(OneHitKill);
    ADD_MOD(EnemySpawn);
    ADD_MOD(KnockbackEdits);
    ADD_MOD(NoDtCooldown);
    ADD_MOD(DtEnemiesDontStun);
    ADD_MOD(EnemySlotting);
    ADD_MOD(PlayerTracker);
    ADD_MOD(TrackingFullHouse);
    ADD_MOD(InfTrickRange);
    ADD_MOD(TimerMem);
    ADD_MOD(TrickDown);
    ADD_MOD(DamageMultiplier);
    ADD_MOD(HpInOrbsDisplay);
    ADD_MOD(InfDT);
    //ADD_MOD(InfPlayerHealth);
    ADD_MOD(BerialDaze);
    ADD_MOD(RoseRemovesPins);
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
    //ADD_MOD(EnemyReplace);
    ADD_MOD(EnemyReplaceAgain);
    ADD_MOD(BpJumpHook);
    ADD_MOD(NoAutomaticCharacters);
    ADD_MOD(DisableChimeraBlades);
    ADD_MOD(Quicksilver);
    ADD_MOD(TwCmdHolyWater);
    ADD_MOD(BigHeadMode);
    ADD_MOD(TwCmdPlayerTransforms);
    ADD_MOD(ModTwCmdSuperhot);
    ADD_MOD(InfCalibur);
    ADD_MOD(RgMultiplier);
    ADD_MOD(DeltaTime);
    ADD_MOD(ActiveBlock);
    ADD_MOD(NeroFullHouse);
    ADD_MOD(FrostsDontJump);
    ADD_MOD(NeroSnatchLength);
    ADD_MOD(ChargeChecker);
    ADD_MOD(EnemyStats);
    ADD_MOD(FasterFastDrive);
    ADD_MOD(RoseOptions);
    ADD_MOD(InstantTrick);
    //ADD_MOD(ShaderEditor); // unfinished
    ADD_MOD(LoadOrder); // in debug tab for now until i figure out how to get rid of SEH exceptions
    ADD_MOD(RoomRespawn);
}

// Initializes mods, checks for errors
std::optional<std::string> Mods::on_initialize(Mod::ModType type) const {
    for (auto& mod : m_mods) {
        if (mod->get_mod_type() != type) {
            continue;
        }
        //spdlog::info("%s::onInitialize()\n", mod->getModName().data());

        if (auto e = mod->on_initialize(); e != std::nullopt) {
            //spdlog::info("%s::onInitialize() has failed: %s\n", mod->getModName().data(), *e);
            return e;
        }
    }

    utility::Config cfg{CONFIG_FILENAME};
    for (auto& mod : m_mods) {
        if (mod->get_mod_type() != type) {
            continue;
        }
        //spdlog::info("%s::onConfigLoad()\n", mod->getModName().data());
        mod->on_config_load(cfg);
        auto& hks = mod->m_hotkeys;
        if (hks.empty()) { continue; }
        for (const auto& key : hks) {
            key->on_config_load(cfg);
        }
    }

    if (type == Mod::ModType::SLOW) { g_framework->get_menu_key_struct()->on_config_load(cfg); }
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

    g_framework->get_menu_key_struct()->on_config_save(cfg);

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
void Mods::on_draw_ui(uint32_t hash) {
    std::vector<uint32_t>::iterator itr = std::find(m_hash.begin(), m_hash.end(), hash);
    if (itr != m_hash.cend()) {
        m_mods[std::distance(m_hash.begin(), itr)]->on_gui_frame();
    }
    else {
        spdlog::error("Mods::onDrawUI not found in hashindex!");
        __debugbreak();
    }
}

// this is terrible atm
void Mods::on_chat_command(const std::string& message) {
    std::size_t hash = std::hash<std::string>{}(message);
    spdlog::debug("[TwitchCommand] hash:%d message:%s", hash, message.c_str());
    for (auto& mod : m_mods) {
        mod->on_twitch_command(hash);
    }
}

void Mods::on_update_input(utility::Input& input) {
    for (auto& mod : m_mods) {
        mod->on_update_input(input);
    }
}

void Mods::on_draw_custom_imgui_window() {
    MessageDisplayMod* m = dynamic_cast<MessageDisplayMod*>(m_mods[0].get());
    m->custom_imgui_window();
}

void Mods::on_hotkey_tab(utility::Input& input)
{
    ImGui::Spacing();

    ImGui::Text("Keys will probably show up only on US keyboard layouts.");
    ImGui::Text("We use MapVirtualKeyA();");
    ImGui::Text("Making sure dear imgui renders unicode properly is complicated");
    ImGui::Text("please understand. ;_;");

    ImGui::Spacing();

    auto& menu = g_framework->get_menu_key_struct();
    if (menu) { menu->draw(input); ImGui::Separator(); }

    for (auto& mod : m_mods) {
        // it was too late i realized some shit binds multiple hotkeys
        auto& hks = mod->m_hotkeys;
        if (hks.empty()) { continue; }
        for (const auto& key : hks) {
            key->draw(input); ImGui::Separator();
        }
    }
}
