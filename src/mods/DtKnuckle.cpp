#include "DtKnuckle.hpp"

bool DtKnuckle::mod_enabled{false};
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

uintptr_t DtKnuckle::jmp_ret1{NULL};
	int moveID                        = 0x31C;
	float inputCooldown               = 0.0f;
	bool spectreFlag                  = 0;
	bool endFlag                      = 0;
	int16_t desiredInput              = 0;
	int16_t previousInput             = 0;
	constexpr uintptr_t detour1_call1 = 0x00829BE0;
uintptr_t DtKnuckle::jmp_ret2{NULL};
uintptr_t DtKnuckle::jmp_ret3{NULL};
	constexpr uintptr_t detour3_conditional1 = 0x0082A9D5;
uintptr_t DtKnuckle::jmp_ret4{NULL};
	constexpr uintptr_t detour4_conditional1 = 0x00829671;
uintptr_t DtKnuckle::jmp_ret5{NULL};
	constexpr uintptr_t detour5_jmp1 = 0x0082A8E5;
	constexpr uintptr_t detour5_mov1 = 0x00E1657C;
uintptr_t DtKnuckle::jmp_ret6{NULL};
uintptr_t DtKnuckle::jmp_ret7{NULL};
uintptr_t DtKnuckle::jmp_ret8{NULL};
uintptr_t DtKnuckle::jmp_ret9{NULL};
uintptr_t DtKnuckle::jmp_ret10{NULL};
	constexpr uintptr_t detour10_conditional1 = 0x007E6E8D;
uintptr_t DtKnuckle::jmp_ret11{NULL};
	float decrement = 30.0f;
uintptr_t DtKnuckle::jmp_ret12{NULL};
	constexpr uintptr_t detour12_conditional1 = 0x00829BD8;
uintptr_t DtKnuckle::jmp_ret13{NULL};

void DtKnuckle::toggle(bool enable) {
	if (enable) {
		// dt check nop 1
		install_patch_offset(0x42A84F, patch1, "\x90\x90\x90\x90\x90\x90", 6);

		// dt check nop 2 I assume
		install_patch_offset(0x42A8AC, patch2, "\x90\x90\x90\x90\x90\x90", 6);

		// DT appearance bypass
		install_patch_offset(0x42908B, patch3, "\xEB\x08", 2);
		install_patch_offset(0x4290D0, patch4, "\xEB\x08", 2);
		install_patch_offset(0x428FD9, patch5, "\xEB\x08", 2);
		install_patch_offset(0x428EEE, patch6, "\xEB\x08", 2);
		install_patch_offset(0x428E8B, patch7, "\xEB\x08", 2);

		// Fix height reset on landing patch
		install_patch_offset(0x428E00, patch8, "\x90\x90\x90\x90\x90", 5);

		// Enable efx for guardian
		install_patch_offset(0x4298D3, patch9, "\x90\x90", 2);
		install_patch_offset(0x4299B9, patch10, "\x90\x90", 2);

		// Timely spectre culling
		install_patch_offset(0x429B40, patch11, "\xEB\x2E", 2);
	} else {
		patch1.reset();
		patch2.reset();
		patch3.reset();
		patch4.reset();
		patch5.reset();
		patch6.reset();
		patch7.reset();
		patch8.reset();
		patch9.reset();
		patch10.reset();
		patch11.reset();
	}
}

// Input check and initialize spectre
naked void detour1(void) {
	_asm {
			cmp byte ptr [DtKnuckle::mod_enabled], 0
			je originalcode

			pushad
			mov ebp, [static_mediator_ptr]
			mov ebp, [ebp]
			mov ebp, [ebp+0x24]
			cmp ebp, edx // compare sMediator player to register player
			jne handler
			cmp dword ptr [ebp], 0xBE4FA0 // is Nero ?
			jne handler

			movss xmm0, [inputCooldown] // reduce input cooldown
			subss xmm0, [edx+0x10] // delta
			movss [inputCooldown], xmm0

			mov eax, [ebp+0x1374] // raw controller input
			//test eax, eax
			//je handler3 // no input, jump out

			cmp dword ptr [inputCooldown], 0
			jg handler3 // cooldown has value != 0, jump out
			
			test ax, [desiredInput]
			je handler3 // incorrect input, jump out
			cmp ax, [previousInput]
			je handler3 // stale input, jump out

			mov dword ptr [inputCooldown], 0x42480000 // 50.0f // set input cooldown

		// ForwardCheck:
			mov eax, [ebp+0x21CC]
			cmp al, 01
			jne BackCheck
			mov dword ptr [moveID], 0x317
			jmp SpawnSpectre

		BackCheck:
			mov eax, [ebp+0x21E0]
			cmp al, 01
			jne SpawnSpectre
			mov dword ptr [moveID], 0x330
		SpawnSpectre:
			mov edi, [ebp+0xCDF8]
			mov eax, [edi+0x22A8]
			cmp eax, 0x7
			je handler
			mov eax, 0x00000004
			mov esi, [moveID]
			mov [esp+0x0C], esi
			mov dword ptr [esp+0x10], 0x04
			movss [esp+0x14], xmm0
			mov byte ptr [spectreFlag], 0x01
			call detour1_call1
			mov eax, [ebp+0x0000CDF8]
			mov ebx, [eax+0x1370]
			mov dword ptr [ebx+0x14F0], 0x0C
			movq xmm0, qword ptr [esp+0x0C]
			mov ecx, [esp+0x14]
			add eax, 0x000022B4
			movq qword ptr [eax], xmm0
			mov [eax+0x08], ecx
			popad
			jmp code
				
		handler3:
			mov [previousInput], ax
		handler:
			popad
		code:
			mov dword ptr [moveID], 0x0000031C
		originalcode:
			lea edi, [esp+0x14]
			mov ecx, esi
		// jmp_ret:
			jmp dword ptr [DtKnuckle::jmp_ret1]
	}
}

// better flag remove
naked void detour2(void) {
	_asm {
			cmp byte ptr [DtKnuckle::mod_enabled], 0
			je originalcode

			push eax
			xor eax, eax // because mov al
			mov al, [spectreFlag]
			test eax, eax
			je handler
			mov byte ptr [spectreFlag], 00
			mov byte ptr [endFlag], 1
			jmp originalcode // we skip popping eax here not sure if correct

		handler:
			pop eax
		originalcode:
			mov dword ptr [esi+0x000022C4],00000000
			jmp dword ptr [DtKnuckle::jmp_ret2]
	}
}

// animation check bypass
naked void detour3(void) {
	_asm {
			cmp byte ptr [DtKnuckle::mod_enabled], 0
			je originalcode

			cmp byte ptr [spectreFlag], 01
			jne originalcode
			mov al, 01
		originalcode:
			test al, al
			je jmp_je
		// jmp_ret:
			jmp dword ptr [DtKnuckle::jmp_ret3]

		jmp_je:
			jmp dword ptr [detour3_conditional1]
	}
}

// motion check bypass
naked void detour4(void) {
	_asm {
			cmp byte ptr [DtKnuckle::mod_enabled], 0
			je originalcode

			cmp byte ptr [spectreFlag], 01
			jne originalcode
			mov al, 01
		originalcode:
			test al, al
			jne jmp_jne
			cmp dword ptr [esi+0x000022A8], 00
		// jmp_ret:
			jmp dword ptr [DtKnuckle::jmp_ret4]

		jmp_jne:
			jmp dword ptr [detour4_conditional1]
	}
}
// Disjoint
naked void detour5(void) {
	_asm {
			cmp byte ptr [DtKnuckle::mod_enabled], 0
			je originalcode

			push eax
			xor eax, eax // because mov al
			mov al, [spectreFlag]
			test eax, eax
			je handler
			pop eax
			jmp jmp_jmp1
			
		handler:
			pop eax
		originalcode:
			cmp eax, esi
			je jmp_jmp1
			mov eax, [ecx*0x4+detour5_mov1] // might need 0x00E1657C
		// jmp_ret:
			jmp dword ptr [DtKnuckle::jmp_ret5]

		jmp_jmp1:
			jmp dword ptr [detour5_jmp1]
	}
}

// Fix height reset on landing
naked void detour6(void) {
	_asm {
			cmp byte ptr [DtKnuckle::mod_enabled], 0
			je originalcode

			fld dword ptr [esi+0x34]
			fstp dword ptr [esi+0x34]
			jmp jmp_ret

		originalcode:
			fld dword ptr [eax+0x34]
			fstp dword ptr [esi+0x34]
		jmp_ret:
			jmp dword ptr [DtKnuckle::jmp_ret6]
	}
}

// rotation fix 1
naked void detour7(void) {
	_asm {
			cmp byte ptr [DtKnuckle::mod_enabled], 0
			je originalcode

			push eax
			xor eax, eax
			mov al, [spectreFlag]
			test eax,eax
			je handler
			pop eax
			jmp jmp_ret

		handler:
			pop eax
			movss [edi+0x44], xmm1
			jmp jmp_ret

		originalcode:
			movss [edi+0x44], xmm1
		jmp_ret:
			jmp dword ptr [DtKnuckle::jmp_ret7]
	}
}

// rotation fix 2
naked void detour8(void) {
	_asm {
			cmp byte ptr [DtKnuckle::mod_enabled], 0
			je originalcode

			push eax
			xor eax, eax
			mov al, [spectreFlag]
			test eax,eax
			je handler
			pop eax
			jmp jmp_ret

		handler:
			pop eax
			movss [esi+0x44], xmm1
			jmp jmp_ret

		originalcode:
			movss [esi+0x44], xmm1
		jmp_ret:
			jmp dword ptr [DtKnuckle::jmp_ret8]
	}
}

// rotation fix 3
naked void detour9(void) {
	_asm {
			cmp byte ptr [DtKnuckle::mod_enabled], 0
			je originalcode

			fld dword ptr [edx+0x44]
			fstp dword ptr [edi+0x44]
			jmp jmp_ret

		originalcode:
			fld dword ptr [eax+0x44]
			fstp dword ptr [edi+0x44]
		jmp_ret:
			jmp dword ptr [DtKnuckle::jmp_ret9]
	}
}

// Prevent DT moves from cancelling guardian
naked void detour10(void) {
	_asm {
			cmp byte ptr [DtKnuckle::mod_enabled], 0
			je originalcode

			push ecx
			xor ecx, ecx
			mov cl, [spectreFlag]
			test ecx, ecx
			je handler
			pop ecx
			mov al, 0
			jmp originalcode

		handler:
			pop ecx
		originalcode:
			cmp al, 01
			jne jmp_jne
			push edi
		// jmp_ret:
			jmp dword ptr [DtKnuckle::jmp_ret10]
		jmp_jne:
			jmp dword ptr [detour10_conditional1]
	}
}

// Decrease height
naked void detour11(void) {
	_asm {
			cmp byte ptr [DtKnuckle::mod_enabled], 0
			je originalcode

			push eax
			xor eax, eax
			mov al,[spectreFlag]
			test eax, eax
			je handler
			pop eax
			movss xmm1, [edi+0x34]
			subss xmm1, [decrement]
			movss [edi+0x34], xmm1
			jmp originalcode

		handler:
			pop eax
		originalcode:
			fld dword ptr [edx+0x00001358]
		// jmp_ret:
			jmp dword ptr [DtKnuckle::jmp_ret11]
	}
}

// Prevent guardian visible time from being extended
naked void detour12(void) {
	_asm {
			cmp byte ptr [DtKnuckle::mod_enabled], 0
			je originalcode

			push ecx
			mov ecx,[edi+0x22A8]
			cmp ecx, 0x4
			jne handler
			xor ecx, ecx // for cl
			mov cl, [endFlag]
			cmp ecx, 1
			jne handler
			mov ecx, [edi+0x22C4]
			cmp ecx, 00
			jne handler
			pop ecx
			xor eax, eax
			jmp originalcode

		handler:
			pop ecx
		originalcode:
			test eax, eax
			je jmp_je
			lea esp, [esp+0x00000000]
		// jmp_ret:
			jmp dword ptr [DtKnuckle::jmp_ret12]
		jmp_je:
			jmp dword ptr [detour12_conditional1]
	}
}

// remove endFlag
naked void detour13(void) {
	_asm {
			cmp byte ptr [DtKnuckle::mod_enabled], 0
			je originalcode

			mov byte ptr [endFlag],00
		originalcode:
			mov [edi+0x000022A8],ebx
		// jmp_ret:
			jmp dword ptr [DtKnuckle::jmp_ret13]
	}
}

std::optional<std::string> DtKnuckle::on_initialize() {
	if (!install_hook_offset(0x3A92BF, hook1, &detour1, &jmp_ret1, 6)) {
		spdlog::error("Failed to init DtKnuckle mod\n");
		return "Failed to init DtKnuckle mod";
	}
	if (!install_hook_offset(0x4297D1, hook2, &detour2, &jmp_ret2, 10)) {
		spdlog::error("Failed to init DtKnuckle mod2\n");
		return "Failed to init DtKnuckle mod2";
	}
	if (!install_hook_offset(0x42A841, hook3, &detour3, &jmp_ret3, 8)) {
		spdlog::error("Failed to init DtKnuckle mod3\n");
		return "Failed to init DtKnuckle mod3";
	}
	if (!install_hook_offset(0x429650, hook4, &detour4, &jmp_ret4, 11)) {
		spdlog::error("Failed to init DtKnuckle mod4\n");
		return "Failed to init DtKnuckle mod4";
	}
	if (!install_hook_offset(0x42A8D0, hook5, &detour5, &jmp_ret5, 11)) {
		spdlog::error("Failed to init DtKnuckle mod5\n");
		return "Failed to init DtKnuckle mod5";
	}
	if (!install_hook_offset(0x428E19, hook6, &detour6, &jmp_ret6, 6)) {
		spdlog::error("Failed to init DtKnuckle mod6\n");
		return "Failed to init DtKnuckle mod6";
	}
	if (!install_hook_offset(0x5FB7F7, hook7, &detour7, &jmp_ret7, 5)) {
		spdlog::error("Failed to init DtKnuckle mod7\n");
		return "Failed to init DtKnuckle mod7";
	}
	if (!install_hook_offset(0x428E6F, hook8, &detour8, &jmp_ret8, 5)) {
		spdlog::error("Failed to init DtKnuckle mod8\n");
		return "Failed to init DtKnuckle mod8";
	}
	if (!install_hook_offset(0x42A91A, hook9, &detour9, &jmp_ret9, 6)) {
		spdlog::error("Failed to init DtKnuckle mod9\n");
		return "Failed to init DtKnuckle mod9";
	}
	if (!install_hook_offset(0x3E6E2C, hook10, &detour10, &jmp_ret10, 5)) {
		spdlog::error("Failed to init DtKnuckle mod10\n");
		return "Failed to init DtKnuckle mod10";
	}
	if (!install_hook_offset(0x42A8FD, hook11, &detour11, &jmp_ret11, 6)) {
		spdlog::error("Failed to init DtKnuckle mod11\n");
		return "Failed to init DtKnuckle mod11";
	}
	if (!install_hook_offset(0x429BB5, hook12, &detour12, &jmp_ret12, 11)) {
		spdlog::error("Failed to init DtKnuckle mod12\n");
		return "Failed to init DtKnuckle mod12";
	}
	if (!install_hook_offset(0x42AA1E, hook13, &detour13, &jmp_ret13, 6)) {
		spdlog::error("Failed to init DtKnuckle mod13\n");
		return "Failed to init DtKnuckle mod13";
	}
	return Mod::on_initialize();
}

void DtKnuckle::on_gui_frame() {
	if (ImGui::Checkbox("Guardian Devil", &mod_enabled)) {
		toggle(mod_enabled);
	}
	ImGui::SameLine();
	help_marker("Triggers a stand attack when you input the selected button.\nLockon+forward/back for other attacks");
	ImGui::SameLine(sameLineWidth);
	ImGui::PushItemWidth(sameLineItemWidth);
	if (ImGui::BeginCombo("Guardian Input", devil4_sdk::getButtonInfo(desiredInput).second)) {
		for (const auto& buttonPair : buttonPairs) {
			bool is_selected = (desiredInput == buttonPair.first);
			if (ImGui::Selectable(buttonPair.second, is_selected)) {
				desiredInput = buttonPair.first;
			}
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();
}

void DtKnuckle::on_config_load(const utility::Config& cfg) {
	mod_enabled = cfg.get<bool>("dt_knuckle").value_or(false);
	toggle(mod_enabled);
	desiredInput = cfg.get<int16_t>("knuckle_input").value_or(0x100); // L1 default
}

void DtKnuckle::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("dt_knuckle", mod_enabled);
	cfg.set<int16_t>("knuckle_input", desiredInput);
}
