#pragma once
/*#define GLM_ENABLE_EXPERIMENTAL

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

//#include "glm/mat3x3.hpp"

using Vector3f = glm::vec3;
using Vector4 = glm::vec4;

// Generated using ReClass 2016
#include <cstdint>*/
class cAreaJump;
class roomInfo;

class cAreaJump
{
public:
	virtual void function0(); //
	virtual void function1(); //
	virtual void function2(); //
	virtual void function3(); //
	virtual void function4(); //
	virtual void function5(); //
	virtual void function6(); //
	virtual void function7(); //
	virtual void function8(); //
	virtual void function9(); //

	char pad_0x0004[0x28]; //0x0004
	roomInfo* room_info_ptr; //0x002C 
	char pad_0x0030[0x20]; //0x0030
	Vector3f unk_vec; //0x0050 
	Vector3f unk_vec2; //0x005C 
	__int32 init_jump; //0x0068 
	__int32 room_id; //0x006C 
	__int32 mission; //0x0070 
	__int32 bp_floor_stage; //0x0074 
	char pad_0x0078[0x48]; //0x0078

}; //Size=0x00C0

class roomInfo
{
public:
	virtual void function0(); //
	virtual void function1(); //
	virtual void function2(); //
	virtual void function3(); //
	virtual void function4(); //
	virtual void function5(); //
	virtual void function6(); //
	virtual void function7(); //
	virtual void function8(); //
	virtual void function9(); //

	char n0000127_e[20]; //0x11E5888 
	char pad_0x0018[0x3EC]; //0x0018

}; //Size=0x0404

class sWorkRate;
class unkClass;
class unkClass2;

class sWorkRate
{
public:
	virtual void function0(); //
	virtual void function1(); //
	virtual void function2(); //
	virtual void function3(); //
	virtual void function4(); //
	virtual void function5(); //
	virtual void function6(); //
	virtual void function7(); //
	virtual void function8(); //
	virtual void function9(); //

	char pad_0x0004[0x1C]; //0x0004
	float game_speed; //0x0020 
	float unk_float2; //0x0024 
	float global_speed; //0x0028 
	float player_speed; //0x002C 
	float enemy_speed; //0x0030 
	float room_speed; //0x0034 
	float turbo_speed; //0x0038 
	float unk_float3; //0x003C 
	char pad_0x0040[0x10]; //0x0040
	unkClass* unk_class_ptr; //0x0050 
	char pad_0x0054[0x4]; //0x0054

}; //Size=0x0058

class unkClass
{
public:
	unkClass2* next; //0x0000 
	uint32_t n0000045_c; //0x0004 
	char pad_0x0008[0x4]; //0x0008
	uint32_t n0000045_e; //0x000C 
	char pad_0x0010[0x10]; //0x0010
	uint32_t n00000463; //0x0020 
	char pad_0x0024[0x8]; //0x0024

}; //Size=0x002C

class unkClass2
{
public:
	uint32_t next_ptr; //0x0000 
	char pad_0x0004[0x400]; //0x0004

}; //Size=0x0404

class Inputs
{
public:
	virtual void function0(); //
	virtual void function1(); //
	virtual void function2(); //
	virtual void function3(); //
	virtual void function4(); //
	virtual void function5(); //
	virtual void function6(); //
	virtual void function7(); //
	virtual void function8(); //
	virtual void function9(); //

	__int32 button_pressed; //0x0004 
	__int32 button_released; //0x0008 
	__int32 button_went_down; //0x000C 
	__int32 button_pressed02; //0x0010 
	char pad_0x0014[0x4]; //0x0014
	float unk_float; //0x0018 
	char pad_0x001_c[0x18]; //0x001C

}; //Size=0x0034

class MtObject;

class MtDTI
{
public:
	char *m_name; //0x0004
	class MtDTI *mp_next; //0x0008
	class MtDTI *mp_child; //0x000C
	class MtDTI *mp_parent; //0x0010
	class MtDTI *mp_link; //0x0014
	uint32_t mp_link1; //0x0018
	uint32_t m_id; //0x001C
	// hope it's fucking thiscall
	//virtual void vecDelDtor(MtDTI* this, unsigned int x)();
	//virtual void MtObject* newInstance(MtDTI *this)();

	virtual void vec_del_dtor(unsigned int x);
	virtual MtObject* new_instance();
	virtual void function2();
	virtual void function3();
	virtual void function4();
	virtual void function5();
	virtual void function6();
	virtual void function7();
	virtual void function8();
	virtual void function9();
}; //Size: 0x0020

class MtObject
{
public:
	virtual void vec_del_dtor(unsigned int i);
	virtual void create_ui(void* prop);
	virtual bool is_enable_instance();
	virtual void create_property(void* prop);
	virtual MtDTI* get_dti();
	virtual MtObject* allocate(size_t size, int a2);
	virtual void function6();
	virtual void function7();
	virtual void function8();
	virtual void function9();
	virtual void function10();
	virtual void function11();
}; //Size: 0x0004

class MtMatrix
{
public:
	Vector4 m1; //0x0000
	Vector4 m2; //0x0010
	Vector4 m3; //0x0020
	Vector4 m4; //0x0030
}; //Size: 0x0040

class CUnit : public MtObject
{
public:
	uint32_t bitfield; //0x0004
	class CUnit *mp_next_unit; //0x0008
	class CUnit *mp_prev_unit; //0x000C
	float m_delta_time; //0x0010
	uint32_t some_union; //0x0014
}; //Size: 0x0018
static_assert(sizeof(CUnit) == 0x18);

class UCoord : public CUnit
{
public:
	class UCoord *mp_parent; //0x0018
	char pad_001_c[20]; //0x001C
	Vector3f m_pos; //0x0030
	char pad_003_c[4]; //0x003C
	Vector4 m_quat; //0x0040
	Vector3f m_scale; //0x0050
	Matrix4x4 m_lmat; //0x005C
	Matrix4x4 m_wmat; //0x009C
}; //Size: 0x00DC
static_assert(sizeof(UCoord) == 0xDC);

class UFilter : public CUnit
{
public:
	uint16_t m_pass; //0x0018
	char pad_001_a[2]; //0x001A
	uint32_t m_priority; //0x001C
}; //Size: 0x0020
static_assert(sizeof(UFilter) == 0x20);

class MtHermiteCurve
{
public:
	float x[8]; //0x0000
	float y[8]; //0x0020
}; //Size: 0x0040
static_assert(sizeof(MtHermiteCurve) == 0x40);

enum class CcType : int32_t
{
	TYPE_UNUSED = 0,
	TYPE_HUE = 1,
	TYPE_CONTRAST = 2,
	TYPE_CONTRAST2 = 3,
	TYPE_CHROMA = 4,
	TYPE_CHROMA2 = 5,
	TYPE_BRIGHTNESS = 6,
	TYPE_BRIGHTNESS2 = 7,
	TYPE_SCALE = 8,
	TYPE_SCALE2 = 9,
	TYPE_INPUTLEVEL = 10,
	TYPE_INPUTLEVEL2 = 11,
	TYPE_OUTPUTLEVEL = 12,
	TYPE_OUTPUTLEVEL2 = 13,
	TYPE_SEPIA = 14,
	TYPE_SEPIA2 = 15,
	TYPE_NEGA = 16,
	TYPE_NEGA2 = 17,
	TYPE_GRAYSCALE = 18,
	TYPE_RREPLACE = 19,
	TYPE_GREPLACE = 20,
	TYPE_BREPLACE = 21,
	TYPE_SEPIA3 = 22
}; //0x0008

class UColorCorrectFilterCorrector
{
public:

	bool m_enable; //0x0004
	/*uint16_t N00002DAF; //0x0005
	uint8_t N00002DB0; //0x0007*/
	CcType m_type; //0x0008
	Vector4 m_factor; //0x000C
	Vector4 m_factor2; //0x001C
	char pad_002_c[4]; //0x002C

	virtual void function0();
	virtual void function1();
	virtual void function2();
	virtual void function3();
	virtual void function4();
	virtual void function5();
	virtual void function6();
	virtual void function7();
	virtual void function8();
	virtual void function9();
}; //Size: 0x0030

class UColorCorrectFilter : public UFilter
{
public:

	class UColorCorrectFilterCorrector correctors[16]; //0x0020
	Matrix4x4 m_matrix; //0x0320
	class MtHermiteCurve m_tone_curve_r; //0x0360
	class MtHermiteCurve m_tone_curve_g; //0x03A0
	class MtHermiteCurve m_tone_curve_b; //0x03E0
	Vector4 m_gamma; //0x0420
	class cTransTexture *c_tans_texture; //0x0430
}; //Size: 0x0434


class UEnemySomething
{
public:
	char pad_0000[4992]; //0x0000
	Vector3f m_spawn_coords; //0x1380
	char pad_138_c[8]; //0x138C
	uint8_t n00004023; //0x1394
	char pad_1395[3]; //0x1395
	int32_t m_enemy_spawn_effect_something; //0x1398
	char pad_139_c[12]; //0x139C
	uint32_t unk_type_something; //0x13A8
	uint32_t n00003_fef; //0x13AC
	uint32_t n00003_f_f0; //0x13B0
	uint32_t n00003_f_f1; //0x13B4
	uint32_t n00003_f_f2; //0x13B8
	uint32_t n00003_f_f3; //0x13BC
	Matrix4x4 n00003_f_f4; //0x13C0
	uint32_t n00003_f_f5; //0x1400
	uint8_t n00003_f_f6; //0x1404
	uint8_t n00004033; //0x1405
	uint8_t n00004037; //0x1406
	char pad_1407[1]; //0x1407
	float n00003_f_f7; //0x1408
	uint32_t n00003_f_f8; //0x140C
	uint32_t n00003_f_f9; //0x1410
	uint32_t n00003_ffa; //0x1414
	uint32_t n0000403_d; //0x1418 Ptr
	char pad_141_c[24]; //0x141C
	float n00004000; //0x1434
	uint8_t n00004001; //0x1438
	char pad_1439[3]; //0x1439
	uint32_t n00004002; //0x143C
	uint32_t n00004003; //0x1440
	uint32_t n00004004; //0x1444
	char pad_1448[228]; //0x1448
	bool n00003_c7_b; //0x152C
	bool n00003_fdd; //0x152D
	bool n00003_f_e0; //0x152E
	char pad_152_f[4409]; //0x152F
}; //Size: 0x2668

class SMediator
{
public:
	char pad_0000[36]; //0x0000
	class uPlayer* player_ptr; //0x0024
	char pad_0028[24]; //0x0028

	// TODO(): make it into methods for this shit?
	//uPlayer* getLocalPlayer();
}; //Size: 0x0040

class UModelJoint
{
public:
	char pad_0000[48]; //0x0000
	Vector3f size; //0x0030
	char pad_003_c[84]; //0x003C
}; //Size: 0x0090
static_assert(sizeof(UModelJoint) == 0x90);

class UModelJointArr
{
public:
	class UModelJoint joint[75]; //0x0000
}; //Size: 0x2A30
static_assert(sizeof(UModelJointArr) == 0x2A30);

class uPlayer : public UCoord
{
public:
    char pad_00_dc[0x204]; //0x00DC
	uint32_t m_joint_array_size; //0x02E0
	class UModelJointArr *joint_array; //0x02E4
	char pad_02_e8[0x13E8]; //0x02E8
	bool lockontoggle; //0x16D0
    char pad_16_d1[0x77F]; //0x16D1
	Vector3f m_d_velocity; //0x1E50
	char pad_1_e5_c[0x4]; //0x1E5C
	float m_d_vel_magnitude; //0x1E60
	char pad_1_e64[0xA04]; //0x1E64
}; //Size: 0x2868
static_assert(sizeof(uPlayer) == 0x2868);

//I tried adding this in through reclass but it drives me crazy when you screw up later offsets by editing earlier stuff
//also I swear there's no goto shortcut im not scrolling to 14D98
//0x00330 start of animation stuff (also a pointer)
//0x00348 float animationFrame
//0x00EA8 uint32_t grounded (3 = aerial)
//0x01201 bool isHitstop
//0x01408 start of inputs stuff (also a pointer)
//0x0140C bool*4 inputs on hold
//0x01410 bool*4 inputs on press?
//0x01414 bool*4 inputs on release?
//0x01494 uint32_t playerID
//0x01500 uint32_t moveid pushed to move selection switch
//0x01504 uint32_t movePart
//0x01554 uint32_t attackState (jumping, doing an action etc)
//0x01564 uint32_t moveID
//0x01568 uint32_t prevMoveID
//0x015CC float currentHP
//0x015D0 float maxHP
//0x01640 bool isDead
//0x01660 float 180float (some float that starts at 0, incs to 180, then resets?)
//0x016AC uint32_t movementType (0=still, 1=walking, 2=running)
//0x019B8 pointer to uPlayer
//0x01D94 dante gilgamesh pointer
//->0x0137C bool shouldShow
//0x01D98 dante lucifer pointer
//->0x0137C bool shouldShow
//0x01D9C dante ebony+ivory pointer
//->0x0137C bool shouldShow
//0x01DA0 dante shotgun pointer
//->0x0137C bool shouldShow
//0x01DA4 dante pandora pointer
//->0x0137C bool shouldShow
//0x01DA8 pointer only in dantes char, contains what I used for moveid
//->0x0225C uint32_t moveiddante (should replace this with uPlayer+0x295C moveid)
//0x01DB4 uint32_t danteSwordID
//0x01DB8 uint32_t danteGunID
//0x01DBC uint32_t standing pose
//0x01DFC start of move properties stuff (also a pointer)
//0x01E14 uint8_t*4 canCancel (0x00 in active frames, 0x10 in cancellable frames, 0x30 in buffer frames)
//0x01E68 float hitStopTimer
//0x01E64 float direction of magnitude?
//0x01E7B uint8_t airAttackCount
//0x01E7D uint8_t weight
//0x01E7E uint8_t airHikeCount
//0x01E8C pointer used by every air move
//->0x0001C bool compared to in air moves, no idea what it does
//0x01F24 float currentDT
//0x01F28 float maxDT
//0x01F50 uint8_t everyMoveWrites (no clue what this is, every move seems to mov 1 to it)
//0x01FF4 float some jump timer
//0x01FF8 float another jump timer
//0x028A4 pointer to uPlayer+330 for animation stuff, e.g. uPlayer + 0x28A4->0x18 = frame of animation = uPlayer + 348
//0x02008 bool groundedbad
//0x0295C uint32_t moveid
//0x03080 pointer to locked on enemy
//0x03084 pointer to a later part of locked on enemy
//0x030C4 uint8_t canBuffer (0=can't act, 1=can buffer, 2=can act) (probably int8 didn't check)
//0x03178 bool isJumpBuffered (1 will make the player jump on the first available frame)
//0x0CCE8 uint32_t exceedLevel
//0x14D98 uint32_t currentStyle (wtf is that offset DevilMayCry4_DX9.exe+3B6C92 - mov [esi+00014D98],00000004)

class UStageSetTimeSlow
{
public:
	char pad_0000[4992]; //0x0000
	float m_time_left; //0x1380
	float m_duration; //0x1384
	float m_enemy_work_rate; //0x1388
	float m_stage_work_rate; //0x138C
	char pad_1390[3288]; //0x1390
}; //Size: 0x2068
static_assert(sizeof(UStageSetTimeSlow) == 0x2068);

class CResource : public MtObject
{
public:
	char path[64]; //0x0004
	uint32_t m_ref_count; //0x0044
	uint32_t m_attr; //0x0048
	uint32_t m_flags_something; //0x004C
	uint32_t m_size; //0x0050
	int64_t m_id; //0x0054
	char pad_005_c[4]; //0x005C
}; //Size: 0x0060

class N00002D7A
{
public:
	char pad_0000[4]; //0x0000
}; //Size: 0x0004
static_assert(sizeof(N00002D7A) == 0x4);

class REffectList : public CResource
{
public:
	float m_base_fps; //0x0060
	char *m_param_buf; //0x0064
	uint32_t m_param_buff_size; //0x0068
	uint32_t m_list_num; //0x006C
	uint32_t n00002_d88; //0x0070
	uint32_t n00002_d89; //0x0074
	uint32_t m_resource_ptr; //0x0078
	uint32_t m_resource_info_num; //0x007C
}; //Size: 0x0080

class MtAllocator : public MtObject
{
public:
	uint32_t m_used_size; //0x0004
	uint32_t m_max_used_size; //0x0008
	uint32_t m_total_size; //0x000C
	char *m_name; //0x0010
	uint16_t m_type; //0x0014
	uint16_t m_attr; //0x0016
	uint32_t m_owner; //0x0018
	class MtCriticalSection *m_cs; //0x001C
}; //Size: 0x0020

class MtHeapAllocator : public MtAllocator
{
public:
	char pad_0020[60]; //0x0020
}; //Size: 0x005C

class UEfctCam
{
public:
	char pad_0000[512]; //0x0000
	uint32_t some_field; //0x0200
}; //Size: 0x0204

class UTvNoiseFilter : public UFilter
{
public:
	Vector3f m_noisy_uv_offset; //0x0020
	char pad_002_c[8]; //0x002C
	float m_noise_power; //0x0034
	float m_noise_power_croma; //0x0038
	float m_scale_y; //0x003C
	float m_scale_cr_cb; //0x0040
	uint32_t m_scanline_size; //0x0044
	float m_scanline_alpha; //0x0048
	uint32_t noise_texture_ptr; //0x004C
	uint32_t mp_tv_mask_texture_ptr; //0x0050
	bool m_enable_blank_scan; //0x0054
	char pad_0055[3]; //0x0055
	float m_blank_ofs_y; //0x0058
	float m_blank_speed; //0x005C
	uint8_t m_blank_alpha; //0x0060
	char pad_0061[3]; //0x0061
	float m_blank_size; //0x0064
	float m_shock_noise_freq; //0x0068
	float m_h_sync_noise_amplitude; //0x006C
	float m_v_sync_noise_amplitude; //0x0070
	float m_shock_detail_reduction; //0x0074
	float m_h_sync_blank_width; //0x0078
	float m_v_sync_blank_width; //0x007C
	Vector4 m_v_sync_blank_color; //0x0080
	uint32_t m_shock_noise_time; //0x0090
	float m_h_sync_noise_value; //0x0094
	float m_v_sync_noise_value; //0x0098
	float m_h_sync_lag; //0x009C
	float m_v_sync_lag; //0x00A0
	float m_detail_reduction; //0x00A4
	float m_h_sync_noise_offset; //0x00A8
	char pad_00_ac[1992]; //0x00AC
}; //Size: 0x0874


class SStylishCount
{
public:
	char pad_0000[32]; //0x0000
	uint32_t current_style_tier; //0x0020
	char pad_0024[508]; //0x0024
}; //Size: 0x0220
static_assert(sizeof(SStylishCount) == 0x220);

class UEm003Shl : public MtObject
{
public:
	char pad_0004[44]; //0x0004
	Vector3f m_pos_maybe; //0x0030
	char pad_003_c[3716]; //0x003C
	Vector3f m_some_float_vec; //0x0EC0
	char pad_0_ecc[2260]; //0x0ECC
	uint32_t m_required_idk_wtf_is_this_needs_to_be2; //0x17A0
	uint32_t m_required_idk_wtf_is_this_needs_to_be6; //0x17A4
	char pad_17_a8[12]; //0x17A8
	uint16_t n00005_a23; //0x17B4
	uint8_t m_required_some_byte_idk; //0x17B6
	char pad_17_b7[57]; //0x17B7
}; //Size: 0x17F0
static_assert(sizeof(UEm003Shl) == 0x17F0);

/* 8554 */
struct __declspec(align(4)) MtStringSTRING
{
    int ref;
    unsigned int length;
    char str[1];
};

/* 8555 */
struct MtString
{
    MtStringSTRING *value;
};

class MtCriticalSection
{
public:
    int8_t cs[28]; //0x0000
}; //Size: 0x001C
static_assert(sizeof(MtCriticalSection) == 0x1C);

class CSystem : public MtObject
{
public:
    class MtCriticalSection critical_section; //0x0004
}; //Size: 0x0020

class SRender : public CSystem
{
public:
    char pad_0020[20]; //0x0020
    IDirect3DDevice9* mp_device; //0x0034
    char pad_0038[40]; //0x0038
}; //Size: 0x0060

class SDevil4Main
{
public:
    char pad_0000[66376]; //0x0000
    class SRender *s_render; //0x10348
    char pad_1034_c[60]; //0x1034C
}; //Size: 0x10388
static_assert(sizeof(SDevil4Main) == 0x10388);

class SDevil4MainPtr
{
public:
    class SDevil4Main *mp_devil4_main; //0x0000
    char pad_0004[60]; //0x0004
}; //Size: 0x0040
static_assert(sizeof(SDevil4MainPtr) == 0x40);

class sMouse
{
public:
    char pad_0000[144]; //0x0000
    bool m_show_mouse_cursor; //0x0090
    bool N00000272; //0x0091
    bool N00000276; //0x0092
    char pad_0093[2029]; //0x0093
}; //Size: 0x0880
static_assert(sizeof(sMouse) == 0x880);

class sMousePtr
{
public:
    class sMouse *m_mouse_ptr; //0x0000
}; //Size: 0x0004
static_assert(sizeof(sMousePtr) == 0x4);