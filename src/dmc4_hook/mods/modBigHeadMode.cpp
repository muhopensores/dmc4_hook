#include "modBigHeadMode.hpp"

#include "glm/gtx/compatibility.hpp"

bool g_enable_mod = false;

static uintptr_t JointSizeDetour1Continue = NULL;
static uintptr_t JointSizeDetour2Continue = NULL;

glm::vec3 size{ 7.0f, 7.0f, 7.0f };

float getCurrentStyleRank() {
	constexpr uintptr_t sStylishCountPtr = 0x00E558CC;
	sStylishCount* sc = (sStylishCount*)*(uintptr_t*)sStylishCountPtr;
	uint32_t rank = sc->currentStyleTier;
	float normalizedRank = glm::smoothstep(0.0f, 7.0f, (float)rank); // no style no head
	//                                    edge0 edge1        x
	// smoothstep Returns 0.0 if x <= edge0 and 1.0 if x >= edge1 and performs smooth
	// Hermite interpolation between 0 and 1 when edge0 < x < edge1.
	return normalizedRank;
}

void scaleHeadJoint(uModel__Joint* joint) {
	joint->size = size * getCurrentStyleRank();
}

int isHeadJoint(uModel__Joint* joint) {

	constexpr uintptr_t staticMediatorPtr = 0x00E558B8;
	sMediator* sMedPtr = (sMediator*)*(uintptr_t*)staticMediatorPtr;
	uPlayer* uPlr = sMedPtr->playerPtr;
	uModel__Joint* head = &uPlr->jointArray->joint[4]; // seems to be heads for both chars

	return joint == head;
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
	ImGui::Checkbox("Big head mode", &g_enable_mod);
}

void BigHeadMode::onTwitchCommand(std::size_t hash)
{
	if (hash == m_command) {
		g_enable_mod = !g_enable_mod;
	}
}
