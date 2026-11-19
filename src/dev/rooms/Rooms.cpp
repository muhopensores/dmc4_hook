#include "Rooms.hpp"
#include "../../sdk/RoomClasses.hpp"
#include "../../sdk/sArea.hpp"
#include "../../mods/AreaJump.hpp"
#include "../../sdk/MissionMenu.hpp"
#include <spdlog/spdlog.h>
#include <cstring>

bool Rooms::test_room = true;

namespace {

constexpr uintptr_t kAreaSite = 0x8DB1A9;   // sArea constructor: mov dword ptr [eax+4], "AreaRoot"; next: call createAreaInfo
constexpr uint8_t kAreaBytes[7] = {0xC7, 0x40, 0x04, 0xDC, 0xF6, 0xBF, 0x00};
uintptr_t jmp_ret_area = 0;

void __stdcall on_area_info() { room_classes::create_all(); }

naked void detour_area() {
    _asm {
        pushad
        call on_area_info
        popad
        mov dword ptr [eax+4], 0x00BFF6DC   // "AreaRoot"
        jmp dword ptr [jmp_ret_area]
    }
}

// The test class (R2): aRoom900 : aRoom200, logs from every overridden slot.
struct TestFields {
    uint32_t frames;
    uint32_t moves_logged;
};

void test_create(room_classes::Ctx& c) { spdlog::info("Rooms test: aRoom900 created ({})", (void*)c.room); }
void test_destroy(room_classes::Ctx& c) {
    spdlog::info("Rooms test: aRoom900 destroyed after {} frames", static_cast<TestFields*>(c.fields)->frames);
}
void test_init(room_classes::Ctx& c) {
    spdlog::info("Rooms test: aRoom900 init (room {}, stage {})", c.room->room_no, c.room->stage_no);
    room_classes::base(c);
}
void test_move(room_classes::Ctx& c) {
    room_classes::base(c);
    auto* f = static_cast<TestFields*>(c.fields);
    if (++f->frames % 60 == 0 && f->moves_logged < 30) {   // once a second, the first half minute
        ++f->moves_logged;
        spdlog::info("Rooms test: aRoom900 move, {} frames, mode {}", f->frames, c.room->mode);
    }
}
void test_final(room_classes::Ctx& c) {
    spdlog::info("Rooms test: aRoom900 final after {} frames", static_cast<TestFields*>(c.fields)->frames);
    room_classes::base(c);
}

const char* room_name(aRoom* r) {   // vtable slot 10 getName (a static string in every room class)
    if (!r || !r->vtable) return "?";
    using FnName = const char*(__thiscall*)(void*);
    return reinterpret_cast<FnName>(static_cast<void**>(r->vtable)[10])(r);
}

} // namespace

std::optional<std::string> Rooms::on_initialize() {
    if (std::memcmp(reinterpret_cast<const void*>(kAreaSite), kAreaBytes, sizeof kAreaBytes) != 0) {
        spdlog::error("Rooms: unexpected code at 0x{:X}; room classes won't be made", kAreaSite);
        return "Failed to init Rooms (area hook site)";
    }
    if (!install_hook_absolute(kAreaSite, hook_area, &detour_area, &jmp_ret_area, 7)) {
        spdlog::error("Rooms: failed to hook the sArea constructor (0x{:X})", kAreaSite);
        return "Failed to init Rooms (area hook)";
    }
    mission_menu::install();   // Enter from the Mission Menu
    return Mod::on_initialize();
}

void Rooms::on_config_load(const utility::Config& cfg) {
    test_room = cfg.get<bool>("rooms_test_room").value_or(true);
    if (!test_room || room_classes::created()) return;
    room_classes::ClassDesc d;
    d.owner = "Rooms test";
    d.room = 900;
    d.base = "aRoom200";
    d.fields_size = sizeof(TestFields);
    d.cb.create = test_create;
    d.cb.destroy = test_destroy;
    d.cb.init = test_init;
    d.cb.move = test_move;
    d.cb.final = test_final;
    room_classes::add(d);
}

void Rooms::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("rooms_test_room", test_room);
}

void Rooms::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    ImGui::PushID("Rooms");
    if (ImGui::TreeNode(_("Room classes (Rooms)"))) {
        ImGui::Checkbox(_("Test room 900"), &test_room);
        ImGui::SameLine();
        help_marker(_("Adds the test room class aRoom900, derived from aRoom200 (stage 200's files and scripts), which "
                      "writes to the log from its init, move and final. Takes effect at the next start."));
        if (aRoom* r = room_classes::current())
            ImGui::Text(_("Current room: %s, room %d, stage %d"), room_name(r), r->room_no, r->stage_no);
        if (!room_classes::created())
            ImGui::TextDisabled(_("Room classes are made when the game starts."));
        const auto rows = room_classes::list();
        if (rows.empty()) ImGui::TextDisabled(_("No room classes added."));
        for (const auto& i : rows) {
            ImGui::PushID(i.name.c_str());
            ImGui::BulletText("%s : %s, room %d, stage %d, %u bytes (%s)", i.name.c_str(), i.base.c_str(), i.room, i.stage,
                              i.size, i.owner.c_str());
            if (!i.error.empty()) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", i.error.c_str());
            } else if (i.created) {
                ImGui::SameLine();
                ImGui::TextDisabled(i.in_area_table ? _("recorded, %d live") : _("NOT recorded by sArea, %d live"), i.live);
                ImGui::SameLine();
                if (ImGui::SmallButton(_("Enter"))) mission_menu::enter_room(i.room);   // from a room, or the Mission Menu
            }
            ImGui::PopID();
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}
