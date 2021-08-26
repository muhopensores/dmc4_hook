
#include "Devil4.hpp"

static MtHeapAllocator** mtHeapAllocStaticPtr = (MtHeapAllocator**)0x00E1434C;

static uintptr_t fptrUpdateActorList{ 0x008DC540 }; // Spawns shit
static uintptr_t someStruct{ 0x00E552CC };

namespace Devil4SDK {

	void* mtAllocateHeap(size_t size, int a2) {
		static MtHeapAllocator* mtHeapAlloc = *mtHeapAllocStaticPtr;

		return (void*)mtHeapAlloc->allocate(size, a2);
	}

	//_DWORD *__usercall uEm003Shl_ConstructorMaybe_sub_560330@<eax>(int a1@<eax>)
	MtObject* uEm003Shl_Consturctor_sub(void* mem) {
		MtObject* retVal = 0;
		constexpr uintptr_t fptrEm003Shl_Consturctor = 0x00560330;
		__asm {
			mov eax, mem
			call fptrEm003Shl_Consturctor
			mov [retVal], eax
		}
		return retVal;
	}

	//char __userpurge spawnOrSomething_sub_8DC540@<al>(int sUnitMaybe@<eax>, _DWORD *mtObjectToSpawn@<esi>, int someIndex)
	void spawnOrSomething(void* a1, MtObject* objToSpawn, int a3) {
		constexpr uintptr_t fptrSpawnOrSomething = 0x008DC540;

		/*__asm {
			mov ecx, 12h
			mov eax, [someStruct] // static
			mov eax, [eax]
			mov esi, objToSpawn
			push 12h
			call fptrUpdateActorList
		}*/

		__asm {
			mov eax, [a1]
			mov eax, [eax]
			mov esi, objToSpawn
			mov ecx, a3
			push a3
			call fptrSpawnOrSomething
		}

	}

	void uEm003Shl_new() {
		void* mem = mtAllocateHeap(sizeof(uEm003Shl), 0x10);
		if (!mem) {
			return;
		}
		
		uEm003Shl* shell = (uEm003Shl*)uEm003Shl_Consturctor_sub(mem);
		if (!shell) {
			return;
		}

		spawnOrSomething((void*)0x00E552CC, shell, 0x12);

		shell->mSomeFloatVec = glm::vec3(0.0f, 0.0f, 0.0f);
		
		auto lp = Devil4SDK::getLocalPlayer();
		shell->mPosMaybe = lp->mPos;

		shell->mRequiredIdkWtfIsThisNeedsToBe2 = 2;
		shell->mRequiredIdkWtfIsThisNeedsToBe6 = 0;
		shell->mRequiredSomeByteIdk = 0;
	}

	uPlayer* getLocalPlayer() {
		constexpr uintptr_t staticMediatorPtr = 0x00E558B8;
		static sMediator* sMediatorPtr = (sMediator*)*(uintptr_t*)staticMediatorPtr;
		return sMediatorPtr->playerPtr;
	}

	sStylishCount* getStylishCount() {
		constexpr uintptr_t sStylishCountPtr = 0x00E558CC;
		static sStylishCount* sc = (sStylishCount*)*(uintptr_t*)sStylishCountPtr;
		return sc;
	}

	sWorkRate* getWorkRate() {
		constexpr uintptr_t sWorkRatePtr = 0x00E558D0;
		static sWorkRate* sw = (sWorkRate*)*(uintptr_t*)sWorkRatePtr;
		return sw;
	}
}