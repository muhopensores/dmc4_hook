#include "modBigHeadMode.hpp"

#include "glm/gtx/compatibility.hpp"
#include "../sdk/Devil4.hpp"

bool g_enable_mod = false;
bool g_swole_mode = false;
static uintptr_t JointSizeDetour1Continue = NULL;
static uintptr_t JointSizeDetour2Continue = NULL;

glm::vec3 size{ 0.5f, 0.5f, 0.5f };
glm::vec3 size_torso{ 0.9f, 0.8f, 0.8f };

static float getCurrentStyleRank() {
	sStylishCount* sc = Devil4SDK::getStylishCount();
	uint32_t rank = sc->currentStyleTier;
	//float normalizedRank = glm::smoothstep(0.0f, 7.0f, (float)(rank+1)); // no style no head
	//                                    edge0 edge1        x
	// smoothstep Returns 0.0 if x <= edge0 and 1.0 if x >= edge1 and performs smooth
	// Hermite interpolation between 0 and 1 when edge0 < x < edge1.
	//return normalizedRank;
	return (float)(rank + 1);
}

static void scaleHeadJoint(uModel__Joint* joint) {
	if (g_swole_mode) {
		joint->size = size * (glm::clamp(getCurrentStyleRank(),1.5f,3.0f));
	}
	else {
		joint->size = size * getCurrentStyleRank();
	}
}

static int isHeadJoint(uModel__Joint* joint) {
	uPlayer* uPlr = Devil4SDK::getLocalPlayer();
    if (uPlr) {
        if (g_swole_mode) {
            uModel__Joint* torso = &uPlr->jointArray->joint[2]; // seems to be torso for both chars
            return joint == torso;
        }
        else {
            uModel__Joint* head = &uPlr->jointArray->joint[4]; // seems to be heads for both chars
            return joint == head;
        }
        return 0;
	}
    return 0;
}

naked void JointSizeDetour1() {
	__asm {
		pushf
		cmp byte ptr [g_enable_mod], 1
		jne originalCode1
		pusha
		push esi
		call isHeadJoint
		test eax, eax
		pop esi
		popa
	    je originalCode1
		pusha
		push esi
		call scaleHeadJoint
		pop esi
		popa
		popf
		jmp bail
	originalCode1:
		popf
		movss [esi+30h],xmm0
		movss xmm0,[esp+54h]
		movss [esi+34h],xmm0
		movss xmm0,[esp+58h]
		movss [esi+38h],xmm0
	bail:
		jmp dword ptr [JointSizeDetour1Continue]
	}
}

naked void JointSizeDetour2() {
	__asm {
		pushf
		cmp byte ptr [g_enable_mod], 1
		jne originalCode2
		pusha
		push esi
		call isHeadJoint
		test eax, eax
		pop esi
		popa
		je originalCode2
		pusha
		push esi
		call scaleHeadJoint
		pop esi
		popa
		popf
		jmp bail

	originalCode2:
		popf
		movss [esi+30h], xmm0
		movss xmm0, [esp+1C4h]
		movss [esi+34h],xmm0
		movss xmm0,[esp+1C8h]
		movss [esi+38h],xmm0

	bail:
		jmp dword ptr [JointSizeDetour2Continue]

	}
}


std::optional<std::string> BigHeadMode::onInitialize()
{

#if 1
	// DevilMayCry4_DX9.exe+5E9774 
	if (!install_hook_offset(0x5E9774 , hook1, &JointSizeDetour1, &JointSizeDetour1Continue, 0x1B))
	{
		HL_LOG_ERR("Failed to init BigHeadMode mod\n");
		return "Failed to init BigHeadMode mod";
	}
	// DevilMayCry4_DX9.exe+5FC978 

	if (!install_hook_offset(0x5FC978 , hook2, &JointSizeDetour2, &JointSizeDetour2Continue, 0x21))
	{
		HL_LOG_ERR("Failed to init BigHeadMode mod\n");
		return "Failed to init BigHeadMode mod";
	}
#endif
	m_command = std::hash<std::string>{}("\\" + getModName());
	return Mod::onInitialize();
}

/*void BigHeadMode::onFrame(fmilliseconds & dt)
{
}*/

void BigHeadMode::onGUIframe()
{
    if (ImGui::Checkbox("Big Head Mode", &g_enable_mod)) {
        if (g_swole_mode == true)
            g_swole_mode = false;
	}
	if (ImGui::Checkbox("Swole Mode", &g_swole_mode)) {
		g_enable_mod = g_swole_mode;
	}
}

void BigHeadMode::onTwitchCommand(std::size_t hash)
{
	if (hash == m_command) {
		g_enable_mod = !g_enable_mod;
	}
	if (hash == m_swCommand) {
		g_enable_mod = !g_enable_mod;
		g_swole_mode = !g_swole_mode;
	}
}
