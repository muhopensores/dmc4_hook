// include your mod header file
#ifdef DEVELOPER
#include "ShaderEditor.hpp"
#include "d3dx9shader.h"
// TODO(): this shit was ported from dmc3 to do some visual debugging on frots
// it's basically useless. You'd need to get the address returned by CreatePixelShader,
// search that pointer in cheat engine, there would be a bunch of results, the proper one
// is a big array of d3d9 pixel shaders, can be checked with an access bp on that location.
// swap it with whatever address shader compiler gives out and the game would render with your custom one.
// draw commands seem to use POD structs so no MTFramework DTIs.
// this also leaks memory

static uintptr_t  jmp_return { NULL };
static uintptr_t  jmp_return2 { NULL };
static bool mod_enabled{ false };
//uintptr_t  ModShaderEditor::some_shared_ptr{ NULL };

class sRenderStub
{
public:
    char pad_0000[52]; //0x0000
    IDirect3DDevice9* D3D9Device; //0x0034
}; //Size: 0x0038
static_assert(sizeof(sRenderStub) == 0x38);

// dont care lmaoo
class cMaterial
{
public:
    char pad_0000[32]; //0x0000
    float N000077A1; //0x0020
    char pad_0024[16]; //0x0024
    float green; //0x0034
    char pad_0038[8]; //0x0038
}; //Size: 0x0040
static_assert(sizeof(cMaterial) == 0x40);

std::vector<cMaterial*> g_mats;
std::mutex g_i_mutex;  // protects g_i
bool g_is_capturing = false;
cMaterial* g_arg = 0;


// this is material hook
void wew() {
    if (g_arg && g_is_capturing) {
        const std::lock_guard<std::mutex> lock(g_i_mutex);
        g_mats.push_back(g_arg);
    }
}

// detour 
// naked is defined to __declspec(naked)
naked void detour() {
	__asm {
        mov [ecx],00C087DCh
        mov [g_arg], ecx
        pushad
        call wew
        popad
        jmp dword ptr [jmp_return]
	}
}

void ShaderEditor::create_shader_program(const char* shader_program) {
    IDirect3DPixelShader9* shader;
    HRESULT result;
    LPD3DXBUFFER codeBuffer;
    LPD3DXBUFFER errorBuffer;

    sRenderStub** sRenderPtr = (sRenderStub**)0x00E552D8;
    sRenderStub* sRender = *sRenderPtr;
    auto device = sRender->D3D9Device;

    result = D3DXAssembleShader( shader_program, strlen(shader_program), NULL, NULL, 0, &codeBuffer, &errorBuffer );
    if( errorBuffer != NULL )
    {
        const char* errorMessage = (const char*)errorBuffer->GetBufferPointer();
        sprintf(m_error_string, "Pixel shader failed to compile: %s", errorMessage);
        printf("%s\n", m_error_string);
        return;
    }
    else if( FAILED(result) )
    {
        sprintf(m_error_string, "Pixel shader failed to compile\n");
        printf(m_error_string);
        return;
    }

    result = device->CreatePixelShader( (DWORD*)codeBuffer->GetBufferPointer(), &shader );
    codeBuffer->Release();

    if( result != D3D_OK )
    {
        sprintf(m_error_string, "Failed to allocate pixel shader.\n");
        printf(m_error_string);
        return;
    }

    IDirect3DPixelShader9** pShaderLoc= &(m_shader_loc);
    InterlockedCompareExchange((LONG*)(pShaderLoc), (LONG)shader, (LONG)(m_shader_loc));
    //memcpy(px_shader_array_static->shaders[0]->pShader, shader, sizeof(void*));
    return;
}

std::optional<std::string> ShaderEditor::on_initialize() {
    if (!install_hook_absolute(0x00A61640, hook, &detour, &jmp_return, 8))
    {
        spdlog::error("Failed to init sheder mod\n");
        return "Failed to init sheder mod";
    }
    const char* default_shader = R"(
ps_3_0
    def c12, 0, 1, -1, -0
    def c13, 0.25, 0.5, 0, 0
    dcl_texcoord v0
    dcl_texcoord1_pp v1.xyz
    dcl_texcoord2 v2
    dcl_texcoord3 v3
    dcl_texcoord4 v4
    dcl_2d s0
    dcl_2d s1
    dcl_2d s2
    add r0, c9.z, -v0.w
    texkill r0
    nrm_pp r0.xyz, v1
    dp3_pp r0.x, r0, -c7
    if_lt v0.w, c9.x
      rcp r0.y, v2.w
      mul r0.zw, r0.y, v2.xyxy
      mad r1.xy, v2, r0.y, -c8.x
      mov r1.z, c12.x
      texldl r2, r1.xyzz, s0
      mov r1.w, r0.w
      texldl r3, r1.xwzz, s0
      mad r4.y, v2.y, r0.y, c8.x
      mov r4.xzw, r1.xyzz
      texldl r5, r4, s0
      mov r1.x, r0.z
      texldl r6, r1.xyzz, s0
      texldl r7, r1.xwzz, s0
      mov r4.xzw, r1.xyzz
      texldl r8, r4, s0
      mad r1.x, v2.x, r0.y, c8.x
      texldl r9, r1.xyzz, s0
      texldl r10, r1.xwzz, s0
      mov r4.xzw, r1.xyzz
      texldl r1, r4, s0
      mov r2.y, r3.x
      mov r2.z, r5.x
      mad r1.yzw, v2.z, r0.y, -r2.xxyz
      cmp_pp r2.xyz, r1.yzww, c12.y, c12.x
      mov r6.y, r7.x
      mov r6.z, r8.x
      mad r3.xyz, v2.z, r0.y, -r6
      cmp_pp r3.xyz, r3, c12.y, c12.x
      mov r9.y, r10.x
      mov r9.z, r1.x
      mad r4.xyz, v2.z, r0.y, -r9
      cmp_pp r4.xyz, r4, c12.y, c12.x
      mul r0.yz, r0.xzww, c8.y
      frc r0.yz, r0
      add_pp r2.xyz, r2, r3
      cmp_pp r1.xyz, r1.yzww, c12.z, c12.w
      add_pp r1.xyz, r4, r1
      mad_pp r1.xyz, r1, r0.y, r2
      add_pp r0.y, r1.y, r1.x
      add_pp r0.w, -r1.x, r1.z
      mad r0.y, r0.w, r0.z, r0.y
      mul_pp r0.y, r0.y, c13.x
      mov_pp r0.z, c6.w
    else
      if_lt v0.w, c9.y
        rcp r0.w, v3.w
        mul r1.xy, r0.w, v3
        mad r2.xy, v3, r0.w, -c8.x
        mov r2.z, c12.x
        texldl r3, r2.xyzz, s1
        mov r2.w, r1.y
        texldl r4, r2.xwzz, s1
        mad r5.y, v3.y, r0.w, c8.x
        mov r5.xzw, r2.xyzz
        texldl r6, r5, s1
        mov r2.x, r1.x
        texldl r7, r2.xyzz, s1
        texldl r8, r2.xwzz, s1
        mov r5.xzw, r2.xyzz
        texldl r9, r5, s1
        mad r2.x, v3.x, r0.w, c8.x
        texldl r10, r2.xyzz, s1
        texldl r11, r2.xwzz, s1
        mov r5.xzw, r2.xyzz
        texldl r2, r5, s1
        mov r3.y, r4.x
        mov r3.z, r6.x
        mad r2.yzw, v3.z, r0.w, -r3.xxyz
        cmp_pp r3.xyz, r2.yzww, c12.y, c12.x
        mov r7.y, r8.x
        mov r7.z, r9.x
        mad r4.xyz, v3.z, r0.w, -r7
        cmp_pp r4.xyz, r4, c12.y, c12.x
        mov r10.y, r11.x
        mov r10.z, r2.x
        mad r5.xyz, v3.z, r0.w, -r10
        cmp_pp r5.xyz, r5, c12.y, c12.x
        mul r1.xy, r1, c8.y
        frc r1.xy, r1
        add_pp r3.xyz, r3, r4
        cmp_pp r2.xyz, r2.yzww, c12.z, c12.w
        add_pp r2.xyz, r5, r2
        mad_pp r1.xzw, r2.xyyz, r1.x, r3.xyyz
        add_pp r0.w, r1.z, r1.x
        add_pp r1.x, -r1.x, r1.w
        mad r0.w, r1.x, r1.y, r0.w
        mul_pp r0.y, r0.w, c13.x
        mov_pp r0.z, c6.w
      else
        rcp r0.w, v4.w
        mul r1.xy, r0.w, v4
        mad r2.xy, v4, r0.w, -c8.x
        mov r2.z, c12.x
        texldl r3, r2.xyzz, s2
        mov r2.w, r1.y
        texldl r4, r2.xwzz, s2
        mad r5.y, v4.y, r0.w, c8.x
        mov r5.xzw, r2.xyzz
        texldl r6, r5, s2
        mov r2.x, r1.x
        texldl r7, r2.xyzz, s2
        texldl r8, r2.xwzz, s2
        mov r5.xzw, r2.xyzz
        texldl r9, r5, s2
        mad r2.x, v4.x, r0.w, c8.x
        texldl r10, r2.xyzz, s2
        texldl r11, r2.xwzz, s2
        mov r5.xzw, r2.xyzz
        texldl r2, r5, s2
        mov r3.y, r4.x
        mov r3.z, r6.x
        mad r2.yzw, v4.z, r0.w, -r3.xxyz
        cmp_pp r3.xyz, r2.yzww, c12.y, c12.x
        mov r7.y, r8.x
        mov r7.z, r9.x
        mad r4.xyz, v4.z, r0.w, -r7
        cmp_pp r4.xyz, r4, c12.y, c12.x
        mov r10.y, r11.x
        mov r10.z, r2.x
        mad r5.xyz, v4.z, r0.w, -r10
        cmp_pp r5.xyz, r5, c12.y, c12.x
        mul r1.xy, r1, c8.y
        frc r1.xy, r1
        add_pp r3.xyz, r3, r4
        cmp_pp r2.xyz, r2.yzww, c12.z, c12.w
        add_pp r2.xyz, r5, r2
        mad_pp r1.xzw, r2.xyyz, r1.x, r3.xyyz
        add_pp r0.w, r1.z, r1.x
        add_pp r1.x, -r1.x, r1.w
        mad r0.w, r1.x, r1.y, r0.w
        mul_pp r0.y, r0.w, c13.x
        mad_sat_pp r0.w, v0.w, c10.x, c10.y
        mad_pp r0.z, r0.w, -c6.w, c6.w
      endif
    endif
    mul_sat r0.x, r0.x, c11.x
    add_pp r0.x, -r0.x, c12.y
    max_pp r1.x, r0.y, r0.x
    mul_pp oC0.w, r0.z, r1.x
    add r0.xyz, c0, -v0
    dp3 r0.x, r0, r0
    rsq r0.x, r0.x
    rcp r0.x, r0.x
    add r0.x, r0.x, -c3.x
    mul r0.x, r0.x, c3.z
    mad r1, v0.xyzx, c12.yyyx, c12.xxxy
    dp4 r0.y, c4, r1
    mul r0.y, r0.y, c3.w
    max r1.x, r0.x, r0.y
    min r0.x, c5.w, r1.x
    max r1.x, r0.x, c12.x
    mov r0.xyz, c6
    add r1.yzw, -r0.xxyz, c5.xxyz
    mad_pp r1.xyz, r1.x, r1.yzww, c6
    cmp_pp r0.xyz, -c2.x, r0, r1
    mov r1.y, c13.y
    mad_pp oC0.xyz, r0, r1.y, c1.x
)";
        strcpy(m_own_shader, default_shader);
	return Mod::on_initialize();
}

// onFrame()
// do something each frame example
//void ModShaderEditor::onFrame(fmilliseconds& dt) {}

// onConfigSave
// save your data into cfg structure.
//void ModShaderEditor::onConfigSave(utility::Config& cfg) { cfg.set<variable_type>("config_string",variable_name); };

// onConfigLoad
// load data into variables from config structure.
//void ModShaderEditor::onConfigLoad(const utility::Config& cfg) { //variable_name = cfg.get<variable_type>("config_string").value_or(default_value); };

// onGUIframe()
// draw your imgui widgets here, you are inside imgui context.
void ShaderEditor::on_gui_frame() { 
    if (ImGui::Button("clear")) {
        g_mats.clear();
    }
    ImGui::Checkbox("capture", &g_is_capturing);
    for (auto& entry : g_mats) {
        if (entry) {
            char label[128];
            sprintf(label, "0x%X", entry);
            if (ImGui::Button(label)) {
                entry->green = 0.1f;
                entry->N000077A1 = 0.1f;
                //entry = NULL;
            }
        }

    }

    ImGui::TextWrapped(m_error_string);
    ImGui::InputScalar("input address", ImGuiDataType_U32, &m_shader_loc, NULL, NULL, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputTextMultiline("Code your own shader", m_own_shader, 2048);
    if (ImGui::Button("REcompile shader")) {
        create_shader_program(m_own_shader);
    }
};

// onGamePause()
// do something when toggling a gui
//void ModName::onGamePause(bool toggle) { };

// onMessage()
// handle some window message, return true to pass to the game window
// or false to drop it.
//bool ModName::onMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) { return true; };

#endif
