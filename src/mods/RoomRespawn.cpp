// include your mod header file
#include "RoomRespawn.hpp"

static uintptr_t  jmp_return { NULL };
static uintptr_t  script_bp_jmp_return { NULL };
static uintptr_t  script_mission_jmp_return { NULL };

static uint32_t   g_spawn_index_cache{};

bool RoomRespawn::g_reset_manager { false };

struct RPlacement : CResource {};

// linker errors with actual MtObject vtable,
// too lazy to debug properly
struct MtObjectT {
    void* vtable;
};

struct RPlacementXmlTrack : MtObjectT
{
    MtString m_track_name;
    MtString m_prop_name;
    MtString m_prop_type;
    uint32_t m_prop_index;
    RPlacementXmlTrack **mp_track;
    uint32_t m_track_num;
    bool m_track_expand;
    bool m_track_select;
    RPlacementXmlTrack *mp_parent_track;
};

struct RPlacementXmlRootTrack : RPlacementXmlTrack {};

struct RPlacementXml : RPlacement 
{
    RPlacementXmlRootTrack m_root_track;
};
// same linker fucky-wucky 
class CUnitT : public MtObjectT
{
public:
    uint32_t bitfield; //0x0004
    class CUnit *mp_next_unit; //0x0008
    class CUnit *mp_prev_unit; //0x000C
    float m_delta_time; //0x0010
    uint32_t some_union; //0x0014
}; //Size: 0x0018
static_assert(sizeof(CUnit) == 0x18);

struct UEnemySetCtrl : CUnitT
{
    RPlacementXml *mp_placement;
    int32_t m_set_id;
    int32_t m_group_id;
    int32_t m_loop_num;
    int32_t m_dead_timer;
    int32_t m_attack_num;
    float   m_wait_timer;
    int32_t m_current_track;
    int32_t m_permit_attack_id;
    RPlacementXmlTrack *mp_track;
    bool m_active;
    bool m_battle;
};

static void try_respawn() {
    // man for some reason all this bullshit doesnt seem to crash or leak memory wtf 
    //__debugbreak();
    static constexpr uintptr_t s_unit_ptr                                   = 0x00E552CC;
    static constexpr uintptr_t s_unit_add_bottom_sub_8dc540                 = 0x008DC540;
    static constexpr uintptr_t r_placement_xml_play_track                   = 0x00737F40;
    static constexpr uintptr_t u_enemy_set_ctrl_placement_new_sub_7375d0    = 0x007375D0;
    static constexpr uintptr_t r_placement_xml_loading_something_sub_737cc0 = 0x00737CC0;

    UEnemySetCtrl* u_sc_ptr = nullptr;
    //uint32_t rPlacementXmlRet = 0;
    RoomRespawn::g_reset_manager = true;
    _asm {
        //allocate memory for uEnemySetCtrl
        mov ecx, 0E1434Ch // ptr to MtHeapAllocator
        mov eax, [ecx]
        mov ecx, eax // MtHeapAllocator this ptr
        mov edx, [ecx] 
        mov eax, [edx + 0x14] // vtable MtAlloc ptr
        push 10h // dont remember some flags or something
        push 70h // size iirc
        call eax
        test eax, eax
        jz ALLO_FAIL
        // uEnemySetCtrl placement new
        // needs mission or some index from cBranchSomething script
        movzx edx, [g_spawn_index_cache]
        call u_enemy_set_ctrl_placement_new_sub_7375d0
        test eax, eax
        jz ALLO_FAIL
        mov esi, eax
        mov [u_sc_ptr], eax // save usc for later just in case
        // sUnit add bottom, same as enemy spawn stuff
        // so TODO(): general sUnit functions in Devil4.hpp
        mov eax, [s_unit_ptr]
        mov eax, [eax] //sUnit in eax
        push 1 // moveline some argument
        call s_unit_add_bottom_sub_8dc540
        mov eax, [u_sc_ptr]
        // load rPlacementXml emset bullshit
        push 0 // idk what this one means
        push eax // uEnemySetCtrl
        call r_placement_xml_loading_something_sub_737cc0
        // that thing above seems to be writing past sizeof(uEnemySetCtrl) ?!
        // not sure whats the proper size for it, works on my machine

        // mov [rPlacementXmlRet], eax // save just in case
        // return value from rPlacementXml_loading_something_sub_737CC0
        // should have an index for PlayTrack
        // rPlacementXml::PlayTrack
        push eax
        mov eax, [u_sc_ptr]
        push eax
        call r_placement_xml_play_track

    ALLO_FAIL:
        nop
    }
}

// detour 
// naked is defined to __declspec(naked)
naked void detour() 
{
	__asm {
        cmp byte ptr [RoomRespawn::g_reset_manager], 1
        jne originalCode
        mov al, 1
        ret 4
    originalCode:
        mov eax, dword ptr [0E552C8h]
        mov eax, [eax]
        jmp DWORD PTR [jmp_return]
	}
}

naked void script_mission_detour()
{
    __asm {
        mov ecx, [ebx+04h]
        movzx edx, BYTE PTR [ecx+01h]
        mov DWORD PTR [g_spawn_index_cache], edx
        jmp DWORD PTR [script_mission_jmp_return]
    }
}

naked void script_mission_bp_detour()
{
    __asm {
        mov ecx, [ebx+04h]
        movzx edx, BYTE PTR [ecx+01h]
        mov DWORD PTR [g_spawn_index_cache], edx
        jmp DWORD PTR [script_bp_jmp_return]
    }
}

std::optional<std::string> RoomRespawn::on_initialize() {
    // char __userpurge cEnemyResetMgr_next_slot_something_sub_738D70@<al>(int groupId@<ecx>, __int16 IndexSomething@<di>, cEnemyResetMgr *a3@<esi>, int a4)
    if (!install_hook_absolute(0x738D70, m_hook, &detour, &jmp_return, 5))
    {
        spdlog::error("Failed to init LoadOrder mod\n");
        return "Failed to init LoadOrder mod";
    }

    if (!install_hook_absolute(0x44AAA5, m_script_enemy_set_ctrl_opcode_mission, &script_mission_detour, &script_mission_jmp_return, 7))
    {
        spdlog::error("Failed to init LoadOrder mod\n");
        return "Failed to init LoadOrder mod";
    }
    if (!install_hook_absolute(0x44CCE1, m_script_enemy_set_ctrl_opcode_bp, &script_mission_bp_detour, &script_bp_jmp_return, 7))
    {
        spdlog::error("Failed to init LoadOrder mod\n");
        return "Failed to init LoadOrder mod";
    }
    
	return Mod::on_initialize();
}

// onFrame()
// do something each frame example
//void RoomRespawn::onFrame(fmilliseconds& dt) {}

// onConfigSave
// save your data into cfg structure.
void RoomRespawn::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("respawn_enemies_when_revisiting_rooms", g_reset_manager);
};

// onConfigLoad
// load data into variables from config structure.
void RoomRespawn::on_config_load(const utility::Config& cfg) {
    g_reset_manager = cfg.get<bool>("respawn_enemies_when_revisiting_rooms").value_or(false);
};

// onGUIframe()
// draw your imgui widgets here, you are inside imgui context.
void RoomRespawn::on_gui_frame() {
    ImGui::Checkbox(_("Respawn enemies when visiting the same room multiple times"), &g_reset_manager);
    ImGui::SameLine();
    help_marker(_("This will break your style points in the current mission"));
    if (ImGui::Button(_("Respawn Enemies in current room"))) {
        try_respawn();
    }
    ImGui::SameLine();
    help_marker(_("Respawning enemies in incorrect missions will freeze the game\nThis button will enable the above checkbox"));
};

// onGamePause()
// do something when toggling a gui
//void ModName::onGamePause(bool toggle) { };

// onMessage()
// handle some window message, return true to pass to the game window
// or false to drop it.
//bool ModName::onMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) { return true; };
