#pragma once
/*#define GLM_ENABLE_EXPERIMENTAL

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

//#include "glm/mat3x3.hpp"

using Vector3f = glm::vec3;
using Vector4 = glm::vec4;

// Generated using ReClass 2016
#include <cstdint>*/

#include <vector>

enum class GameDifficulty : uint32_t {
    HUMAN                 = 0,
    DEVIL_HUNTER          = 1,
    SON_OF_SPARDA_HOH_HAH = 2,
    DANTE_MUST_DIE        = 3,
    LEGENDARY_DARK_KNIGHT = 4,
};

enum class GameMode : uint32_t {
    DEFAULT        = 0,
    HEAVEN_OR_HELL = 1,
    HELL_AND_HELL  = 2,
};

static const std::vector<std::pair<uint16_t, const char*>> buttonPairs = {
    {0x0000, "None"},
    {0x0001, "Back"},
    {0x0002, "Left Thumb"},
    {0x0004, "Right Thumb"},
    {0x0008, "Start"},
    {0x0010, "Dpad Up"},
    {0x0020, "Dpad Right"},
    {0x0040, "Dpad Down"},
    {0x0080, "Dpad Left"},
    {0x0100, "Left Shoulder"},
    {0x0200, "Right Shoulder"},
    {0x0400, "Left Trigger"},
    {0x0800, "Right Trigger"},
    {0x1000, "Y"},
    {0x2000, "B"},
    {0x4000, "A"},
    {0x8000, "X"}
};

class cAreaJump;
class roomInfo;

class aGame
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

    char pad_4[0x28];
    roomInfo* roomInfoPtr1; // 0x2c
    char pad_30[0x20];
    Vector3f unk_vec; // 0x50
    Vector3f unk_vec2; // 0x5c
    int init_jump; // 0x68
    int room_id; // 0x6c
    int mission_id; // 0x70
    int bp_floor; // 0x74
    char pad_78[0x159];
    bool m_paused; // 0x1d1
}; // Size: 0x1d2

class currentMission
{
public:
    char pad_0[0x20];
    int restart; // 0x20
    char pad_24[0x4];
    int currentCharacter; // 0x28
    char pad_2c[0xdc];
    int roomNumber; // 0x108
    int roomNumber2; // 0x10c
}; // Size: 0x110

class currentRoom
{
public:
    char pad_0[0x20];
    bool allowPause; // 0x20
    char pad_21[0x10];
    bool isPaused; // 0x31
    char pad_32[0xa];
    uint32_t restartLastEvent; // 0x3c
}; // Size: 0x40

class sArea
{
public:
    char pad_0x0000[0x3830]; // 0x0000
    aGame* aGamePtr; // 0x3830 aGame
    currentMission* currentMissionPtr; // 0x3834
    currentRoom* currentRoomPtr; // x03838
};

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

class MtDTI
{
public:
	char *m_name; //0x0004
	class MtDTI *mp_next; //0x0008
	class MtDTI *mp_child; //0x000C
	class MtDTI *mp_parent; //0x0010
	class MtDTI *mp_link; //0x0014
	size_t m_size; //0x0018
	uint32_t m_id; //0x001C
	// hope it's fucking thiscall
	//virtual void vecDelDtor(MtDTI* this, unsigned int x)();
	//virtual void MtObject* newInstance(MtDTI *this)();

	virtual void vec_del_dtor(unsigned int x) {};
	virtual void* new_instance() {return nullptr;};
}; //Size: 0x0020

class MtObject
{
public:
	virtual void vec_del_dtor(unsigned int i) {};
	virtual void create_ui(void* prop) {};
	virtual bool is_enable_instance() {return 1;};
	virtual void create_property(void* prop) {};
	virtual MtDTI* get_dti() {return (MtDTI*)0x00E5C5A8;};
}; //Size: 0x0004

class uActor {
    virtual void destructor();
    virtual void getTypeInfo();
    virtual void ukn1();
    virtual void ukn2();
    virtual void getDTI();
    virtual void setup();//0x14
    virtual void freeze(); //0x18, as in literally frosted over
    virtual void ukn4(); //0x1C
    virtual void ukn5(); //0x20
    virtual void render(void* mtrans); // 0x24
    virtual void ukn7(); // 0x28
    virtual void ukn8(); // 0x2C
    virtual void die(); // 0x30
    virtual void updateLmat();//0x34
    virtual void updateWmat();//0x38
    virtual void getJointMatrix(int jntInd);//0x3C
   // virtual void LoadrModel(CResource* mdl);//0x40
    virtual void lifecycle();//0x78
    virtual void getCenterPos();//0x84
    virtual void getCenterJointInd(int index);//0x88
    virtual void DamageReaction();//0x90
    virtual void AtckHitCallback();//0x94

    uintptr_t vtable;
    int transFlags;
};

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
    union {
        uint32_t bitfield;
        struct {
        uint16_t pad0;
        uint8_t mTransMode;
        uint8_t mTransView;
        };
    };
	class CUnit *mp_next_unit; //0x0008
	class CUnit *mp_prev_unit; //0x000C
	float m_delta_time; //0x0010
	char reserved_state_flags[4]; //0x0014
}; //Size: 0x0018
static_assert(sizeof(CUnit) == 0x18);

class UCoord : public CUnit
{
public:
	class UCoord *mp_parent; //0x0018
	uint32_t mParentNo;//attached entity's joint index
    uint32_t mOrder;//axis order
    char pad_024_c[0xC];
	Vector3f m_pos; //0x0030
	char pad_003_c[4]; //0x003C
	Vector4 m_quat; //0x0040
	Vector3f m_scale; //0x0050
    char pad_05C_c[4];
	Matrix4x4 m_lmat; //0x0060
	Matrix4x4 m_wmat; //0x00A0
}; //Size: 0x00E0
static_assert(sizeof(UCoord) == 0xE0);

class uModel : public UCoord
{
public:
    Matrix4x4 mPrevWmat;
    uint32_t mLightGroup;
};

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

class uEnemyDamagePossibility {
public:
    char pad_00[4]; // 0x0
}; // Size: 0x4

class uEnemyDamage {
public:
    char pad_0[0x18];
    float HP; // 0x18
    float HPMax; // 0x1c
    char pad_20[0xc];
    float HPTaken; // 0x2c
    char pad_30[0xc];
    int stun[5]; // 0x3c
    int displacement[5]; // 0x50
    int unknown[8]; // 0x64
    char pad_84[0x8];
    bool isDead; // 0x8c
    char pad_8d[0x1f];
    float stunResetTimer; // 0xac
}; // Size: 0xb0

class uEnemy {
public:
    char pad_0[0x10];
    float delta; // 0x10
    uint8_t moveID; // 0x14
    uint8_t movePart; // 0x15
    char pad_16[0x1a];
    Vector3f position; // 0x30
    char pad_3c[0x14];
    Vector3f scale; // 0x50
    char pad_5c[0x74];
    Vector3f positionKnockback; // 0xd0
    char pad_dc[0x14];
    Vector3f rotationKnockback; // 0xf0
    char pad_fc[0x34];
    bool isLit; // 0x130
    char pad_131[0x3];
    bool causeShadow; // 0x134
    char pad_135[0x1ef];
    bool playAnims; // 0x324
    char pad_325[0xf];
    uint8_t moveID2; // 0x334
    char pad_335[0x13];
    float animFrame; // 0x348
    char pad_34c[0xb54];
    uint16_t launchStateThing1; // 0xea0
    char pad_ea2[0x6];
    uint16_t launchStateThing2; // 0xea8
    char pad_eaa[0x55a];
    bool inBattle; // 0x1404
    bool isActive; // 0x1405
    char pad_1406[0xa];
    uint32_t ID; // 0x1410
    char pad_1414[0xec];
    class uEnemyDamagePossibility DamagePtrMephistoFaustAssaultGladius; // 0x1500
    class uEnemyDamagePossibility DamagePtrAltoBiancoFrostBlitz; // 0x1504
    class uEnemyDamagePossibility DamagePtrCutlass; // 0x1508
    char pad_150c[0x20];
    class uEnemyDamagePossibility DamagePtrScarecrowLegArmMega; // 0x152c
    char pad_1530[0xec];
    bool grounded; // 0x161c
    char pad_161d[0x523];
    Vector3f velocity; // 0x1b40
    char pad_1b4c[0x6478];
    class uEnemyDamagePossibility DamagePtrBasilisk; // 0x7fc4
}; // Size: 0x7fc8

class uBoss {
public:
    char pad_0[0x14];
    uint8_t moveID; // 0x14
    uint8_t movePart; // 0x15
    char pad_16[0x1a];
    Vector3f position; // 0x30
    char pad_3c[0x14];
    Vector3f scale; // 0x50
    char pad_5c[0x2d8];
    uint8_t moveID2; // 0x334
    char pad_335[0x13];
    float animFrame; // 0x348
    char pad_34c[0x10c0];
    uint32_t inputHold; // 0x140c
    uint32_t inputPress; // 0x1410
    char pad_1414[0x108];
    float HP; // 0x151c
    float HPMax; // 0x1520
    char pad_1524[0xa00];
    float DT; // 0x1f24
    float DTMax; // 0x1f28
}; // Size: 0x1f2c

class cCameraPlayer {
public:
    char pad_0[0x10];
    Vector3f eye; // 0x10
    char pad_1c[0x4];
    Vector3f lookat; // 0x20
    char pad_2c[0x14];
    float nearClipPlane; // 0x40
    char pad_44[0x90];
    float angle; // 0xd4
    float distance; // 0xd8
    float distanceLockon; // 0xdc
    char pad_e0[0x4];
    float FOV; // 0xe4
    float FOVBattle; // 0xe8
    char pad_ec[0x114];
    Matrix4x4 possibleMat1; // 0x200 // buncha possibilities up til 230
}; // Size: 0x240

// sMediator + D0 camera, had a more useable glm mat but less settable values
class uCameraCtrl {
public:
    char pad_0[0x1c];
    float nearClipPlane; // 0x1c
    char pad_20[0x4];
    float FOV; // 0x24
    char pad_28[0x8];
    Vector3f eye; // 0x30
    char pad_3c[0x4];
    Vector3f up; // 0x40
    char pad_4c[0x4];
    Vector4f lookat; // 0x50
    char pad_60[0x150];
    Matrix4x4 possibleMat5; // 0x1b0
    char pad_1f0[0x2a0];
    cCameraPlayer* cCameraPlayer1; // 0x490
}; // Size: 0x494

class SMediator {
public:
    char pad_0[0x24];
    class uPlayer* player_ptr; // 0x24
    char pad_28[0x88];
    class uBoss* uBoss1; // 0xb0
    char pad_b4[0x1c];
    class uCameraCtrl* camera1; // 0xd0
    char pad_d4[0xc];
    uint32_t missionID; // 0xe0 // always shows 50 for BP
    uint32_t roomID; // 0xe4
    char pad_e8[0x2c];
    uint32_t orbCount; // 0x114
    uint32_t orbMissionCurrent; // 0x118
    uint32_t orbMissionPotential; // 0x11C
    char pad_118[0x14];
    float enemyDifficulty; // 0x134
    char pad_138[0x4];
    uint32_t hellGoldOrbCount; // 0x13c
    GameDifficulty gameDifficulty; // 0x140
    GameMode gameMode; // 0x144
    char pad_148[0x70];
    class uEnemy* uEnemies[10]; // 0x1b8
    uint32_t enemyCount[3]; // 0x1e0
    char pad_1ec[0x64];
    float bpTimer; // 0x250
    char pad_254[0x114];
    bool hasUnlockedDT; // 0x368
    bool hasUnlockedAirHike; // 0x369
    char pad_36a[0x1];
    bool hasUnlockedSprint; // 0x36b
    char pad_36c[0x36];
    bool hasUnlockedHold; // 0x3a2
    bool hasUnlockedMaxAct; // 0x3a3
    char pad_3a4[0x5];
    bool displayDisasterGauge; // 0x3a9
    char pad_3aa[0xea];
    bool turboEnabled; // 0x494
    char pad_495[0x38b];
    sWorkRate workRate; // 0x820
}; // Size: 0x874

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

class MotionData {
public:
    char pad_0[0x4];
    uint32_t hitboxStartTime; // 0x4
    char pad_8[0xc];
    uint32_t passThroughEnemiesIdk; // 0x14
    char pad_18[0x24];
    uint32_t sfxTiming; // 0x3c
    char pad_40[0x10];
    uint32_t sfxToPlay; // 0x50
}; // Size: 0x54

class InnerMotionPtr {
public:
    MotionData motionData[100]; // 0x0
}; // Size: 0x20d0

class MotionPtr {
public:
    char pad_0[0x4];
    char motionString1[30]; // 0x4
    char pad_22[0x17e];
    class InnerMotionPtr* innerMotionPtr1; // 0x1a0
}; // Size: 0x1a4

class LuciferPin {
public:
    char pad_0[0x14];
    char pin_state;//0x14 (3-pinned, 4-primed, 7-climax)
    char pop_state;//0x15
    char pad_15[2];
    uintptr_t PinnedTarget;//0x18
    char pad_1C[0x14];
    Vector3f position;
    char pad_38[8];
    Vector4 rotation;
    Vector3f scale; // 0x50
    char pad_5c[0x1734];
    float timer; // 0x1790
    float timerMax; // 0x1794
    char pad_1798[0x1c];
    bool penetrated; // 0x17b4
}; // Size: 0x17b5

class DevilArm {
public:
    char pad_0[0x1370];
    uint8_t ID; // 0x1370
    char pad_1371[0xb];
    bool visible; // 0x137c
    char pad_137d[0x47];
    float roseYAxisVelocity; // 0x13c4
    char pad_13c8[0x20];
    bool roseAlive; // 0x13e8
}; // Size: 0x13e9

class CharacterSettingsOne {
public:
    char pad_0[0x1c];
    bool groundedActual; // 0x1c
    char pad_1d[0x143];
    float offsetFromGround; // 0x160
}; // Size: 0x164

class CharacterSettingsTwo {
public:
    char pad_0[0xC8];
    float sprintTimer; // 0xc8
    float sprintSpeedMultiplier; // 0xd0
    char pad_d4[0xa4];
    float fallSpeed; // 0x178
}; // Size: 0x17C

class CharacterSettingsThree {
public:
    char pad_0[0x8];
    float stingerSpeed; // 0x8
    float stingerLength; // 0xc
    char pad_10[0x14];
    float firstShotEbonyIvoryBounce; // 0x24
    char pad_28[0x40];
    float rainstormBounce; // 0x68
    char pad_6c[0xc8];
    float trickTravelSpeed; // 0x134
    float trickTravelDistance; // 0x138
    char pad_13c[0x18];
    float rainstormHeightRestriction; // 0x154
    char pad_158[0x20];
    float crossunderBanDistance; // 0x178
    char pad_17c[0x8c];
    float jealousyBounce; // 0x208
    char pad_20c[0x8];
    float snatchLengthLv1; // 0x214
    float snatchLengthLv2; // 0x218
    float snatchLengthLv3; // 0x21c
    char pad_220[0xc];
    Vector3f snatchEnemyOffset; // 0x22c
    char pad_238[0x164];
    float funshipDrainRate; // 0x39c
    char pad_3a0[0x40];
    float funshipBounce; // 0x3e0
    float funshipUnbounce; // 0x3e4
}; // Size: 0x3e8

class uPlayer : public UCoord {
public:
    char pad_dc[0x14];
    Vector4f rotationKnockback; // 0xf0
    char pad_100[0x30];
    uint8_t armShadows; // 0x130
    char pad_131[0x3];
    uint8_t causeShadows; // 0x134
    char pad_135[0x3];
    uint8_t bodyShadows; // 0x138
    char pad_139[0x43];
    uint32_t boneMapping[5]; // 0x17c
    char pad_190[0xec];
    uint32_t meshCount; // 0x27c
    char pad_280[0x24];
    class uPlayer* player_ptr; // 0x2a4
    char pad_2a8[0x38];
    uint32_t m_joint_array_size; // 0x2e0
    class UModelJointArr* joint_array; // 0x2e4
    char pad_2e8[0x60];
    float animFrame; // 0x348
    char pad_34c[0x6ec];
    class MotionPtr* motionPtr1; // 0xa38
    class MotionPtr* motionPtr2; // 0xa3c
    char pad_a40[0x2b4];
    float speedUnsettable; // 0xcf4
    char pad_cf8[0x1b0];
    uint8_t grounded; // 0xea8
    char pad_ea9[0x17];
    float groundInertiaX; // 0xec0
    float inertiaY; // 0xec4
    float groundInertiaZ; // 0xec8
    char pad_ecc[0x335];
    bool hitstop; // 0x1201
    char pad_1202[0xe];
    float facingDirection; // 0x1210
    char pad_1214[0x160];
    uint16_t buttonInputRaw; // 0x1374
    uint16_t stickInputRaw; // 0x1376
    char pad_1378[0x4c];
    float stickDirection; // 0x13c4
    char pad_13c8[0x44];
    uint8_t inputHold[4];  // 0x140c
    uint8_t inputPress[4];    // 0x1410
    uint8_t inputRelease[4];  // 0x1414
    char pad_1418[0x4];
    uint32_t stickThreshold; // 0x141c
    float stickFacingWorldAdjusted; // 0x1420
    bool stickPastThreshold; // 0x1424
    char pad_1425[0x3f];
    float characterSpeed; // 0x1464
    char pad_1468[0x2c];
    uint32_t controllerID; // 0x1494
    char pad_1498[0x58];
    uint32_t canWeaponChange; // 0x14f0
    char pad_14f4[0xc];
    uint32_t moveBank; // 0x1500
    uint32_t movePart; // 0x1504
    char pad_1508[0x48];
    uint8_t cancels1[4]; // 0x1550
    uint8_t cancels2[4]; // 0x1554
    char pad_1558[0xc];
    uint32_t moveID2; // 0x1564
    char pad_1568[0x64];
    float HP; // 0x15cc
    float HPMax; // 0x15d0
    char pad_15d4[0x6c];
    bool isDead; // 0x1640
    char pad_1641[0x6b];
    uint32_t walkType; // 0x16ac
    char pad_16b0[0x10];
    float inertiaDirectionReal; // 0x16c0
    char pad_16c4[0xc];
    bool lockedOn; // 0x16d0
    char pad_16d1[0x3f];
    float rotation2; // 0x1710
    char pad_1714[0x6c];
    float groundInertia; // 0x1780
    char pad_1784[0x1c];
    uintptr_t* collisionPtr1; // 0x17a0
    char pad_17a4[0x5f0];
    class DevilArm* gilgamesh; // 0x1d94
    class DevilArm* lucifer; // 0x1d98
    class DevilArm* ebonyIvory; // 0x1d9c
    class DevilArm* coyoteA; // 0x1da0
    class DevilArm* pandora; // 0x1da4
    char pad_1da8[0xc];
    uint32_t currentSwordUI; // 0x1db4
    uint32_t currentGunUI; // 0x1db8
    char pad_1dbc[0x4];
    uint32_t currentSword; // 0x1dc0
    char pad_1dc4[0x4c];
    uint8_t meleeCancelPermissions1[4]; // 0x1e10
    uint8_t jumpCancelPermissions1[4]; // 0x1e14
    char pad_1e18[0x4];
    float inertia; // 0x1e1c
    char pad_1e20[0x4];
    float rotation3; // 0x1e24
    char pad_1e28[0x28];
    Vector3f m_d_velocity; // 0x1e50
    char pad_1e5c[0x4];
    float m_d_vel_magnitude; // 0x1e60
    float directionOfMagnitude; // 0x1e64
    float hitstopTimer; // 0x1e68
    char pad_1e6c[0xf];
    uint8_t airAttackCount; // 0x1e7b
    char pad_1e7c[0x1];
    uint8_t weight; // 0x1e7d
    uint8_t airHikeCount; // 0x1e7e
    bool canAirHike; // 0x1e7f
    char pad_1e80[0xc];
    class CharacterSettingsOne* characterSettingsOne; // 0x1e8c
    char pad_1e90[0xc];
    class CharacterSettingsTwo* charactersettingsTwo; // 0x1e9c
    char pad_1ea0[0x8];
    class CharacterSettingsThree* charactersettingsThree; // 0x1ea8
    char pad_1eac[0x78];
    float DT; // 0x1f24
    float maxDT; // 0x1f28
    char pad_1f2c[0x24];
    bool fullHouseSomething1; // 0x1f50
    char pad_1f51[0x2e];
    bool canWallHike; // 0x1f7f
    char pad_1f80[0x80];
    class uEnemy* lockOnTargetPtr1; // 0x2000
    char pad_2004[0x4];
    bool grounded2; // 0x2008
    char pad_2009[0x167];
    Vector3f location2; // 0x2170
    char pad_217c[0x50];
    bool tiltForward; // 0x21cc
    char pad_21cd[0x13];
    bool tiltBack; // 0x21e0
    char pad_21e1[0x217];
    float roundTripCharge; // 0x23f8
    char pad_23fc[0x3fc];
    bool tiltBackForward; // 0x27f8
    char pad_27f9[0xab];
    uintptr_t* animFrameInfo; // 0x28a4
    char pad_28a8[0xb4];
    uint32_t moveID3; // 0x295c
    char pad_2960[0x38];
    uint32_t moveIDBest; // 0x2998
    char pad_299c[0x6e4];
    class uEnemy* lockOnTargetPtr2; // 0x3080
    char pad_3084[0x40];
    uint8_t bufferPermissions; // 0x30c4
    char pad_30c5[0x2b];
    uint8_t meleeCancelPermissions2; // 0x30f0
    char pad_30f1[0x83];
    uint8_t jumpCancelPermissions2; // 0x3174
    char pad_3175[0x3];
    uint8_t bufferedJump; // 0x3178
    char pad_3179[0x53];
    uint8_t gunCancelPermissions2; // 0x31cc
    char pad_31cd[0x9b1b];
    uint8_t exceedLevel; // 0xcce8
    char pad_cce9[0xd3];
    float exceedTimer; // 0xcdbc
    char pad_cdc0[0x7fd8];
    uint32_t currentStyle; // 0x14d98
    char pad_14d9c[0x20];
    class LuciferPin* luciferPins[15]; // 0x14dbc
    char pad_14df8[0x3fc];
    float disasterGauge; // 0x151f4
    char pad_151f8[0x870];
    class uEnemy* lockOnTargetPtr3; // 0x15a68
}; // Size: 0x15a6c

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
    virtual void callDestructor(); //0x0
    virtual void getMtFileInfo();//0x14
    virtual void getExtension(); //0x18

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
    virtual MtObject* allocate(size_t size, int align_mask);
    virtual uint16_t deallocate(MtObject* object); // 0x0018

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

// @dog
class DogData {};
class dogFireball : public MtObject {
public:
    private: char pad_4[0x11]; public:
    int fireballWasShot; // 0x15
    private: char pad_19[0x17]; public:
    Vector3f posMaybe; // 0x30
    private: char pad_3c[0x1478]; public:
    uPlayer* playerPtr1; // 0x14b4
    private: char pad_14b8[0x48]; public:
    DogData* dogData; // 0x1500
    private: char pad_1504[0x72cc]; public:
    int unkn1; // 0x87d0
}; // Size: 0x87d4

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


struct kAtckDefTbl {
    uint32_t atckAttr;
    uint32_t atckId;
    uint32_t atckLevel;
    uint32_t atckInfo;
    union {
        uint32_t buffer;
        struct {
            uint8_t atckCommand;
            uint8_t atckCommandNo;
            uint8_t atckCondition;
            uint8_t ukn;
        };
    } command;
    uint32_t atckConditionWp;
    uint32_t atckConditionStyle;
    uint32_t ukn; // aerial lock
    uint32_t atckAs;
    uint32_t cancelId[5];
};