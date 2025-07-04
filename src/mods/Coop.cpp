#include "Coop.hpp"
#include "..\sdk\Pad.hpp"
#include "..\sdk\Devil4.hpp"
#include "..\sdk\Cam.hpp"
#include "CharSwitcher.hpp"
#include "DebugCam.hpp"

bool Coop::mod_enabled;
static unsigned int player_num = 1;
static bool player_char[4] = {0, 0, 0, 0};
static sDevil4Pad** sDevil4Pad_ptr = (sDevil4Pad**)0x00e559c4;
static uintptr_t sCamera_ptr = 0x00E552D4;
static uintptr_t sMediator_ptr = 0x00E558B8;
static uintptr_t sDevil4Pad_cons = 0x00484C90;
static uintptr_t update_analog_info_call = 0x007b0250;
static uintptr_t uCameraCtrl_setup_end = 0x04f76da;

std::vector<sDevil4Pad*> PadArr;
std::vector<void*> PlayerArr;
std::vector<void*> CamArr;
std::vector<void*> HUDArr;

uintptr_t Coop::jmp_ret1 = NULL;
uintptr_t Coop::jmp_ret2 = NULL;
uintptr_t Coop::jmp_ret3 = NULL;
uintptr_t Coop::jmp_ret4 = NULL;
uintptr_t Coop::jmp_ret5 = NULL;
uintptr_t Coop::jmp_ret6 = NULL;
uintptr_t Coop::jmp_ret7 = NULL;
uintptr_t Coop::jmp_ret8 = NULL;
uintptr_t Coop::jmp_ret9 = NULL;
uintptr_t Coop::jmp_ret10 = NULL;
uintptr_t Coop::jmp_ret11 = NULL;
uintptr_t Coop::jmp_ret12 = NULL;


void Coop::toggle(bool enable) {
    if (enable) {
        devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\player\\uPlayerNero",
                                        MODE_ASYNC | MODE_USECACHE | MODE_QUALITY_HIGHEST);
        devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\player\\uPlayerDante",
                                        MODE_ASYNC | MODE_USECACHE | MODE_QUALITY_HIGHEST);
        install_patch_offset(0xDE86, patch1, "\xEB\x13", 2); //prevent pausing on pad disconnect
    } else {
        patch1.reset();
    }
}

void get_analog_level(sDevil4Pad* pad, kAnlg* anlg, bool r_side) {

    float anlg_x = pad->mPadInfo[0].mAnlg[r_side].x;
    float anlg_y = pad->mPadInfo[0].mAnlg[r_side].y;

    if ((0.24f <= abs(anlg_x)) || (0.24 <= abs(anlg_y))) {
        anlg->x = (ushort)(int)(anlg_x * 127.0f);
        anlg->y = (ushort)(int)(anlg_y * 127.0f);
    } else {
        anlg->x = 0;
        anlg->y = 0;
    }
}

void kb_to_analog(kAnlg* input, int char_id, int key_id) {
    uintptr_t sSavePtr = 0x00e558c8;
    sKeyboard* sKbPtr  = (sKeyboard*)0x00e559c0;

    short anlg         = input->y;
    short anlg_shift   = anlg >> 0x1f;
    if ((int)((anlg ^ anlg_shift) - anlg_shift) < 0x40) {
        if (char_id == 0) {
            anlg = *(uint*)(sSavePtr + 0x1b0 + key_id * 16);
        } else if (char_id == 1) {
            anlg = *(uint*)(sSavePtr + 0x298 + key_id * 16);
        } else {
            anlg = 0;
        }
        if ((((sKbPtr->mState).on[anlg >> 5] >> ((byte)anlg & 0x1f) & 1) != 0) ||
            ((*(byte*)((sKbPtr->mState).on + 3) & 0x40) != 0)) {
            input->x = 0x7f;
        }
        if (char_id == 0) {
            anlg = *(uint*)(sSavePtr + 0x1b4 + key_id * 16);
        } else if (char_id == 1) {
            anlg = *(uint*)(sSavePtr + 0x29c + key_id * 16);
        } else {
            anlg = 0;
        }
        if ((((sKbPtr->mState).on[anlg >> 5] >> ((byte)anlg & 0x1f) & 1) != 0) ||
            ((*(byte*)((sKbPtr->mState).on + 3) & 0x10) != 0)) {
            input->y = (0x3f < (short)(input->y)) - 1 & 0xff81;
        }        
    }

    anlg = input->x;
    anlg_shift = anlg >> 0x1f;
    if ((int)((anlg ^ anlg_shift) - anlg_shift) < 0x40) {
        if (char_id == 0) {
            anlg = *(uint*)(sSavePtr + 0x1bc + key_id * 16);
        } else if (char_id == 1) {
            anlg = *(uint*)(sSavePtr + 0x2a4 + key_id * 16);
        } else {
            anlg = 0;
        }
        if ((((sKbPtr->mState).on[anlg >> 5] >> ((byte)anlg & 0x1f) & 1) != 0) ||
            ((*(byte*)((sKbPtr->mState).on + 3) & 0x40) != 0)) {
            input->x = 0x7f;
        }
        if (char_id == 0) {
            anlg = *(uint*)(sSavePtr + 0x1b8 + key_id * 4);
        } else if (char_id == 1) {
            anlg = *(uint*)(sSavePtr + 0x2a0 + key_id * 4);
        } else {
            anlg = 0;
        }
        if ((((sKbPtr->mState).on[anlg >> 5] >> ((byte)anlg & 0x1f) & 1) != 0) ||
            ((*(byte*)((sKbPtr->mState).on + 3) & 0x10) != 0)) {
            input->x = (0x3f < (short)(input->x)) - 1 & 0xff81;
        }
    }
}

void __stdcall update_analog_info(kAnlg* anlg) {
    _asm {
            pushad
            mov esi, [anlg]
            call update_analog_info_call
            popad
    }
}

void __stdcall new_pad_update_func(cPeripheral* peri) {
    byte control_num = *(byte*)((uintptr_t)peri + 0x95);
    sDevil4Pad* pad;
    if (control_num < 4)
        pad = PadArr[control_num];
    peri->mPadBtnOn = pad->mPadInfo[0].mBtn.on;
    peri->mPadBtnRel = pad->mPadInfo[0].mBtn.rel;
    peri->mPadBtnTrg = pad->mPadInfo[0].mBtn.trg;
    memcpy(&peri->mPadBtnPress, &pad->mPadInfo[0].mPress, sizeof(float) * 15);

    get_analog_level(pad, &peri->mAnlgL, 0);
    kb_to_analog(&peri->mAnlgL, 0, 0);
    update_analog_info(&peri->mAnlgL);

    get_analog_level(pad, &peri->mAnlgR, 1);
    kb_to_analog(&peri->mAnlgR, 0, 1);
    update_analog_info(&peri->mAnlgR);
}

naked void detour1() {
    _asm {
            cmp byte ptr [Coop::mod_enabled], 1
            jne originalcode
            push [esp+4]
            call new_pad_update_func
            jmp handle
        originalcode:
            push ebp
            mov ebp,esp
            and esp,-0x08
            jmp [Coop::jmp_ret1]
        handle:
            ret 4
    }
}

sDevil4Pad* create_pad(int id) {
    sDevil4Pad* pad  = ((sDevil4Pad * (*)()) sDevil4Pad_cons)();
    pad->mPad->Info.Socket_no = id;
    //pad->mPad->Info.Pad_no = id;
    //pad->mPad->Info.Kind = 1;
    pad->mUserPadId = 0;
    return pad;
}

void __stdcall move_pad(sDevil4Pad* obj) {
    uintptr_t move_func = *(uintptr_t*)(*(uintptr_t*)obj + 0x18);
    _asm {
            mov ecx, [obj]
            call move_func
    }
}

void __stdcall update_pad_loop() {
    for (int i = 1; i < PadArr.size(); i++) {
        std::thread update([i]() { move_pad(PadArr[i]); });
        update.detach();
    }
}

naked void detour2() {
    _asm {
            cmp byte ptr [Coop::mod_enabled], 1
            jne originalcode
            pushad
            call update_pad_loop
            popad
        originalcode:
            mov ecx,[ebp+0x00010388]
            jmp [Coop::jmp_ret2]
    }
}

void* make_nero() {
    uintptr_t nero_cons = 0x007E1A70;
    void* nero_ptr = ((void* (*)())(nero_cons))();
    return nero_ptr;
}

void* make_dante() {
    uintptr_t dante_cons = 0x007B2130;
    void* dante_ptr      = ((void* (*)())(dante_cons))();
    return dante_ptr;
}

void* make_nero_hud() {
    uintptr_t nero_hud_cons = 0x00507A60;
    void* hud_ptr         = ((void* (*)())(nero_hud_cons))();
    return hud_ptr;
}

void* make_dante_hud() {
    uintptr_t dante_hud_cons = 0x00503210;
    void* hud_ptr            = ((void* (*)())(dante_hud_cons))();
    return hud_ptr;
}

void player_factory(uint char_id, uint player_num) {
    void* player;
    if (char_id == 0) {
        player = make_nero();
    } else {
        player = make_dante();
    }
    *(byte*)((uintptr_t)player + 0x1405) = player_num; //Pad ID (used for cPeripheral update)
    *(byte*)((uintptr_t)player + 0xE64) = player_num;  //Player ID
    if (PlayerArr.size() <= 4) {
        PlayerArr.push_back(player);
    }
    devil4_sdk::sUnit_spawn(player, 13);
    
    void* hud;
    if (char_id == 0) {
        hud = make_nero_hud();
    } else {
        hud = make_dante_hud();
    }
    if (HUDArr.size() <= 4) {
        HUDArr.push_back(hud);
        ((cUnit*)hud)->flags.bits.mTransView = (1 << (HUDArr.size() - 1));
    }
    devil4_sdk::sUnit_spawn(hud, 25);
}

void* make_cam(){ //make uCameraCtrl
    void* cam = ((void* (*)())(0x004F71B0))();
    devil4_sdk::sUnit_spawn(cam, 18);
    CamArr.push_back(cam);
    if (cam != CamArr[0])
        *(void**)((uintptr_t)cam + 0x3a0) = (void*)((uintptr_t)CamArr[0] + 0x3a0);
    return cam;
}

void* __stdcall get_pl_from_cam(void* cam) {
    uintptr_t ucam = *(uintptr_t*)((uintptr_t)cam + 0x6C);
    for (int i = 0; i < CamArr.size(); i++) {
        if (ucam == (uintptr_t)CamArr[i])
            return PlayerArr[i];
    }
    return 0;
}

void* __stdcall get_pad_from_cam(void* cam) {
    uintptr_t ucam = *(uintptr_t*)((uintptr_t)cam + 0x6C);
    for (int i = 0; i < CamArr.size(); i++) {
        if (ucam == (uintptr_t)CamArr[i])
            return PadArr[i];
    }
    return 0;
}


naked void detour3() {
    _asm {
            cmp byte ptr [Coop::mod_enabled],1
            jne originalcode

            push edi
            call get_pl_from_cam

            test eax, eax
            je originalcode

            jmp handle
        originalcode:
            mov eax, [esi+0x24]
        handle:
            test eax,eax
            jmp [Coop::jmp_ret3]
    }
}

naked void detour4() {
    _asm {
            cmp byte ptr [Coop::mod_enabled],1
            jne originalcode

            push eax
            push esi
            call get_pad_from_cam

            test eax, eax
            je handle2

            mov edi, eax
            pop eax

            jmp handle
        originalcode:
            mov edi, dword ptr [sDevil4Pad_ptr]
            mov edi, [edi]
        handle:
            jmp [Coop::jmp_ret4]
        handle2:
            pop eax
            jmp originalcode
    }
}

void __stdcall setup_cam(void* cam) {
    sCamera* s_cam = *(sCamera**)sCamera_ptr;
    for (int i = 1; i < CamArr.size(); i++) {
        if (cam == CamArr[i]) {
            sCamera_ViewPort* vp = &s_cam->viewports[i];
            vp->mpCamera = cam;
            vp->mAttr = 0x17;
            return;
        }
    }
}

naked void detour5() {
    _asm {
            cmp byte ptr [Coop::mod_enabled],1
            jne originalcode

            mov ecx,[sMediator_ptr]
            mov ecx,[ecx]
            mov ecx,[ecx+0xD0]
            
            test ecx,ecx
            je originalcode

            cmp ecx, esi
            je originalcode

            pushad
            push esi
            call setup_cam
            popad
            
            jmp [uCameraCtrl_setup_end]

        originalcode:
            mov ecx,[0x00E552D4]
            mov ecx,[ecx]
            jmp [Coop::jmp_ret5]
    }
}

naked void detour6() {
    _asm {
            cmp byte ptr [Coop::mod_enabled],1
            jne originalcode

            push esi
            call get_pad_from_cam

            test eax, eax
            je originalcode

            jmp handle
        originalcode:
            mov eax,[sDevil4Pad_ptr]
            mov eax,[eax]
        handle:
            jmp [Coop::jmp_ret6]
    }
}

void* __stdcall get_cam_from_pl(void* player) {
    byte id = *(byte*)((uintptr_t)player + 0x1405);
    if (!CamArr.empty())
        return CamArr[id];
    else return 0;
}

naked void detour7() {
    _asm {
            cmp byte ptr [Coop::mod_enabled],1
            jne originalcode

            push esi
            call get_cam_from_pl

            test eax,eax
            je handle2

            jmp handle
        originalcode:
            mov eax, dword ptr [eax + 0xd0]
        handle:
            jmp [Coop::jmp_ret7]
        handle2:
            mov eax,[sMediator_ptr]
            mov eax,[eax]
            jmp originalcode
    }
}

naked void detour8() {
    _asm {
            cmp byte ptr [Coop::mod_enabled],1
            jne originalcode

            push ebx
            call get_pl_from_cam

            jmp handle
        originalcode:
            mov eax, dword ptr [esi+0x24]
        handle:
            test eax,eax
            jmp [Coop::jmp_ret8]
    }
}

naked void detour9() {
    _asm {
            cmp byte ptr [Coop::mod_enabled],1
            jne originalcode

            push ebx
            call get_pl_from_cam

            jmp handle
        originalcode:
            mov eax, dword ptr [esi+0x24]
        handle:
            test eax,eax
            jmp [Coop::jmp_ret9]
    }
}

naked void detour10() {
    _asm {
            cmp byte ptr [Coop::mod_enabled],1
            jne originalcode

            push ebx
            call get_pl_from_cam

            jmp handle
        originalcode:
            mov eax, dword ptr [esi+0x24]
        handle:
            test eax,eax
            jmp [Coop::jmp_ret10]
    }
}

void* __stdcall get_pl_from_hud(void* hud) {
    for (int i = 1; i < HUDArr.size(); i++) {
        if (hud == HUDArr[i])
            return PlayerArr[i];
    }
    return 0;
}

naked void detour11() {//Nero HUD
    _asm {
            cmp byte ptr [Coop::mod_enabled],1
            jne originalcode

            push edi
            mov edi,eax

            push esi
            call get_pl_from_hud

            test eax,eax
            je handle2

            jmp handle

        originalcode:
            mov eax, dword ptr [eax+0x24]
            test eax, eax
            jmp [Coop::jmp_ret11]

        handle:
            pop edi
            test eax,eax
            jmp [Coop::jmp_ret11]

        handle2:
            mov eax,edi
            pop edi
            jmp originalcode
    }
}

naked void detour12() {//Dante HUD
    _asm {
            cmp byte ptr [Coop::mod_enabled],1
            jne originalcode

            mov edi,eax

            push esi
            call get_pl_from_hud

            test eax,eax
            je handle2

            jmp handle

        originalcode:
            mov ecx, dword ptr [eax+0x24]
            xor edi, edi
            jmp [Coop::jmp_ret12]

        handle:
            mov ecx,eax
            xor edi,edi
            jmp [Coop::jmp_ret12]

        handle2:
            mov eax,edi
            jmp originalcode
    }
}

std::optional<std::string> Coop::on_initialize() {
    if (!install_hook_offset(0x3AFD10, hook1, &detour1, &jmp_ret1, 6)) { //replace player pad update func
        spdlog::error("Failed to init Coop mod1\n");
        return "Failed to init Coop mod1";
    }

    if (!install_hook_offset(0x4AEFA1, hook2, &detour2, &jmp_ret2, 6)) { //call sPad move loop
        spdlog::error("Failed to init Coop mod2\n");
        return "Failed to init Coop mod2";
    }

    std::unique_ptr<FunctionHook> temp_hook3, temp_hook4, temp_hook5;//hook3 & 5 fall out of scope somehow ?!?!

    if (!install_hook_offset(0x19189, hook3, &detour3, &jmp_ret3, 5)) { //Delock cam player pos
        spdlog::error("Failed to init coop mod3\n");
        return "Failed to init coop mod3";
    }

    if (!install_hook_offset(0x22449, hook4, &detour4, &jmp_ret4, 6)) { //Cam pad Y-axis control
        spdlog::error("Failed to init Coop mod4\n");
        return "Failed to init Coop mod4";
    }

    if (!install_hook_offset(0xF75DA, hook5, &detour5, &jmp_ret5, 6)) { //Cam setup
        spdlog::error("Failed to init Coop mod5\n");
        return "Failed to init Coop mod5";
    }

    if (!install_hook_absolute(0x421e14, hook6, &detour6, &jmp_ret6, 5)) { // Cam pad X-axis control
        spdlog::error("Failed to init Coop mod4\n");
        return "Failed to init Coop mod4";
    }

    if (!install_hook_absolute(0x07abcbb, hook7, &detour7, &jmp_ret7, 6)) { // Input relative to cam
        spdlog::error("Failed to init Coop mod4\n");
        return "Failed to init Coop mod4";
    }

    if (!install_hook_absolute(0x419857, hook8, &detour8, &jmp_ret8, 5)) { // Lockon cam
        spdlog::error("Failed to init Coop mod4\n");
        return "Failed to init Coop mod4";
    }

    if (!install_hook_absolute(0x41a1de, hook9, &detour9, &jmp_ret9, 5)) { // Lockon cam
        spdlog::error("Failed to init Coop mod4\n");
        return "Failed to init Coop mod4";
    }

    if (!install_hook_absolute(0x041a286, hook10, &detour10, &jmp_ret10, 5)) { // Lockon cam
        spdlog::error("Failed to init Coop mod4\n");
        return "Failed to init Coop mod4";
    }

    if (!install_hook_absolute(0x508d82, hook11, &detour11, &jmp_ret11, 5)) { // Nero HUD
        spdlog::error("Failed to init Coop mod4\n");
        return "Failed to init Coop mod4";
    }

    //if (!install_hook_absolute(0x506ce4, hook12, &detour12, &jmp_ret12, 5)) { // Dante HUD
    //    spdlog::error("Failed to init Coop mod4\n");
    //    return "Failed to init Coop mod4";
    //}


    //std::thread init([]() {
    //    while (*sDevil4Pad_ptr == nullptr)
    //        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    //    PadArr.push_back(*sDevil4Pad_ptr);
    //    for (int i = 1; i < 4; i++) {
    //        PadArr.push_back(create_pad(i));
    //    }
    //});
    //init.detach();
    return Mod::on_initialize();
}

void Coop::on_frame(fmilliseconds& dt) {
    //if (mod_enabled) {
    //    for (int i = 1; i < PadArr.size(); i++) {
    //        std::thread update([i]() { move_pad(PadArr[i]); });
    //        update.detach();
    //    }
    //}

    if (mod_enabled) {
        void* player   = devil4_sdk::get_local_player();
        void* camera = devil4_sdk::get_local_camera();
        sUnit* s_unit  = devil4_sdk::get_sUnit();
        MoveLine* hud_ml = &s_unit->mMoveLine[25];

        if ((PlayerArr.empty()) && (player != nullptr)) {
            PlayerArr.push_back(player);
        }
        if ((CamArr.empty()) && (camera != nullptr)) {
            CamArr.push_back(camera);
        }

        if ((HUDArr.empty()) && (hud_ml->mTop != nullptr)) {
            HUDArr.push_back((void*)hud_ml->mTop);
            cUnit* hud = (cUnit*)hud_ml->mTop;
            hud->flags.bits.mTransView = 1;
        }

        if ((!PlayerArr.empty()) && (player == nullptr)) {
            PlayerArr.clear();
        }

        if ((!CamArr.empty()) && (camera == nullptr))
            CamArr.clear();

        if ((!HUDArr.empty()) && (hud_ml->mTop == nullptr))
            HUDArr.clear();

    }
}

void Coop::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Coop mode"), &mod_enabled)) {
        toggle(mod_enabled);
        if (CharSwitcher::mod_enabled)
            CharSwitcher::mod_enabled = false;
        //PadArr.push_back(*sDevil4Pad_ptr);
        //for (int i = 1; i < 4; i++)
        //    PadArr.push_back(create_pad(i));
        PadArr.push_back(*sDevil4Pad_ptr);
        for (int i = 1; i < 4; i++) {
            PadArr.push_back(create_pad(i));
        }
    }
    if (ImGui::Button("Spawn Nero")) {
        player_factory(0, 1);
        make_cam();
        sCamera* s_cam = *(sCamera**)sCamera_ptr;
        s_cam->viewports[0].mMode = REGION_LEFT;
        s_cam->viewports[1].mMode = REGION_RIGHT;
    }
    if (ImGui::Button("Spawn Dante")) {
        player_factory(1, 1);
        make_cam();
        sCamera* s_cam = *(sCamera**)sCamera_ptr;
        s_cam->viewports[0].mMode = REGION_LEFT;
        s_cam->viewports[1].mMode = REGION_RIGHT;
    }
}
