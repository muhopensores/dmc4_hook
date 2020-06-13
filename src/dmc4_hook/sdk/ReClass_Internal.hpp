#pragma once
#define GLM_ENABLE_EXPERIMENTAL

#include "glm/vec3.hpp"
//#include "glm/mat3x3.hpp"

using Vector3f = glm::vec3;

// Generated using ReClass 2016
#include <cstdint>

class cAreaJump;
class roomInfo;

class cAreaJump
{
public:
	virtual void Function0(); //
	virtual void Function1(); //
	virtual void Function2(); //
	virtual void Function3(); //
	virtual void Function4(); //
	virtual void Function5(); //
	virtual void Function6(); //
	virtual void Function7(); //
	virtual void Function8(); //
	virtual void Function9(); //

	char pad_0x0004[0x28]; //0x0004
	roomInfo* roomInfoPtr; //0x002C 
	char pad_0x0030[0x20]; //0x0030
	Vector3f unkVec; //0x0050 
	Vector3f unkVec2; //0x005C 
	__int32 initJump; //0x0068 
	__int32 roomId; //0x006C 
	__int32 mission; //0x0070 
	__int32 bpFloorStage; //0x0074 
	char pad_0x0078[0x48]; //0x0078

}; //Size=0x00C0

class roomInfo
{
public:
	virtual void Function0(); //
	virtual void Function1(); //
	virtual void Function2(); //
	virtual void Function3(); //
	virtual void Function4(); //
	virtual void Function5(); //
	virtual void Function6(); //
	virtual void Function7(); //
	virtual void Function8(); //
	virtual void Function9(); //

	char N0000127E[20]; //0x11E5888 
	char pad_0x0018[0x3EC]; //0x0018

}; //Size=0x0404

class sWorkRate;
class unkClass;
class unkClass2;

class sWorkRate
{
public:
	virtual void Function0(); //
	virtual void Function1(); //
	virtual void Function2(); //
	virtual void Function3(); //
	virtual void Function4(); //
	virtual void Function5(); //
	virtual void Function6(); //
	virtual void Function7(); //
	virtual void Function8(); //
	virtual void Function9(); //

	char pad_0x0004[0x1C]; //0x0004
	float unkFloat; //0x0020 
	float unkFloat2; //0x0024 
	float globalSpeed; //0x0028 
	float playerSpeed; //0x002C 
	float enemySpeed; //0x0030 
	float roomSpeed; //0x0034 
	float turboSpeed; //0x0038 
	float unkFloat3; //0x003C 
	char pad_0x0040[0x10]; //0x0040
	unkClass* unkClassPtr; //0x0050 
	char pad_0x0054[0x4]; //0x0054

}; //Size=0x0058

class unkClass
{
public:
	unkClass2* next; //0x0000 
	uint32_t N0000045C; //0x0004 
	char pad_0x0008[0x4]; //0x0008
	uint32_t N0000045E; //0x000C 
	char pad_0x0010[0x10]; //0x0010
	uint32_t N00000463; //0x0020 
	char pad_0x0024[0x8]; //0x0024

}; //Size=0x002C

class unkClass2
{
public:
	uint32_t nextPtr; //0x0000 
	char pad_0x0004[0x400]; //0x0004

}; //Size=0x0404

class Inputs
{
public:
	virtual void Function0(); //
	virtual void Function1(); //
	virtual void Function2(); //
	virtual void Function3(); //
	virtual void Function4(); //
	virtual void Function5(); //
	virtual void Function6(); //
	virtual void Function7(); //
	virtual void Function8(); //
	virtual void Function9(); //

	__int32 ButtonPressed; //0x0004 
	__int32 ButtonReleased; //0x0008 
	__int32 ButtonWentDown; //0x000C 
	__int32 ButtonPressed02; //0x0010 
	char pad_0x0014[0x4]; //0x0014
	float unkFloat; //0x0018 
	char pad_0x001C[0x18]; //0x001C

}; //Size=0x0034