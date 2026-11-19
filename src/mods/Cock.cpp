#include "Cock.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/sUnit.hpp"
#include "../sdk/cResource.hpp"
#include "../sdk/Menus.hpp"   // sprMap, fontMap, rSprLayout, uSprLayout, uDevilCock, the .anm types (cSprAnm, ANM_*)

// The HUD's layouts (uDevilCock) are chained from the cockpit manager, the first unit of move line 25.
// (Its own copies of the layout types used to live here; sprMap's sprId sat at +0x08 in them, so Rot / Rot A / Rot D
// edited the fields 2 bytes over. The SDK's: sprId +0x06, rot +0x08, rotA +0x0A, rotD +0x0C, checked on the pause
// menu's sprites 2026-11-19.)
struct uCockpitMgr : cUnit {
    uDevilCock* mpStartPtr; // 0x18
    uDevilCock* mpEndPtr; // 0x1C
};
static_assert(sizeof(uCockpitMgr) == 0x20);

struct CockpitOverride {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int xOrg = 0;
    int yOrg = 0;
    int wOrg = 0;
    int hOrg = 0;
    int rot = 0;
    int rotA = 0;
    int rotD = 0;
    float color[4] = {1.f, 1.f, 1.f, 1.f};
    int selectedSprite = 0;
};

std::vector<std::vector<sprMap>> baseSpr;
std::vector<CockpitOverride> cockpitOverrides;

static void write_bgra(void* dst, const float in[4]) {
    uint8_t* c = (uint8_t*)dst;
    c[0] = uint8_t(in[2] * 255.0f);
    c[1] = uint8_t(in[1] * 255.0f);
    c[2] = uint8_t(in[0] * 255.0f);
    c[3] = uint8_t(in[3] * 255.0f);
}

// I'm going to tactically omit (_( from this so our zh friend doesn't judge me too much
void Cock::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::CollapsingHeader("HUD")) {
            sUnit* su = devil4_sdk::get_sUnit();
            ImGui::BeginGroup();
            ImGui::PushItemWidth(sameLineItemWidth);
            if (su != nullptr) {
                MoveLine* uiLine = &(su->mMoveLine[25]);
                if (uiLine->mTop != nullptr) {
                    uCockpitMgr* cockmgr = (uCockpitMgr*)uiLine->mTop;
                    size_t cockpitCount  = 0;
                    for (uDevilCock* c = cockmgr->mpStartPtr;; c = c->mpNextPtr) {
                        ++cockpitCount;
                        if (c == cockmgr->mpEndPtr)
                            break;
                    }

                    if (baseSpr.size() != cockpitCount)
                        baseSpr.resize(cockpitCount);

                    if (cockpitOverrides.size() != cockpitCount)
                        cockpitOverrides.resize(cockpitCount);

                    size_t initIndex     = 0;
                    uDevilCock* initCock = cockmgr->mpStartPtr;
                    while (true) {
                        rSprLayout* res = initCock->mpResource;
                        auto& base      = baseSpr[initIndex];
                        if (base.size() != res->mSprMapHead.sprNum) {
                            base.assign(initCock->mpData, initCock->mpData + res->mSprMapHead.sprNum);
                        }
                        if (initCock == cockmgr->mpEndPtr)
                            break;
                        initCock = initCock->mpNextPtr;
                        ++initIndex;
                    }

                    if (ImGui::BeginTabBar("Cocks")) {
                        int cockIndex    = 0;
                        uDevilCock* cock = cockmgr->mpStartPtr;
                        while (true) {
                            CockpitOverride& ov = cockpitOverrides[cockIndex];
                            char label[32];
                            sprintf_s(label, "Cock %zu", cockIndex);
                            if (ImGui::BeginTabItem(label)) {
                                if (ImGui::Button("Reset cock")) {
                                    ov = CockpitOverride{};
                                }
                                ImGui::SliderInt("X", &ov.x, -1000, 1000);
                                ImGui::SliderInt("Y", &ov.y, -1000, 1000);
                                ImGui::SliderInt("W", &ov.w, -500, 500);
                                ImGui::SliderInt("H", &ov.h, -500, 500);
                                ImGui::SliderInt("Org X", &ov.xOrg, -1000, 1000);
                                ImGui::SliderInt("Org Y", &ov.yOrg, -1000, 1000);
                                ImGui::SliderInt("Org W", &ov.wOrg, -500, 500);
                                ImGui::SliderInt("Org H", &ov.hOrg, -500, 500);
                                ImGui::SliderInt("Rotation", &ov.rot, -2000, 2000);
                                ImGui::SliderInt("Rotation A", &ov.rotA, -2000, 2000);
                                ImGui::SliderInt("Rotation D", &ov.rotD, -2000, 2000);
                                ImGui::ColorEdit4("Color", ov.color);
                                ImGui::SeparatorText("Sprites");
                                int spriteCount = (int)baseSpr[cockIndex].size();
                                if (spriteCount > 0) {
                                    ImGui::InputInt("Sprite ID", &ov.selectedSprite);
                                    ov.selectedSprite = std::clamp(ov.selectedSprite, 0, spriteCount - 1);
                                    sprMap& sprite    = baseSpr[cockIndex][ov.selectedSprite];
                                    ImGui::SliderInt("Sprite X", &sprite.posHDTV.x, -2000, 2000);
                                    ImGui::SliderInt("Sprite Y", &sprite.posHDTV.y, -2000, 2000);
                                    ImGui::SliderInt("Sprite W", (int*)&sprite.scaleHDTV.w, -1000, 1000);
                                    ImGui::SliderInt("Sprite H", (int*)&sprite.scaleHDTV.h, -1000, 1000);
                                    ImGui::SliderInt("Sprite Org X", &sprite.posHDTVOrg.x, -2000, 2000);
                                    ImGui::SliderInt("Sprite Org Y", &sprite.posHDTVOrg.y, -2000, 2000);
                                    ImGui::SliderInt("Sprite Org W", (int*)&sprite.scaleHDTVOrg.w, -1000, 1000);
                                    ImGui::SliderInt("Sprite Org H", (int*)&sprite.scaleHDTVOrg.h, -1000, 1000);
                                    static constexpr int rotMin = 0;
                                    static constexpr int rotMax = UINT16_MAX;
                                    ImGui::SliderScalar("Sprite Rot", ImGuiDataType_U16, &sprite.rot, &rotMin, &rotMax);
                                    ImGui::SliderScalar("Sprite Rot A", ImGuiDataType_U16, &sprite.rotA, &rotMin, &rotMax);
                                    ImGui::SliderScalar("Sprite Rot D", ImGuiDataType_U16, &sprite.rotD, &rotMin, &rotMax);
                                }
                                ImGui::EndTabItem();
                            }
                            if (cock == cockmgr->mpEndPtr)
                                break;

                            cock = cock->mpNextPtr;
                            ++cockIndex;
                        }
                        ImGui::EndTabBar();
                    }

                    size_t applyIndex     = 0;
                    uDevilCock* applyCock = cockmgr->mpStartPtr;
                    while (true) {
                        rSprLayout* res     = applyCock->mpResource;
                        auto& base          = baseSpr[applyIndex];
                        CockpitOverride& ov = cockpitOverrides[applyIndex];
                        for (unsigned int i = 0; i < res->mSprMapHead.sprNum; ++i) {
                            sprMap& src        = base[i];
                            sprMap& dst        = applyCock->mpData[i];
                            dst.posHDTV.x      = src.posHDTV.x + ov.x;
                            dst.posHDTV.y      = src.posHDTV.y + ov.y;
                            dst.scaleHDTV.w    = src.scaleHDTV.w + ov.w;
                            dst.scaleHDTV.h    = src.scaleHDTV.h + ov.h;
                            dst.posHDTVOrg.x   = src.posHDTVOrg.x + ov.xOrg;
                            dst.posHDTVOrg.y   = src.posHDTVOrg.y + ov.yOrg;
                            dst.scaleHDTVOrg.w = src.scaleHDTVOrg.w + ov.wOrg;
                            dst.scaleHDTVOrg.h = src.scaleHDTVOrg.h + ov.hOrg;
                            dst.rot            = src.rot + ov.rot;
                            dst.rotA           = src.rotA + ov.rotA;
                            dst.rotD           = src.rotD + ov.rotD;
                            write_bgra(&dst.color, ov.color);
                        }

                        if (applyCock == cockmgr->mpEndPtr)
                            break;

                        applyCock = applyCock->mpNextPtr;
                        ++applyIndex;
                    }
                }
            }
            ImGui::PopItemWidth();
            ImGui::EndGroup();
        }
    }
}
