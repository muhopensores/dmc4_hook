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

std::optional<std::string> ShaderEditor::onInitialize() {
    if (!install_hook_absolute(0x00A61640, hook, &detour, &jmp_return, 8))
    {
        spdlog::error("Failed to init sheder mod\n");
        return "Failed to init sheder mod";
    }
    const char* default_shader = R"(
ps_3_0
def c10, 1, 0, 0.00392156886, 0
def c11, 1, -0.517647088, -0.509803951, -0.5
dcl_texcoord_pp v0
dcl_texcoord1 v1.xyz
dcl_texcoord2_pp v2.xy
dcl_texcoord3_pp v3.xyz
dcl_texcoord4_pp v4.xyz
dcl_texcoord5_pp v5.xyz
dcl_2d s0
dcl_2d s1
dcl_2d s2
dcl_cube s3
mov r0.xw, c11
add r0.x, r0.x, -c6.w
texld_pp r1, v0, s0
max_pp r2.x, r1.w, r0.x
mul_pp oC0.w, r2.x, v0.z
texld_pp r2, v2, s1
add r0.xyz, r2.xyww, c11.yzww
add_pp r0.xyz, r0, r0
mul_pp r0.x, r0.z, r0.x
mad r0.z, r0.x, -r0.x, c11.x
mad r0.z, r0.y, -r0.y, r0.z
mul_pp r2.xyz, r0.y, v4
rsq r0.y, r0.z
rcp_pp r0.y, r0.y
mad_pp r2.xyz, r0.x, v3, r2
mad_pp r0.xyz, r0.y, v5, r2
nrm_pp r2.xyz, r0
add r0.xyz, -c0, v1
dp3 r2.w, r0, r0
rsq r2.w, r2.w
mul r0.xyz, r0, r2.w
rcp r2.w, r2.w
dp3 r3.x, r0, r2
add r3.y, r3.x, r3.x
add r3.x, -r3.x, c11.x
mad_pp r4.xyz, r2, -r3.y, r0
mov_pp r4.w, c9.x
texldl_pp r4, r4, s3
max_pp r0.x, r4.w, c10.z
rcp r0.x, r0.x
mul_pp r0.xyz, r4, r0.x
mul_pp r0.xyz, r0, c8.w
mul r2.x, r3.x, r3.x
mul r2.x, r2.x, r2.x
mul r2.x, r3.x, r2.x
mad_sat r2.x, r2.x, c8.x, c8.y
add r2.x, -r2.x, c11.x
mul_pp r2.x, r2.x, v0.w
texld_pp r3, v2, s2
mul_pp r2.xyz, r2.x, r3
mul_pp r0.xyz, r0, r2
max r2.x, r1.w, c6.w
mul r2.xyz, r2.x, c6
mad_pp r0.xyz, r1, r2, r0
add r1.x, r2.w, -c3.x
mul r1.x, r1.x, c3.z
mad r2, v1.xyzx, c10.xxxy, c10.yyyx
dp4 r1.y, c4, r2
mul r1.y, r1.y, c3.w
max r2.x, r1.x, r1.y
min r1.x, c5.w, r2.x
max r2.x, r1.x, c10.y
mul_pp r1.x, r2.x, c7.x
mov r1.y, c10.y
cmp_pp r1.x, -c2.x, r1.y, r1.x
lrp_pp r2.xyz, r1.x, c5, r0
mad_pp oC0.xyz, r2, -r0.w, c1.x
)";
        strcpy(m_own_shader, default_shader);
	return Mod::onInitialize();
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
void ShaderEditor::onGUIframe() { 
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
