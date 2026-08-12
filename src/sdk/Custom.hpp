#include "uActor.hpp"
#include "uCollisionMgr.hpp"
#include <concepts>
#include <type_traits>
#define naked __declspec(naked)


constexpr uintptr_t rModelDTI           = 0x00EADF48;
constexpr uintptr_t rAtckStatDTI        = 0x00E57498;
constexpr uintptr_t rColShapeDTI        = 0x00E575F8;
constexpr uintptr_t rDfdStatDTI         = 0x00E57638;
constexpr uintptr_t sDevil4Resource_ptr = 0x00E552D0;
constexpr uintptr_t uActorVtablePtr     = 0x00BC4B78; // uActor
constexpr size_t uActorVtableSize = 79;

namespace custom_utils {
    naked void* __cdecl FuncToPtr(...) {
        __asm {
            mov eax, [esp+4]
            ret
        }
    }

    static void __stdcall load_atk_col(void* rAtck, void* rCol, void* ColMgr, void* Obj) {
        uintptr_t load_call = 0x0050BDD0;
        _asm {
            pushad
            push [rAtck]
            push [rCol]
            mov esi,[ColMgr]
            mov eax,[Obj]
            call load_call
            popad
        }
    }

    static void __stdcall get_model(void* obj, void* model) {
        _asm {
            pushad
            mov ecx,[obj]
            mov eax,[ecx]
            mov edx,[eax+0x40]
            mov eax,[model]
            push eax
            call edx
            popad
        }
    }

    static void __stdcall updateLmat(void* obj) {
        _asm {
            pushad
            mov ecx,[obj]
            mov eax,[ecx]
            mov edx,[eax+0x34]
            call edx
            popad
        }
    }

    static void __stdcall updateWmat(void* obj) {
        _asm {
            pushad
            mov ecx,[obj]
            mov eax,[ecx]
            mov edx,[eax+0x38]
            call edx
            popad
        }
    }

    static void __stdcall despawn(void* obj) {
        _asm {
            mov esi,[obj]
            mov eax,[esi+4]
            and eax,-5
            or eax,3
            mov [esi+4],eax
        }
    }

    static void __stdcall ushell_des(void* obj) {
        uintptr_t ushellDTI         = 0x00BEE7C0;
        uintptr_t ushell_destructor = 0x004A7280;
        _asm {
            pushad
            mov ecx,[obj]
            mov esi,ecx
            mov edx, ushellDTI
            mov [esi],edx
            call ushell_destructor
            popad
        }
    }

    static void __stdcall uActorCons(void* obj) {
        uintptr_t uActor_constructor = 0x004A6E80;
        _asm {
            mov esi,[obj]
            call uActor_constructor
        }
    }

    static void __stdcall uCollisionMgrCons(void* obj) {
        uintptr_t uCollisionMgr_constructor = 0x0050B080;
        _asm {
                mov ecx,[obj]
                call uCollisionMgr_constructor
        }
    }

    static void __stdcall collide(void* CollMgr) {
        _asm {
                pushad
                mov ecx,[CollMgr]
                mov eax,[ecx]
                mov edx,[eax+0x18]
                call edx
                popad
        }
    }

    static void __stdcall hitbox_call(void* CollMgr, int id) {
        uintptr_t hitbox_call_func = 0x0050CA60;
        float timer                = 5.0f;
        _asm {
                pushad
                push 00
                push [CollMgr]
                or edx,-1
                mov eax,[id]
                call hitbox_call_func
                test eax,eax
                je return_addr
                movss xmm0,[timer]
                movss [eax+0x14C],xmm0
                xorps xmm0,xmm0
                movss [eax+0x148],xmm0
                mov esi,[CollMgr]
                mov esi,[esi+0xD8]
                lea edx,[esi+0xA0]
                mov [eax+0x140],edx
                mov byte ptr [eax+0x64],1
            return_addr:
                popad
        }
    }
}

struct CustomActor: uActor {
public:
    CustomActor() = default;
    void destructor(uint32_t flag);
    void die();
    void startup_override() {};
    void move_override() {}
    void lifecycle_override() {};
    void onhit_override(void* atk_param, void* dfd_param) {};
    void render(void* ctrans);
    MtDTI* getDTI() { return (MtDTI*)rModelDTI; };
};

void CustomActor::render(void* ctrans) {
    uintptr_t render_jmp = 0x00522DC0;
    _asm {
        pushad
        push ctrans
        call render_jmp
        popad
    }
}

void CustomActor::destructor(uint32_t flag) {
    custom_utils::ushell_des(this);
    if (flag) {
        devil4_sdk::unit_deallocate((MtObject*)this);
    }
}

void CustomActor::die() {
    custom_utils::despawn(this);
}


#define METHOD_TO_VTABLE(cls, method, ofs) \
    this->my_vtable[ofs / 4] = (uintptr_t)custom_utils::FuncToPtr(&##cls::##method);

#define OVERRIDE_DEFAULT_VTABLE(cls, method, ofs) \
    if (&cls##::##method != &CustomActor::##method) \
        METHOD_TO_VTABLE(##cls, ##method, ofs) \
    else METHOD_TO_VTABLE(CustomActor, ##method, ofs)

#define REGISTER_VTABLE(cls) \
    class cls##Vtable { \
        public: \
            std::vector<uintptr_t> my_vtable; \
            size_t size; \
            cls##Vtable() = default; \
            cls##Vtable(void* vtable, size_t size); \
    }; \
    cls##Vtable::cls##Vtable(void* vtable, size_t size) { \
        this->size=size; \
        this->my_vtable=this->my_vtable = std::vector<uintptr_t>((uintptr_t*)vtable, (uintptr_t*)vtable + size); \
        OVERRIDE_DEFAULT_VTABLE(##cls, getDTI, 0x10) \
        OVERRIDE_DEFAULT_VTABLE(##cls, startup_override, 0x14) \
        OVERRIDE_DEFAULT_VTABLE(##cls, render, 0x24) \
        OVERRIDE_DEFAULT_VTABLE(##cls, lifecycle_override, 0x78) \
        OVERRIDE_DEFAULT_VTABLE(##cls, onhit_override, 0x94) \
    } \
    cls##Vtable cls##_vtable((void*)uActorVtablePtr, uActorVtableSize);