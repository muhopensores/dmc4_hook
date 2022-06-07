// include your mod header file
#include "modRoomRespawn.hpp"

static uintptr_t  jmp_return { NULL };
static uintptr_t  script_bp_jmp_return { NULL };
static uintptr_t  script_mission_jmp_return { NULL };

static uint32_t   g_spawn_index_cache{};

static bool g_reset_manager { false };

struct rPlacement : cResource {};

// linker errors with actual MtObject vtable,
// too lazy to debug properly
struct MtObjectT {
    void* vtable;
};

struct rPlacementXmlTrack : MtObjectT
{
    MtString mTrackName;
    MtString mPropName;
    MtString mPropType;
    uint32_t mPropIndex;
    rPlacementXmlTrack **mpTrack;
    uint32_t mTrackNum;
    bool mTrackExpand;
    bool mTrackSelect;
    rPlacementXmlTrack *mpParentTrack;
};

struct rPlacementXmlRootTrack : rPlacementXmlTrack {};

struct rPlacementXml : rPlacement 
{
    rPlacementXmlRootTrack mRootTrack;
};
// same linker fucky-wucky 
class cUnitT : public MtObjectT
{
public:
    uint32_t bitfield; //0x0004
    class cUnit *mpNextUnit; //0x0008
    class cUnit *mpPrevUnit; //0x000C
    float mDeltaTime; //0x0010
    uint32_t someUnion; //0x0014
}; //Size: 0x0018
static_assert(sizeof(cUnit) == 0x18);

struct uEnemySetCtrl : cUnitT
{
    rPlacementXml *mpPlacement;
    int32_t mSetID;
    int32_t mGroupID;
    int32_t mLoopNum;
    int32_t mDeadTimer;
    int32_t mAttackNum;
    float   mWaitTimer;
    int32_t mCurrentTrack;
    int32_t mPermitAttackId;
    rPlacementXmlTrack *mpTrack;
    bool mActive;
    bool mBattle;
};

static void try_respawn() {
    // man for some reason all this bullshit doesnt seem to crash or leak memory wtf 
    //__debugbreak();
    static constexpr uintptr_t sUnitPtr                                   = 0x00E552CC;
    static constexpr uintptr_t sUnit_AddBottom_sub_8DC540                 = 0x008DC540;
    static constexpr uintptr_t rPlacementXmlPlayTrack                     = 0x00737F40;
    static constexpr uintptr_t uEnemySetCtrl_placement_new_sub_7375D0     = 0x007375D0;
    static constexpr uintptr_t rPlacementXml_loading_something_sub_737CC0 = 0x00737CC0;

    uEnemySetCtrl* uScPtr = nullptr;
    //uint32_t rPlacementXmlRet = 0;
    g_reset_manager = true;
    _asm {
        //allocate memory for uEnemySetCtrl
        mov ecx, 0E1434Ch // ptr to MtHeapAllocator
        mov eax, [ecx]
        mov ecx, eax // MtHeapAllocator this ptr
        mov edx, [ecx] 
        mov eax, [edx + 14h] // vtable MtAlloc ptr
        push 10h // dont remember some flags or something
        push 70h // size iirc
        call eax
        test eax, eax
        jz ALLO_FAIL
        // uEnemySetCtrl placement new
        // needs mission or some index from cBranchSomething script
        movzx edx, [g_spawn_index_cache]
        call uEnemySetCtrl_placement_new_sub_7375D0
        test eax, eax
        jz ALLO_FAIL
        mov esi, eax
        mov [uScPtr], eax // save usc for later just in case
        // sUnit add bottom, same as enemy spawn stuff
        // so TODO(): general sUnit functions in Devil4.hpp
        mov eax, [sUnitPtr]
        mov eax, [eax] //sUnit in eax
        push 1 // moveline some argument
        call sUnit_AddBottom_sub_8DC540
        mov eax, [uScPtr]
        // load rPlacementXml emset bullshit
        push 0 // idk what this one means
        push eax // uEnemySetCtrl
        call rPlacementXml_loading_something_sub_737CC0
        // that thing above seems to be writing past sizeof(uEnemySetCtrl) ?!
        // not sure whats the proper size for it, works on my machine

        // mov [rPlacementXmlRet], eax // save just in case
        // return value from rPlacementXml_loading_something_sub_737CC0
        // should have an index for PlayTrack
        // rPlacementXml::PlayTrack
        push eax
        mov eax, [uScPtr]
        push eax
        call rPlacementXmlPlayTrack

    ALLO_FAIL:
        nop
    }
}

// detour 
// naked is defined to __declspec(naked)
naked void detour() 
{
	__asm {
        cmp [g_reset_manager], 1
        jne originalCode
        mov al, 1
        ret 4
    originalCode:
        mov eax, dword ptr [0E552C8h]
        mov eax, [eax]
        jmp DWORD PTR [jmp_return]
	}
}

naked void scriptMissionDetour()
{
    __asm {
        mov ecx, [ebx+04h]
        movzx edx, BYTE PTR [ecx+01h]
        mov DWORD PTR [g_spawn_index_cache], edx
        jmp DWORD PTR [script_mission_jmp_return]
    }
}

naked void scriptMissionBPDetour()
{
    __asm {
        mov ecx, [ebx+04h]
        movzx edx, BYTE PTR [ecx+01h]
        mov DWORD PTR [g_spawn_index_cache], edx
        jmp DWORD PTR [script_bp_jmp_return]
    }
}

std::optional<std::string> RoomRespawn::onInitialize() {
    // char __userpurge cEnemyResetMgr_next_slot_something_sub_738D70@<al>(int groupId@<ecx>, __int16 IndexSomething@<di>, cEnemyResetMgr *a3@<esi>, int a4)
    if (!install_hook_absolute(0x738D70, m_hook, &detour, &jmp_return, 5))
    {
        HL_LOG_ERR("Failed to init LoadOrder mod\n");
        return "Failed to init LoadOrder mod";
    }

    if (!install_hook_absolute(0x44AAA5, m_scriptEnemySetCtrlOpcodeMission, &scriptMissionDetour, &script_mission_jmp_return, 7))
    {
        HL_LOG_ERR("Failed to init LoadOrder mod\n");
        return "Failed to init LoadOrder mod";
    }
    if (!install_hook_absolute(0x44CCE1, m_scriptEnemySetCtrlOpcodeBP, &scriptMissionBPDetour, &script_bp_jmp_return, 7))
    {
        HL_LOG_ERR("Failed to init LoadOrder mod\n");
        return "Failed to init LoadOrder mod";
    }
    
	return Mod::onInitialize();
}

// onFrame()
// do something each frame example
//void RoomRespawn::onFrame(fmilliseconds& dt) {}

// onConfigSave
// save your data into cfg structure.
//void RoomRespawn::onConfigSave(utils::Config& cfg) { cfg.set<variable_type>("config_string",variable_name); };

// onConfigLoad
// load data into variables from config structure.
//void RoomRespawn::onConfigLoad(const utils::Config& cfg) { //variable_name = cfg.get<variable_type>("config_string").value_or(default_value); };

// onGUIframe()
// draw your imgui widgets here, you are inside imgui context.
void RoomRespawn::onGUIframe() 
{
    if (ImGui::Button("try respawning stuff")) {
        try_respawn();
    }
    ImGui::Text("respawning stuff in areas from wrong mission will freeze the game");
    ImGui::Checkbox("Respawn enemies when visiting the same room multiple times", &g_reset_manager);
};

// onGamePause()
// do something when toggling a gui
//void ModName::onGamePause(bool toggle) { };

// onMessage()
// handle some window message, return true to pass to the game window
// or false to drop it.
//bool ModName::onMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) { return true; };

