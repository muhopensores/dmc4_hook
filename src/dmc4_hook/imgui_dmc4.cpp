#include "main.h"
#include "gui_functions.h"
#include "hooks.h"
#include "naked.h"
#include "imgui_dmc4.h"
#include "addresses.h"
#include "patches.h"
#include "bools.h"

// initialization bool
bool g_bWasInitialized = true;

// function to render the gui onto screen
void RenderImgui(IDirect3DDevice9* m_pDevice)
{
    auto main = GetMain(); // get ptr to hacklib main
    if (g_bWasInitialized)
    {
        g_bWasInitialized = false;
        ToggleStuff();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        ImGui_ImplWin32_Init(FindWindowA(NULL, windowName));
        ImGui_ImplDX9_Init(m_pDevice);
    }

    DarkTheme();
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    DrawWindow();

    // specific imgui functions, can be looked up in examples or the documentation
    // references/ points to other functions to apply logic behind the gui toggles/ objects
    {
        BeginDrawing();
        ImGui::SameLine(0, 0);
        FPSDrawing();

        if (ImGui::BeginTabBar("Trainer", ImGuiTabBarFlags_FittingPolicyScroll))
        {
            if (ImGui::BeginTabItem("Player"))
            {
                ImGui::Spacing();
                ImGui::Text("Limit Removal");

                if (ImGui::Checkbox("Remove Sword & Gun Switch Limit", &checkWeaponSwitch))
                {
                    ToggleWeaponSwitch(checkWeaponSwitch);
                }

                if (ImGui::Checkbox("Remove JC Limit", &checkJcCooldown))
                {
                    ToggleJcCooldown(checkJcCooldown);
                }

                if (ImGui::Checkbox("Remove Style Switch Limit", &checkStyleSwitch))
                {
                    ToggleStyleSwitch(checkStyleSwitch);
                }

                if (ImGui::Checkbox("Remove Target Switch Limit", &checkMovingTargetChange))
                {
                    ToggleMovingTargetChange(checkMovingTargetChange);
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                (ImGui::Text("Height Restriction Removal"));

                if (ImGui::Checkbox("Dante", &checkHeightRestrictionDante))
                {
                    ToggleHeightRestrictionDante(checkHeightRestrictionDante);
                }

                ImGui::SameLine(198);

                if (ImGui::Checkbox("Nero", &checkHeightRestrictionNero))
                {
                    ToggleHeightRestrictionNero(checkHeightRestrictionNero);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Checkbox("Player Damage Modifier", &checkDamageModifier))
                {
                    ImGuiToggleDamageModifier();
                }
                ImGui::InputFloat("Multiplier", &damagemultiplier, 0.1f, 1.0f, "%.1f");

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                Misc();

                if (ImGui::Checkbox("Infinite Air Hike", &checkInfiniteAirHike))
                {
                    ToggleInfiniteAirHike(checkInfiniteAirHike);
                }

                ImGui::SameLine(198);

                if (ImGui::Checkbox("Infinite Table Hopper", &checkInfiniteTableHopper))
                {
                    ToggleInfiniteTableHopper(checkInfiniteTableHopper);
                }

                if (ImGui::Checkbox("Trick Down", &checkTrickDown))
                {
                    ImGuiToggleTrickDown();
                }

                ImGui::SameLine(198);

                if (ImGui::Checkbox("Infinite Trick Range", &checkInfiniteTrickRange))
                {
                    ToggleInfiniteTrickRange(checkInfiniteTrickRange);
                }

                if (ImGui::Checkbox("Tracking Full House", &checkTrackingFullHouse))
                {
                    ImGuiToggleTrackingFullHouse();
                }

                ImGui::SameLine(198);

                if (ImGui::Checkbox("Instant Honeycomb", &checkHoneyComb))
                {
                    ImGuiToggleHoneyComb();
                }

                if (ImGui::Checkbox("Fast Pandora", &checkFastPandora))
                {
                    ToggleFastPandora(checkFastPandora);
                }

                ImGui::SameLine(198);

                if (ImGui::Checkbox("Fast Speed", &checkSprintFasterActivate))
                {
                    ToggleSprintFasterActivate(checkSprintFasterActivate);
                }

                if (ImGui::Checkbox("Rose Removes Pins", &checkRoseRemovesPins))
                {
                    ImGuiToggleRoseRemovesPins();
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Spacing();

                if (ImGui::CollapsingHeader("Selective Cancels"))
                {
                    if (ImGui::Checkbox("Enable", &checkSelectiveCancels))
                    {
                        ImGuiToggleSelectiveCancels();
                    }

                    ImGui::Separator();
                    ImGui::Text("Common");

                    if (ImGui::Checkbox("Ecstasy", &checkEcstasyCancel))
                    {
                        ImGuiToggleEcstasyCancel();
                    }

                    ImGui::SameLine(198);

                    if (ImGui::Checkbox("Argument", &checkArgumentCancel))
                    {
                        ImGuiToggleArgumentCancel();
                    }

                    if (ImGui::Checkbox("Kick 13", &checkKickThirteenCancel))
                    {
                        ImGuiToggleKickThirteenCancel();
                    }

                    ImGui::SameLine(198);

                    if (ImGui::Checkbox("Slash Dimension", &checkSlashDimensionCancel))
                    {
                        ImGuiToggleSlashDimensionCancel();
                    }

                    ImGui::Separator();
                    ImGui::Text("Swords");

                    if (ImGui::Checkbox("Prop", &checkPropCancel))
                    {
                        ImGuiTogglePropCancel();
                    }

                    ImGui::SameLine(198);

                    if (ImGui::Checkbox("Shock", &checkShockCancel))
                    {
                        ImGuiToggleShockCancel();
                    }

                    ImGui::Separator();
                    ImGui::Text("Guns");

                    if (ImGui::Checkbox("Omen", &checkOmenCancel))
                    {
                        ImGuiToggleOmenCancel();
                    }
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("System"))
            {
                ImGui::Spacing();
                ImGui::Text("General");

                if (ImGui::Checkbox("Disable Timer", &checkInfiniteTime))
                {
                    ToggleInfiniteTime(checkInfiniteTime);
                }

                ImGui::SameLine(205);

                if (ImGui::Checkbox("Disable Camera Events", &checkDisableCameraEvents))
                {
                    ToggleDisableCameraEvents(checkDisableCameraEvents);
                }

                ImGui::SameLine(0, 1);
                HelpMarker("Certain missions or parts will cause a black screen when this option is enabled. Press the "
                           "start button and then X/A to go into a menu. The screen should return to normal.");

                if (ImGui::Checkbox("Skip Mission Intros", &checkautoSkiptIntro))
                {
                    ToggleAutoSkipIntro(checkautoSkiptIntro);
                }

                ImGui::SameLine(205);

                if (ImGui::Checkbox("Skip Mission Outros", &checkAutoSkipOutro))
                {
                    ToggleAutoSkipOutro(checkAutoSkipOutro);
                }

                if (ImGui::Checkbox("Hide HUD", &checkHideHUD))
                {
                    checkHideStyle = false;
                    ToggleHideHUD(checkHideHUD);
                }

                ImGui::SameLine(205);

                if (ImGui::Checkbox("Hide Timer,Style & Orbs", &checkHideStyle))
                {
                    checkHideHUD = false;
                    ToggleHideStyle(checkHideStyle);
                }

                if (ImGui::Checkbox("Character Change", &checkCharacterChange))
                {
                    ToggleCharacterChange(checkCharacterChange);
                }

                ImGui::SameLine(205);

                if (ImGui::Checkbox("Slow Walk", &checkSlowWalk))
                {
                    ToggleSlowWalk(checkSlowWalk);
                }

                ImGui::SameLine(0, 1);

                HelpMarker("Press & hold the jump button to walk slowly");

                if (ImGui::Checkbox("Auto Open Sealed Doors & Portals", &checkBpPortalAutoOpen))
                {
                    ToggleBpPortalAutoOpen(checkBpPortalAutoOpen);
                }

                if (ImGui::Checkbox("Enemy HP Red Orb Display", &checkOrbDisplay))
                {
                    ImGuiToggleOrbDisplay();
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Text("Game Mode");

                static int diff_index = 0;
                if (ImGui::Combo("Difficulty", &diff_index, "Default\0Dante Must Die\0God Must Die\0"))
                {
                    switch (diff_index)
                    {
                    case 0:
                        SetDefault();
                        break;
                    case 1:
                        SetDMD();
                        break;
                    case 2:
                        SetGMD();
                        break;
                    }
                }

                ImGui::Spacing();
                ImGui::Spacing();

                if (ImGui::Checkbox("Boss Rush Mode", &checkBossRush))
                {
                    ToggleBossRush(checkBossRush);
                }

                ImGui::SameLine(0, 1);
                HelpMarker("Toggle and start BP run");
                ImGui::SameLine(205);

                if (ImGui::Checkbox("LDK on DMD Difficulty", &checkLDKWithDMD))
                {
                    ToggleLDKWithDMD(checkLDKWithDMD);
                    ImGuiToggleLDKWithDMD();
                }

                ImGui::SameLine(0, 1);
                HelpMarker("Start a Mission on Legendary Dark Knight. Enemies are on DMD Difficulty, instead of SOS");

                if (ImGui::Checkbox("Enemies DT Instantly", &checkEnemyInstantDT))
                {
                    ToggleEnemyInstantDT(checkEnemyInstantDT);
                }

                ImGui::SameLine(205);

                if (ImGui::Checkbox("Enemies Don't DT", &checkEnemyNoDT))
                {
                    ToggleEnemyNoDT(checkEnemyNoDT);
                }

                if (ImGui::Checkbox("Enemies Attack Off-Screen", &checkEnemyAttackOffscreen))
                {
                    ToggleEnemyAttackOffscreen(checkEnemyAttackOffscreen);
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                Misc();

                if (ImGui::CollapsingHeader("Speed"))
                {
                    ImGui::InputFloat("Turbo Value", turboValue, 0.1f, 0.5f, "%.1f%");
                    ImGui::Spacing();
                    ImGui::SliderFloat("Global Speed", globalSpeed, 0.0f, 3.0f, "%.1f");
                    ImGui::Spacing();
                    ImGui::SliderFloat("Room Speed", roomSpeed, 0.0f, 3.0f, "%.1f");
                    ImGui::Spacing();
                    ImGui::SliderFloat("Player Speed", playerSpeed, 0.0f, 3.0f, "%.1f");
                    ImGui::Spacing();
                    ImGui::SliderFloat("Enemy Speed", enemySpeed, 0.0f, 3.0f, "%.1f");
                }

                ImGui::Spacing();

                if (ImGui::CollapsingHeader("Camera"))
                {
                    ImGui::InputFloat("Camera Height", &cameraHeight, 1.0f, 10.0f, "%.0f%");
                    ImGui::SameLine(0, 1);
                    HelpMarker("Value between -10000 / 10000. Default value is 170");
                    ImGui::Spacing();
                    ImGui::InputFloat("Camera Distance", &cameraDistance, 1.0f, 10.0f, "%.0f%");
                    ImGui::SameLine(0, 1);
                    HelpMarker("Value between 0 / 50000. Default value is 550");
                    ImGui::Spacing();
                    ImGui::InputFloat("Camera Distance \n(Lockon)", &cameraDistanceLockon, 1.0f, 10.0f, "%.0f%");
                    ImGui::SameLine(0, 1);
                    HelpMarker("Value between 0 / 50000. Default value is 520");
                    ImGui::Spacing();
                    ImGui::InputFloat("Camera Angle", &cameraAngle, 0.1f, 0.5f, "%.1f%");
                    ImGui::SameLine(0, 1);
                    HelpMarker("Value between -1.5 / 1.5. Default value is 0.2");
                    ImGui::Spacing();

                    if (ImGui::Checkbox("Increased Camera Sensitivity", &checkCameraSensitivity))
                    {
                        ToggleCameraSensitivity(checkCameraSensitivity);
                    }
                }

                ImGui::Spacing();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Environment"))
            {
                ImGui::Spacing();
                ImGui::Text("Area Jump");
                ImGui::Spacing();
                ImGui::InputInt("BP Floor\n(no boss stages)", bpFloorStage, 1, 10,
                                ImGuiInputTextFlags_AllowTabInput);
                ImGui::SameLine(0, 1);
                HelpMarker("Type in the BP floor you want to teleport to and choose the correct BP area below (example "
                           "21-39). Only "
                           "1-99, no boss stages. For boss stages simply select the boss room from the listbox below.");
                ImGui::Spacing();

                const char* room_items[] = {
                    "Bloody Palace 1-19",                 // 705
                    "Bloody Palace 21-39",                // 704
                    "Bloody Palace 41-59",                // 703
                    "Bloody Palace 61-79",                // 701
                    "Bloody Palace 81-99",                // 702
                    "Bloody Palace 20",                   // 503
                    "Bloody Palace 40",                   // 504
                    "Bloody Palace 60",                   // 505
                    "Bloody Palace 80",                   // 502
                    "Bloody Palace 100",                  // 501
                    "Bloody Palace 101",                  // 700
                    "Opera House",                        // 0
                    "Opera House Plaza",                  // 1
                    "Storehouse",                         // 2
                    "Cathedral",                          // 3
                    "Terrace / Business District",        // 4
                    "Residential District",               // 5
                    "Port Caerula",                       // 6
                    "Customs House",                      // 7
                    "First Mining Area",                  // 8
                    "Ferrum Hills",                       // 9
                    "M17 Opera House",                    // 10
                    "M17 Opera House Plaza",              // 11
                    "Business District / Terrace",        // 12
                    "M20 Opera House Plaza",              // 13
                    "Second Mining Area",                 // 100
                    "Fortuna Castle Gate",                // 105
                    "Grand Hall (Fortuna Castle)",        // 200
                    "Large Hall",                         // 201
                    "Dining Room",                        // 202
                    "Torture Chamber",                    // 203
                    "Central Courtyard",                  // 204
                    "Foris Falls (Bridge Area)",          // 205
                    "Gallery",                            // 206
                    "Library",                            // 207
                    "Soldier's Graveyard",                // 209
                    "Master's Chamber",                   // 210
                    "Spiral Well",                        // 211
                    "Underground Laboratory",             // 212
                    "R&D Access",                         // 213
                    "Game Room",                          // 214
                    "Containment Room",                   // 215
                    "Angel Creation",                     // 216
                    "Foris Falls (Detour Area)",          // 217
                    "Forest Entrance",                    // 300
                    "Windswept Valley",                   // 301
                    "Ruined Church",                      // 302
                    "Ruined Valley",                      // 303
                    "Ancient Training Ground",            // 304
                    "Lapis River",                        // 305
                    "Ancient Plaza",                      // 306
                    "Den of the She-Viper",               // 307
                    "Forgotten Ruins",                    // 308
                    "Hidden Pit",                         // 309
                    "Ruined Lowlands",                    // 310
                    "Lost Woods",                         // 311
                    "Gran Album Bridge",                  // 400
                    "Grand Hall (Order of the Sword HQ)", // 401
                    "Key Chamber",                        // 402
                    "The Gauntlet",                       // 403
                    "Agnus' Room",                        // 404
                    "Security Corridor",                  // 405
                    "Experiment Disposal",                // 406
                    "Meeting Room",                       // 407
                    "Ascension Chamber",                  // 408
                    "Advent Chamber",                     // 409
                    "Machina Ex Deus",                    // 500
                    "Stairway to Heaven",                 // 501
                    "Sacred Heart",                       // 502
                    "M18",                                // 510
                    "Sky Above Fortuna",                  // 512
                    "Secret Mission 1",                   // 800
                    "Secret Mission 2",                   // 801
                    "Secret Mission 3",                   // 802
                    "Secret Mission 4",                   // 803
                    "Secret Mission 5",                   // 804
                    "Secret Mission 6",                   // 805
                    "Secret Mission 7",                   // 806
                    "Secret Mission 8",                   // 807
                    "Secret Mission 9",                   // 808
                    "Secret Mission 10",                  // 809
                    "Secret Mission 11",                  // 810
                    "Secret Mission 12"                   // 811
                };

                int room_item_current = 0;
                if (ImGui::ListBox("Room Codes\n(including BP)", &room_item_current, room_items,
                                   IM_ARRAYSIZE(room_items), 10))
                {
                    ImGuiStageJump(room_item_current);
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::CollapsingHeader("Enemy Replacement"))
                {
                    ImGui::Spacing();
                    ImGui::Text("No Cyclical Replacements!");
                    ImGui::SameLine(0, 1);
                    HelpMarker("Example: Don't replace Mephisto with Faust and Faust with Mephisto at the same time. "
                               "The game will get stuck in a loop and cause undesired behaviour.");
                    ImGui::Spacing();
                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Scarecrow(Leg) with: "))
                    {
                        const char* scarecrowLeg_items[] = {
                            "Default", "Scarecrow(Arm)", "Mega Scarecrow", "Bianco Angelo", "Alto Angelo",  "Mephisto",
                            "Faust",   "Frost",          "Assault",        "Blitz",         "Chimera Seed", "Basilisk",
                            "Berial",  "Credo",          "Agnus",          "Sanctus",       "Dante",
                        };
                        static int scarecrowLeg_current = 0;
                        if (ImGui::Combo("Replace SC(L)", &scarecrowLeg_current, scarecrowLeg_items,
                                         IM_ARRAYSIZE(scarecrowLeg_items)))
                        {
                            switch (scarecrowLeg_current)
                            {
                            case 0:
                                ReplaceEnemyDefault(replaceScarecrowLeg);
                                break;
                            case 1:
                                ReplaceEnemyOne(replaceScarecrowLeg);
                                break;
                            case 2:
                                ReplaceEnemyTwo(replaceScarecrowLeg);
                                break;
                            case 3:
                                ReplaceEnemyThree(replaceScarecrowLeg);
                                break;
                            case 4:
                                ReplaceEnemyFour(replaceScarecrowLeg);
                                break;
                            case 5:
                                ReplaceEnemyFive(replaceScarecrowLeg);
                                break;
                            case 6:
                                ReplaceEnemySix(replaceScarecrowLeg);
                                break;
                            case 7:
                                ReplaceEnemySeven(replaceScarecrowLeg);
                                break;
                            case 8:
                                ReplaceEnemyEight(replaceScarecrowLeg);
                                break;
                            case 9:
                                ReplaceEnemyNine(replaceScarecrowLeg);
                                break;
                            case 10:
                                ReplaceEnemyTen(replaceScarecrowLeg);
                                break;
                            case 11:
                                ReplaceEnemyEleven(replaceScarecrowLeg);
                                break;
                            case 12:
                                ReplaceEnemyTwelve(replaceScarecrowLeg);
                                break;
                            case 13:
                                ReplaceEnemyThirteen(replaceScarecrowLeg);
                                break;
                            case 14:
                                ReplaceEnemyFourteen(replaceScarecrowLeg);
                                break;
                            case 15:
                                ReplaceEnemyFifteen(replaceScarecrowLeg);
                                break;
                            case 16:
                                ReplaceEnemySixteen(replaceScarecrowLeg);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Scarecrow(Arm) with: "))
                    {
                        const char* scarecrowArm_items[] = {
                            "Default", "Scarecrow(Leg)", "Mega Scarecrow", "Bianco Angelo", "Alto Angelo",  "Mephisto",
                            "Faust",   "Frost",          "Assault",        "Blitz",         "Chimera Seed", "Basilisk",
                            "Berial",  "Credo",          "Agnus",          "Sanctus",       "Dante"
                        };
                        static int scarecrowArm_current = 0;
                        if (ImGui::Combo("Replace SC(A)", &scarecrowArm_current, scarecrowArm_items,
                                         IM_ARRAYSIZE(scarecrowArm_items)))
                        {
                            switch (scarecrowArm_current)
                            {
                            case 0:
                                ReplaceEnemyDefault(replaceScarecrowArm);
                                break;
                            case 1:
                                ReplaceEnemyOne(replaceScarecrowArm);
                                break;
                            case 2:
                                ReplaceEnemyTwo(replaceScarecrowArm);
                                break;
                            case 3:
                                ReplaceEnemyThree(replaceScarecrowArm);
                                break;
                            case 4:
                                ReplaceEnemyFour(replaceScarecrowArm);
                                break;
                            case 5:
                                ReplaceEnemyFive(replaceScarecrowArm);
                                break;
                            case 6:
                                ReplaceEnemySix(replaceScarecrowArm);
                                break;
                            case 7:
                                ReplaceEnemySeven(replaceScarecrowArm);
                                break;
                            case 8:
                                ReplaceEnemyEight(replaceScarecrowArm);
                                break;
                            case 9:
                                ReplaceEnemyNine(replaceScarecrowArm);
                                break;
                            case 10:
                                ReplaceEnemyTen(replaceScarecrowArm);
                                break;
                            case 11:
                                ReplaceEnemyEleven(replaceScarecrowArm);
                                break;
                            case 12:
                                ReplaceEnemyTwelve(replaceScarecrowArm);
                                break;
                            case 13:
                                ReplaceEnemyThirteen(replaceScarecrowArm);
                                break;
                            case 14:
                                ReplaceEnemyFourteen(replaceScarecrowArm);
                                break;
                            case 15:
                                ReplaceEnemyFifteen(replaceScarecrowArm);
                                break;
                            case 16:
                                ReplaceEnemySixteen(replaceScarecrowArm);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Mega Scarecrow with: "))
                    {
                        const char* megaScarecrow_items[] = {
                            "Default",      "Scarecrow(Leg)", "Scarecrow(Arm)", "Bianco Angelo", "Alto Angelo",
                            "Mephisto",     "Faust",          "Frost",          "Assault",       "Blitz",
                            "Chimera Seed", "Basilisk",       "Berial",         "Bael",          "Echidna",
                            "Credo",        "Agnus",          "Sanctus",        "Kyrie",         "Dante"
                        };
                        static int megaScarecrow_current = 0;
                        if (ImGui::Combo("Replace MSC", &megaScarecrow_current, megaScarecrow_items,
                                         IM_ARRAYSIZE(megaScarecrow_items)))
                        {
                            switch (megaScarecrow_current)
                            {
                            case 0:
                                ReplaceEnemyDefault(replaceMegaScarecrow);
                                break;
                            case 1:
                                ReplaceEnemyOne(replaceMegaScarecrow);
                                break;
                            case 2:
                                ReplaceEnemyTwo(replaceMegaScarecrow);
                                break;
                            case 3:
                                ReplaceEnemyThree(replaceMegaScarecrow);
                                break;
                            case 4:
                                ReplaceEnemyFour(replaceMegaScarecrow);
                                break;
                            case 5:
                                ReplaceEnemyFive(replaceMegaScarecrow);
                                break;
                            case 6:
                                ReplaceEnemySix(replaceMegaScarecrow);
                                break;
                            case 7:
                                ReplaceEnemySeven(replaceMegaScarecrow);
                                break;
                            case 8:
                                ReplaceEnemyEight(replaceMegaScarecrow);
                                break;
                            case 9:
                                ReplaceEnemyNine(replaceMegaScarecrow);
                                break;
                            case 10:
                                ReplaceEnemyTen(replaceMegaScarecrow);
                                break;
                            case 11:
                                ReplaceEnemyEleven(replaceMegaScarecrow);
                                break;
                            case 12:
                                ReplaceEnemyTwelve(replaceMegaScarecrow);
                                break;
                            case 13:
                                ReplaceEnemyThirteen(replaceMegaScarecrow);
                                break;
                            case 14:
                                ReplaceEnemyFourteen(replaceMegaScarecrow);
                                break;
                            case 15:
                                ReplaceEnemyFifteen(replaceMegaScarecrow);
                                break;
                            case 16:
                                ReplaceEnemySixteen(replaceMegaScarecrow);
                                break;
                            case 17:
                                ReplaceEnemySeventeeen(replaceMegaScarecrow);
                                break;
                            case 18:
                                ReplaceEnemyEighteen(replaceMegaScarecrow);
                                break;
                            case 19:
                                ReplaceEnemyNineteen(replaceMegaScarecrow);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Bianco Angelo with: "))
                    {
                        const char* bianco_items[] = {
                            "Default", "Scarecrow(Leg)", "Scarecrow(Arm)", "Mega Scarecrow", "Alto Angelo",  "Mephisto",
                            "Faust",   "Frost",          "Assault",        "Blitz",          "Chimera Seed", "Basilisk",
                            "Berial",  "Credo",          "Agnus",          "Sanctus",        "Dante"
                        };
                        static int bianco_current = 0;
                        if (ImGui::Combo("Replace Bianco", &bianco_current, bianco_items, IM_ARRAYSIZE(bianco_items)))
                        {
                            switch (bianco_current)
                            {
                            case 0:
                                ReplaceEnemyDefault(replaceBiancoAngelo);
                                break;
                            case 1:
                                ReplaceEnemyOne(replaceBiancoAngelo);
                                break;
                            case 2:
                                ReplaceEnemyTwo(replaceBiancoAngelo);
                                break;
                            case 3:
                                ReplaceEnemyThree(replaceBiancoAngelo);
                                break;
                            case 4:
                                ReplaceEnemyFour(replaceBiancoAngelo);
                                break;
                            case 5:
                                ReplaceEnemyFive(replaceBiancoAngelo);
                                break;
                            case 6:
                                ReplaceEnemySix(replaceBiancoAngelo);
                                break;
                            case 7:
                                ReplaceEnemySeven(replaceBiancoAngelo);
                                break;
                            case 8:
                                ReplaceEnemyEight(replaceBiancoAngelo);
                                break;
                            case 9:
                                ReplaceEnemyNine(replaceBiancoAngelo);
                                break;
                            case 10:
                                ReplaceEnemyTen(replaceBiancoAngelo);
                                break;
                            case 11:
                                ReplaceEnemyEleven(replaceBiancoAngelo);
                                break;
                            case 12:
                                ReplaceEnemyTwelve(replaceBiancoAngelo);
                                break;
                            case 13:
                                ReplaceEnemyThirteen(replaceBiancoAngelo);
                                break;
                            case 14:
                                ReplaceEnemyFourteen(replaceBiancoAngelo);
                                break;
                            case 15:
                                ReplaceEnemyFifteen(replaceBiancoAngelo);
                                break;
                            case 16:
                                ReplaceEnemySixteen(replaceBiancoAngelo);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Alto Angelo with: "))
                    {
                        const char* alto_items[] = {
                            "Default",      "Scarecrow(Leg)", "Scarecrow(Arm)", "Mega Scarecrow", "Bianco Angelo",
                            "Mephisto",     "Faust",          "Frost",          "Assault",        "Blitz",
                            "Chimera Seed", "Basilisk",       "Berial",         "Bael",           "Echidna",
                            "Credo",        "Agnus",          "Sanctus",        "Kyrie",          "Dante"
                        };
                        static int alto_current = 0;
                        if (ImGui::Combo("Replace Alto", &alto_current, alto_items, IM_ARRAYSIZE(alto_items)))
                        {
                            switch (alto_current)
                            {
                            case 0:
                                ReplaceEnemyDefault(replaceAltoAngelo);
                                break;
                            case 1:
                                ReplaceEnemyOne(replaceAltoAngelo);
                                break;
                            case 2:
                                ReplaceEnemyTwo(replaceAltoAngelo);
                                break;
                            case 3:
                                ReplaceEnemyThree(replaceAltoAngelo);
                                break;
                            case 4:
                                ReplaceEnemyFour(replaceAltoAngelo);
                                break;
                            case 5:
                                ReplaceEnemyFive(replaceAltoAngelo);
                                break;
                            case 6:
                                ReplaceEnemySix(replaceAltoAngelo);
                                break;
                            case 7:
                                ReplaceEnemySeven(replaceAltoAngelo);
                                break;
                            case 8:
                                ReplaceEnemyEight(replaceAltoAngelo);
                                break;
                            case 9:
                                ReplaceEnemyNine(replaceAltoAngelo);
                                break;
                            case 10:
                                ReplaceEnemyTen(replaceAltoAngelo);
                                break;
                            case 11:
                                ReplaceEnemyEleven(replaceAltoAngelo);
                                break;
                            case 12:
                                ReplaceEnemyTwelve(replaceAltoAngelo);
                                break;
                            case 13:
                                ReplaceEnemyThirteen(replaceAltoAngelo);
                                break;
                            case 14:
                                ReplaceEnemyFourteen(replaceAltoAngelo);
                                break;
                            case 15:
                                ReplaceEnemyFifteen(replaceAltoAngelo);
                                break;
                            case 16:
                                ReplaceEnemySixteen(replaceAltoAngelo);
                                break;
                            case 17:
                                ReplaceEnemySeventeeen(replaceAltoAngelo);
                                break;
                            case 18:
                                ReplaceEnemyEighteen(replaceAltoAngelo);
                                break;
                            case 19:
                                ReplaceEnemyNineteen(replaceAltoAngelo);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Mephisto with: "))
                    {
                        const char* mephisto_items[] = { "Default",        "Scarecrow(Leg)", "Scarecrow(Arm)",
                                                         "Mega Scarecrow", "Bianco Angelo",  "Alto Angelo",
                                                         "Faust",          "Frost",          "Assault",
                                                         "Blitz",          "Chimera Seed",   "Basilisk",
                                                         "Berial",         "Credo",          "Agnus",
                                                         "Sanctus",        "Dante" };
                        static int mephisto_current = 0;
                        if (ImGui::Combo("Replace Mephisto", &mephisto_current, mephisto_items,
                                         IM_ARRAYSIZE(mephisto_items)))
                        {
                            switch (mephisto_current)
                            {
                            case 0:
                                ReplaceEnemyDefault(replaceMephisto);
                                break;
                            case 1:
                                ReplaceEnemyOne(replaceMephisto);
                                break;
                            case 2:
                                ReplaceEnemyTwo(replaceMephisto);
                                break;
                            case 3:
                                ReplaceEnemyThree(replaceMephisto);
                                break;
                            case 4:
                                ReplaceEnemyFour(replaceMephisto);
                                break;
                            case 5:
                                ReplaceEnemyFive(replaceMephisto);
                                break;
                            case 6:
                                ReplaceEnemySix(replaceMephisto);
                                break;
                            case 7:
                                ReplaceEnemySeven(replaceMephisto);
                                break;
                            case 8:
                                ReplaceEnemyEight(replaceMephisto);
                                break;
                            case 9:
                                ReplaceEnemyNine(replaceMephisto);
                                break;
                            case 10:
                                ReplaceEnemyTen(replaceMephisto);
                                break;
                            case 11:
                                ReplaceEnemyEleven(replaceMephisto);
                                break;
                            case 12:
                                ReplaceEnemyTwelve(replaceMephisto);
                                break;
                            case 13:
                                ReplaceEnemyThirteen(replaceMephisto);
                                break;
                            case 14:
                                ReplaceEnemyFourteen(replaceMephisto);
                                break;
                            case 15:
                                ReplaceEnemyFifteen(replaceMephisto);
                                break;
                            case 16:
                                ReplaceEnemySixteen(replaceMephisto);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Faust with: "))
                    {
                        const char* faust_items[] = {
                            "Default",      "Scarecrow(Leg)", "Scarecrow(Arm)", "Mega Scarecrow", "Bianco Angelo",
                            "Alto Angelo",  "Mephisto",       "Frost",          "Assault",        "Blitz",
                            "Chimera Seed", "Basilisk",       "Berial",         "Bael",           "Echidna",
                            "Credo",        "Agnus",          "Sanctus",        "Kyrie",          "Dante"
                        };
                        static int faust_current = 0;
                        if (ImGui::Combo("Replace Faust", &faust_current, faust_items, IM_ARRAYSIZE(faust_items)))
                        {
                            switch (faust_current)
                            {
                            case 0:
                                ReplaceEnemyDefault(replaceFaust);
                                break;
                            case 1:
                                ReplaceEnemyOne(replaceFaust);
                                break;
                            case 2:
                                ReplaceEnemyTwo(replaceFaust);
                                break;
                            case 3:
                                ReplaceEnemyThree(replaceFaust);
                                break;
                            case 4:
                                ReplaceEnemyFour(replaceFaust);
                                break;
                            case 5:
                                ReplaceEnemyFive(replaceFaust);
                                break;
                            case 6:
                                ReplaceEnemySix(replaceFaust);
                                break;
                            case 7:
                                ReplaceEnemySeven(replaceFaust);
                                break;
                            case 8:
                                ReplaceEnemyEight(replaceFaust);
                                break;
                            case 9:
                                ReplaceEnemyNine(replaceFaust);
                                break;
                            case 10:
                                ReplaceEnemyTen(replaceFaust);
                                break;
                            case 11:
                                ReplaceEnemyEleven(replaceFaust);
                                break;
                            case 12:
                                ReplaceEnemyTwelve(replaceFaust);
                                break;
                            case 13:
                                ReplaceEnemyThirteen(replaceFaust);
                                break;
                            case 14:
                                ReplaceEnemyFourteen(replaceFaust);
                                break;
                            case 15:
                                ReplaceEnemyFifteen(replaceFaust);
                                break;
                            case 16:
                                ReplaceEnemySixteen(replaceFaust);
                                break;
                            case 17:
                                ReplaceEnemySeventeeen(replaceFaust);
                                break;
                            case 18:
                                ReplaceEnemyEighteen(replaceFaust);
                                break;
                            case 19:
                                ReplaceEnemyNineteen(replaceFaust);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Frost with: "))
                    {
                        const char* frost_items[] = { "Default",        "Scarecrow(Leg)", "Scarecrow(Arm)",
                                                      "Mega Scarecrow", "Bianco Angelo",  "Alto Angelo",
                                                      "Mephisto",       "Faust",          "Assault",
                                                      "Blitz",          "Chimera Seed",   "Basilisk",
                                                      "Berial",         "Credo",          "Agnus",
                                                      "Sanctus",        "Dante" };
                        static int frost_current = 0;
                        if (ImGui::Combo("Replace Faust", &frost_current, frost_items, IM_ARRAYSIZE(frost_items)))
                        {
                            switch (frost_current)
                            {
                            case 0:
                                ReplaceEnemyDefault(replaceFrost);
                                break;
                            case 1:
                                ReplaceEnemyOne(replaceFrost);
                                break;
                            case 2:
                                ReplaceEnemyTwo(replaceFrost);
                                break;
                            case 3:
                                ReplaceEnemyThree(replaceFrost);
                                break;
                            case 4:
                                ReplaceEnemyFour(replaceFrost);
                                break;
                            case 5:
                                ReplaceEnemyFive(replaceFrost);
                                break;
                            case 6:
                                ReplaceEnemySix(replaceFrost);
                                break;
                            case 7:
                                ReplaceEnemySeven(replaceFrost);
                                break;
                            case 8:
                                ReplaceEnemyEight(replaceFrost);
                                break;
                            case 9:
                                ReplaceEnemyNine(replaceFrost);
                                break;
                            case 10:
                                ReplaceEnemyTen(replaceFrost);
                                break;
                            case 11:
                                ReplaceEnemyEleven(replaceFrost);
                                break;
                            case 12:
                                ReplaceEnemyTwelve(replaceFrost);
                                break;
                            case 13:
                                ReplaceEnemyThirteen(replaceFrost);
                                break;
                            case 14:
                                ReplaceEnemyFourteen(replaceFrost);
                                break;
                            case 15:
                                ReplaceEnemyFifteen(replaceFrost);
                                break;
                            case 16:
                                ReplaceEnemySixteen(replaceFrost);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Assault with: "))
                    {
                        const char* assault_items[] = { "Default",        "Scarecrow(Leg)", "Scarecrow(Arm)",
                                                        "Mega Scarecrow", "Bianco Angelo",  "Alto Angelo",
                                                        "Mephisto",       "Faust",          "Frost",
                                                        "Blitz",          "Chimera Seed",   "Basilisk",
                                                        "Berial",         "Credo",          "Agnus",
                                                        "Sanctus",        "Dante" };
                        static int assault_current = 0;
                        if (ImGui::Combo("Replace Assault", &assault_current, assault_items,
                                         IM_ARRAYSIZE(assault_items)))
                        {
                            switch (assault_current)
                            {
                            case 0:
                                ReplaceEnemyDefault(replaceAssault);
                                break;
                            case 1:
                                ReplaceEnemyOne(replaceAssault);
                                break;
                            case 2:
                                ReplaceEnemyTwo(replaceAssault);
                                break;
                            case 3:
                                ReplaceEnemyThree(replaceAssault);
                                break;
                            case 4:
                                ReplaceEnemyFour(replaceAssault);
                                break;
                            case 5:
                                ReplaceEnemyFive(replaceAssault);
                                break;
                            case 6:
                                ReplaceEnemySix(replaceAssault);
                                break;
                            case 7:
                                ReplaceEnemySeven(replaceAssault);
                                break;
                            case 8:
                                ReplaceEnemyEight(replaceAssault);
                                break;
                            case 9:
                                ReplaceEnemyNine(replaceAssault);
                                break;
                            case 10:
                                ReplaceEnemyTen(replaceAssault);
                                break;
                            case 11:
                                ReplaceEnemyEleven(replaceAssault);
                                break;
                            case 12:
                                ReplaceEnemyTwelve(replaceAssault);
                                break;
                            case 13:
                                ReplaceEnemyThirteen(replaceAssault);
                                break;
                            case 14:
                                ReplaceEnemyFourteen(replaceAssault);
                                break;
                            case 15:
                                ReplaceEnemyFifteen(replaceAssault);
                                break;
                            case 16:
                                ReplaceEnemySixteen(replaceAssault);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Blitz with: "))
                    {
                        const char* blitz_items[] = {
                            "Default",      "Scarecrow(Leg)", "Scarecrow(Arm)", "Mega Scarecrow", "Bianco Angelo",
                            "Alto Angelo",  "Mephisto",       "Faust",          "Frost",          "Assault",
                            "Chimera Seed", "Basilisk",       "Berial",         "Bael",           "Echidna",
                            "Credo",        "Agnus",          "Sanctus",        "Kyrie",          "Dante"
                        };
                        static int blitz_current = 0;
                        if (ImGui::Combo("Replace Blitz", &blitz_current, blitz_items, IM_ARRAYSIZE(blitz_items)))
                        {
                            switch (blitz_current)
                            {
                            case 0:
                                ReplaceEnemyDefault(replaceBlitz);
                                break;
                            case 1:
                                ReplaceEnemyOne(replaceBlitz);
                                break;
                            case 2:
                                ReplaceEnemyTwo(replaceBlitz);
                                break;
                            case 3:
                                ReplaceEnemyThree(replaceBlitz);
                                break;
                            case 4:
                                ReplaceEnemyFour(replaceBlitz);
                                break;
                            case 5:
                                ReplaceEnemyFive(replaceBlitz);
                                break;
                            case 6:
                                ReplaceEnemySix(replaceBlitz);
                                break;
                            case 7:
                                ReplaceEnemySeven(replaceBlitz);
                                break;
                            case 8:
                                ReplaceEnemyEight(replaceBlitz);
                                break;
                            case 9:
                                ReplaceEnemyNine(replaceBlitz);
                                break;
                            case 10:
                                ReplaceEnemyTen(replaceBlitz);
                                break;
                            case 11:
                                ReplaceEnemyEleven(replaceBlitz);
                                break;
                            case 12:
                                ReplaceEnemyTwelve(replaceBlitz);
                                break;
                            case 13:
                                ReplaceEnemyThirteen(replaceBlitz);
                                break;
                            case 14:
                                ReplaceEnemyFourteen(replaceBlitz);
                                break;
                            case 15:
                                ReplaceEnemyFifteen(replaceBlitz);
                                break;
                            case 16:
                                ReplaceEnemySixteen(replaceBlitz);
                                break;
                            case 17:
                                ReplaceEnemySeventeeen(replaceBlitz);
                                break;
                            case 18:
                                ReplaceEnemyEighteen(replaceBlitz);
                                break;
                            case 19:
                                ReplaceEnemyNineteen(replaceBlitz);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Chimera Seed with: "))
                    {
                        const char* chimera_items[] = { "Default",        "Scarecrow(Leg)", "Scarecrow(Arm)",
                                                        "Mega Scarecrow", "Bianco Angelo",  "Alto Angelo",
                                                        "Mephisto",       "Faust",          "Frost",
                                                        "Assault",        "Blitz",          "Basilisk",
                                                        "Berial",         "Credo",          "Agnus",
                                                        "Sanctus",        "Dante" };
                        static int chimera_current = 0;
                        if (ImGui::Combo("Replace Seed", &chimera_current, chimera_items, IM_ARRAYSIZE(chimera_items)))
                        {
                            switch (chimera_current)
                            {
                            case 0:
                                ReplaceEnemyDefault(replaceChimera);
                                break;
                            case 1:
                                ReplaceEnemyOne(replaceChimera);
                                break;
                            case 2:
                                ReplaceEnemyTwo(replaceChimera);
                                break;
                            case 3:
                                ReplaceEnemyThree(replaceChimera);
                                break;
                            case 4:
                                ReplaceEnemyFour(replaceChimera);
                                break;
                            case 5:
                                ReplaceEnemyFive(replaceChimera);
                                break;
                            case 6:
                                ReplaceEnemySix(replaceChimera);
                                break;
                            case 7:
                                ReplaceEnemySeven(replaceChimera);
                                break;
                            case 8:
                                ReplaceEnemyEight(replaceChimera);
                                break;
                            case 9:
                                ReplaceEnemyNine(replaceChimera);
                                break;
                            case 10:
                                ReplaceEnemyTen(replaceChimera);
                                break;
                            case 11:
                                ReplaceEnemyEleven(replaceChimera);
                                break;
                            case 12:
                                ReplaceEnemyTwelve(replaceChimera);
                                break;
                            case 13:
                                ReplaceEnemyThirteen(replaceChimera);
                                break;
                            case 14:
                                ReplaceEnemyFourteen(replaceChimera);
                                break;
                            case 15:
                                ReplaceEnemyFifteen(replaceChimera);
                                break;
                            case 16:
                                ReplaceEnemySixteen(replaceChimera);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Basilisk with: "))
                    {
                        const char* basilisk_items[] = { "Default",        "Scarecrow(Leg)", "Scarecrow(Arm)",
                                                         "Mega Scarecrow", "Bianco Angelo",  "Alto Angelo",
                                                         "Mephisto",       "Faust",          "Frost",
                                                         "Assault",        "Blitz",          "Chimera Seed",
                                                         "Berial",         "Credo",          "Agnus",
                                                         "Sanctus",        "Dante" };
                        static int basilisk_current = 0;
                        if (ImGui::Combo("Replace Basilisk", &basilisk_current, basilisk_items,
                                         IM_ARRAYSIZE(basilisk_items)))
                        {
                            switch (basilisk_current)
                            {
                            case 0:
                                ReplaceEnemyDefault(replaceBasilisk);
                                break;
                            case 1:
                                ReplaceEnemyOne(replaceBasilisk);
                                break;
                            case 2:
                                ReplaceEnemyTwo(replaceBasilisk);
                                break;
                            case 3:
                                ReplaceEnemyThree(replaceBasilisk);
                                break;
                            case 4:
                                ReplaceEnemyFour(replaceBasilisk);
                                break;
                            case 5:
                                ReplaceEnemyFive(replaceBasilisk);
                                break;
                            case 6:
                                ReplaceEnemySix(replaceBasilisk);
                                break;
                            case 7:
                                ReplaceEnemySeven(replaceBasilisk);
                                break;
                            case 8:
                                ReplaceEnemyEight(replaceBasilisk);
                                break;
                            case 9:
                                ReplaceEnemyNine(replaceBasilisk);
                                break;
                            case 10:
                                ReplaceEnemyTen(replaceBasilisk);
                                break;
                            case 11:
                                ReplaceEnemyEleven(replaceBasilisk);
                                break;
                            case 12:
                                ReplaceEnemyTwelve(replaceBasilisk);
                                break;
                            case 13:
                                ReplaceEnemyThirteen(replaceBasilisk);
                                break;
                            case 14:
                                ReplaceEnemyFourteen(replaceBasilisk);
                                break;
                            case 15:
                                ReplaceEnemyFifteen(replaceBasilisk);
                                break;
                            case 16:
                                ReplaceEnemySixteen(replaceBasilisk);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Berial with: "))
                    {
                        const char* berial_items[] = { "Default", "Credo", "Agnus", "Sanctus", "Dante" };
                        static int berial_current = 0;
                        if (ImGui::Combo("Replace Berial", &berial_current, berial_items, IM_ARRAYSIZE(berial_items)))
                        {
                            switch (berial_current)
                            {
                            case 0:
                                ReplaceEnemyDefault(replaceBerial);
                                break;
                            case 1:
                                ReplaceEnemyOne(replaceBerial);
                                break;
                            case 2:
                                ReplaceEnemyTwo(replaceBerial);
                                break;
                            case 3:
                                ReplaceEnemyThree(replaceBerial);
                                break;
                            case 4:
                                ReplaceEnemyFour(replaceBerial);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Bael with: "))
                    {
                        const char* bael_items[] = { "Default", "Berial", "Credo", "Agnus", "Sanctus", "Dante" };
                        static int bael_current = 0;
                        if (ImGui::Combo("Replace Bael", &bael_current, bael_items, IM_ARRAYSIZE(bael_items)))
                        {
                            switch (bael_current)
                            {
                            case 0:
                                ReplaceEnemyDefault(replaceBael);
                                break;
                            case 1:
                                ReplaceEnemyOne(replaceBael);
                                break;
                            case 2:
                                ReplaceEnemyTwo(replaceBael);
                                break;
                            case 3:
                                ReplaceEnemyThree(replaceBael);
                                break;
                            case 4:
                                ReplaceEnemyFour(replaceBael);
                                break;
                            case 5:
                                ReplaceEnemyFive(replaceBael);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Echidna with: "))
                    {
                        const char* echidna_items[] = { "Default", "Berial", "Credo", "Agnus", "Sanctus", "Dante" };
                        static int echidna_current = 0;
                        if (ImGui::Combo("Replace Echidna", &echidna_current, echidna_items,
                                         IM_ARRAYSIZE(echidna_items)))
                        {
                            switch (echidna_current)
                            {
                            case 0:
                                ReplaceEnemyDefault(replaceEchidna);
                                break;
                            case 1:
                                ReplaceEnemyOne(replaceEchidna);
                                break;
                            case 2:
                                ReplaceEnemyTwo(replaceEchidna);
                                break;
                            case 3:
                                ReplaceEnemyThree(replaceEchidna);
                                break;
                            case 4:
                                ReplaceEnemyFour(replaceEchidna);
                                break;
                            case 5:
                                ReplaceEnemyFive(replaceEchidna);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Credo with: "))
                    {
                        const char* credo_items[] = { "Default", "Berial",  "Bael", "Echidna",
                                                      "Agnus",   "Sanctus", "Dante" };
                        static int credo_current = 0;
                        if (ImGui::Combo("Replace Credo", &credo_current, credo_items, IM_ARRAYSIZE(credo_items)))
                        {
                            switch (credo_current)
                            {
                            case 0:
                                ReplaceEnemyDefault(replaceCredo);
                                break;
                            case 1:
                                ReplaceEnemyOne(replaceCredo);
                                break;
                            case 2:
                                ReplaceEnemyTwo(replaceCredo);
                                break;
                            case 3:
                                ReplaceEnemyThree(replaceCredo);
                                break;
                            case 4:
                                ReplaceEnemyFour(replaceCredo);
                                break;
                            case 5:
                                ReplaceEnemyFive(replaceCredo);
                                break;
                            case 6:
                                ReplaceEnemySix(replaceCredo);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Agnus with: "))
                    {
                        const char* agnus_items[] = { "Default", "Berial", "Credo", "Sanctus" };
                        static int agnus_current = 0;
                        if (ImGui::Combo("Replace Agnus", &agnus_current, agnus_items, IM_ARRAYSIZE(agnus_items)))
                        {
                            switch (agnus_current)
                            {
                            case 0:
                                ReplaceEnemyDefault(replaceAgnus);
                                break;
                            case 1:
                                ReplaceEnemyOne(replaceAgnus);
                                break;
                            case 2:
                                ReplaceEnemyTwo(replaceAgnus);
                                break;
                            case 3:
                                ReplaceEnemyThree(replaceAgnus);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Practice"))
            {
                ImGui::Spacing();
                ImGui::Text("General");

                if (ImGui::Checkbox("Infinite Health (All)", &checkinfiniteAllHealth))
                {
                    ToggleInfiniteAllHealth(checkinfiniteAllHealth);
                }

                ImGui::SameLine(202);

                if (ImGui::Checkbox("Infinite DT", &checkInfiniteDT))
                {
                    ImGuiToggleInfDT();
                }

                if (ImGui::Checkbox("Infinite Player Health", &checkInfinitePlayerHealth))
                {
                    ImGuiToggleInfPlayerHealth();
                }

                ImGui::SameLine(202);

                if (ImGui::Checkbox("Infinite Revive", &checkInfiniteRevive))
                {
                    ToggleInfiniteRevive(checkInfiniteRevive);
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                Misc();

                if (ImGui::Checkbox("Berial Daze", &checkBerialPractice))
                {
                    ToggleBerialPractice(checkBerialPractice);
                    ImGuiToggleBerialDaze();
                }

                ImGui::SameLine(202);

                if (ImGui::Checkbox("Stun Anything", &checkStunAnything))
                {
                    ToggleStunAnything(checkStunAnything);
                }

                if (ImGui::Checkbox("Remove Launch & Knockaway Armour", &checkRemoveLaunchArmour))
                {
                    ToggleRemoveLaunchArmour(checkRemoveLaunchArmour);
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::CollapsingHeader("Disable Darkslayer Inputs"))
                {
                    if (ImGui::Checkbox("Disable Darkslayer Dpad Up", &checkDisableDarkslayerUp))
                    {
                        ToggleDisableDarkslayerUp(checkDisableDarkslayerUp);
                    }

                    if (ImGui::Checkbox("Disable Darkslayer Dpad Down", &checkDisableDarkslayerDown))
                    {
                        ToggleDisableDarkslayerDown(checkDisableDarkslayerDown);
                    }

                    if (ImGui::Checkbox("Disable Darkslayer Dpad Left", &checkDisableDarkslayerLeft))
                    {
                        ToggleDisableDarkslayerLeft(checkDisableDarkslayerLeft);
                    }

                    if (ImGui::Checkbox("Disable Darkslayer Dpad Right", &checkDisableDarkslayerRight))
                    {
                        ToggleDisableDarkslayerRight(checkDisableDarkslayerRight);
                    }
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::EndTabItem();
            }

            CreditsDrawing();
            ImGui::EndTabBar();
        }
        ImGui::End();
    }
    // Render dear imgui into screen
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}