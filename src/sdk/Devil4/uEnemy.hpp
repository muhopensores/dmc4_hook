#pragma once

#include "..\Math.hpp"

class uEnemySomething
{
public:
	using uint32_t = unsigned int;
	using uint8_t = unsigned char;

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
static_assert(sizeof(uEnemySomething) == 0x2668);