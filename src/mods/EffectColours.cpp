#include "EffectColours.hpp"

bool      EffectColours::mod_enabled{ false };
uintptr_t EffectColours::jmp_ret{ NULL };
int8_t coloursPicked[4]{(int8_t)255, (int8_t)255, (int8_t)0, (int8_t)255}; // bgra
float coloursPickedFloat[4]{0.0f, 1.0f, 1.0f, 1.0f}; //rgba

naked void detour(void) {
	_asm {
		cmp byte ptr [EffectColours::mod_enabled], 0
		je originalcode

		cmp dword ptr [esi+0x3B], 925982514 // '2_17'
		jne originalcode
		mov eax, dword ptr [coloursPicked]
		jmp cont

	originalcode:
		mov eax, [esi]
	cont:
		push ebx
		mov bl, [esp+0x0C]
		jmp dword ptr [EffectColours::jmp_ret]
	}
}

std::optional<std::string> EffectColours::on_initialize() {
	if (!install_hook_offset(0x5801B1, hook, &detour, &jmp_ret, 7)) {
		spdlog::error("Failed to init EffectColours mod\n");
		return "Failed to init EffectColours mod";
	}
	return Mod::on_initialize();
}

void EffectColours::on_gui_frame() {
	if (ImGui::CollapsingHeader(_("Colours"))) {
		ImGui::Checkbox(_("Custom Colours"), &mod_enabled);
		ImGui::SameLine();
		help_marker(_("I haven't figured this out yet"));
		if (ImGui::ColorPicker4(_("Colours"), coloursPickedFloat)) {
			coloursPicked[2] = (int8_t)(coloursPickedFloat[0] * 255.0f); // Red
			coloursPicked[1] = (int8_t)(coloursPickedFloat[1] * 255.0f); // Green
			coloursPicked[0] = (int8_t)(coloursPickedFloat[2] * 255.0f); // Blue
			coloursPicked[3] = (int8_t)(coloursPickedFloat[3] * 255.0f); // Alpha
		}
	}
}

void EffectColours::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("effect_colours_enable").value_or(false);
	coloursPickedFloat[2] = cfg.get<float>("coloursPickedFloat[2]").value_or(0.0f);
	coloursPickedFloat[1] = cfg.get<float>("coloursPickedFloat[1]").value_or(1.0f);
	coloursPickedFloat[0] = cfg.get<float>("coloursPickedFloat[0]").value_or(1.0f);
	coloursPickedFloat[3] = cfg.get<float>("coloursPickedFloat[3]").value_or(1.0f);
	coloursPicked[2] = (int8_t)(coloursPickedFloat[2] * 255.0f); // Red
	coloursPicked[1] = (int8_t)(coloursPickedFloat[1] * 255.0f); // Green
	coloursPicked[0] = (int8_t)(coloursPickedFloat[0] * 255.0f); // Blue
	coloursPicked[3] = (int8_t)(coloursPickedFloat[3] * 255.0f); // Alpha
};

void EffectColours::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("effect_colours_enable", mod_enabled);
	cfg.set<float>("coloursPickedFloat[2]", coloursPickedFloat[2]);
	cfg.set<float>("coloursPickedFloat[1]", coloursPickedFloat[1]);
	cfg.set<float>("coloursPickedFloat[0]", coloursPickedFloat[0]);
	cfg.set<float>("coloursPickedFloat[3]", coloursPickedFloat[3]);
};
