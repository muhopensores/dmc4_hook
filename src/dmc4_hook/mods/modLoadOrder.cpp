// include your mod header file
#include "modLoadOrder.hpp"

static uintptr_t  jmp_return   { NULL };
static uintptr_t  jmp_return01 { NULL };

static bool mod_enabled{ false };
//uintptr_t  ModLoadOrder::some_shared_ptr{ NULL };

/*
This file has been generated by IDA.
*/

#define __int8 char
#define __int16 short
#define __int32 int
#define __int64 long long
#define __cppobj  

struct MtPropertyList;
struct MtPropertyListVtbl;
struct MtObjectVtbl;
struct MtDTIVtbl;
struct MtUI;
struct MtProperty;
struct MtStream;
struct MtPropertyCustomPARAM;
struct MtPropertyCustomVtbl;

/* 8511 */
struct __cppobj MtCriticalSection
{
    BYTE mCSection[28];
};

/* 8561 */
struct __cppobj __declspec(align(4)) cSystem : MtObject
{
    MtCriticalSection mCS;
    bool mThreadSafe;
};

/* 10289 */
struct __cppobj sResourceTypeInfo : MtObject
{
    unsigned int mAttr;
    const char *mExt;
    const MtDTI *mpDTI;
};

/* 8663 */
struct __cppobj MtPropertyCustom
{
    MtPropertyCustomVtbl *vfptr;
};

/* 10290 */
struct __cppobj sResourceProperty : MtPropertyCustom
{
};

/* 10291 */
struct __declspec(align(4)) sResourceLOADING_INFO
{
    char path[64];
    unsigned int attr;
    bool active;
};

/* 10292 */
struct sResourceRESOURCEWORK
{
    cResource *presource;
    unsigned int offset;
    unsigned int orgsize;
    unsigned int datasize;
    unsigned int readsize;
    unsigned __int32 threadid : 31;
    unsigned __int32 complete : 1;
};

/* 10293 */
struct __cppobj sResourceDECODEWORK
{
    MtStream *pin;
    MtCriticalSection cs;
    cResource *parc;
    char *pbuf;
    unsigned int bufsize;
    unsigned int readpt;
    unsigned int writept;
    unsigned int rnum;
    unsigned int rrequest;
    unsigned int rcomplete;
    unsigned int cancel;
    unsigned int datasize;
    unsigned int orgsize;
    __int64 start_time;
    __int64 end_time;
    sResourceRESOURCEWORK rwork[4096];
};

/* 10294 */
struct __cppobj sResourceRemoteInfo : MtObject
{
    MtString mPCName;
    MtString mFolderName;
};

/* 10295 */
struct __cppobj __declspec(align(8)) sResource : cSystem
{
    sResourceTypeInfo mTypeInfo[256];
    unsigned int mTypeInfoNum;
    MtString mRootDirectory;
    MtString mResourcePath;
    MtString mNativePath;
    MtString mResourceFolder;
    MtString mNativeFolder;
    cResource *mpTable[8192];
    sResourceProperty mResourceProperty;
    bool mOptimizeEnable;
    bool mBuildComplete;
    bool mForceHDDCache;
    bool mForceBackGround;
    void *mLoaderThreadHandle;
    unsigned int mLoaderThreadID;
    unsigned int mAsyncPriority;
    bool mLoadEnd;
    bool mAutoUpdate;
    bool mCacheEnable;
    cResource *mpLoadList[1024];
    unsigned int mLoadNum;
    sResourceLOADING_INFO mLoadingInfo;
    sResourceDECODEWORK mDecodeWork;
    unsigned int mDecodeThreadMask;
    sResourceRemoteInfo mRemoteInfo;
    unsigned int mDecodeThreadNum;
    void *mDecodeThread[3];
    unsigned int mDecodeThreadID[3];
    void *mDecodeEvent[3];
    void *mDecodeSync[3];
};

/* 8665 */
struct MtPropertyCustomVtbl
{
    void *f1;
    void *f2;
    void *f3;
    void *f4;
    void *f5;
};

/* 8510 */
struct __cppobj MtStream : MtObject
{
};

/* 126 */
struct __cppobj MtUI
{
};

/* 127 */
struct __cppobj MtProperty
{
    const char *mName;
    unsigned __int16 mType;
    unsigned __int16 mAttr;
    MtObject *mpOwner;
    int ___u4;
    int ___u5;
    void* f1;
    void* f2;
    unsigned int mIndex;
    MtProperty *mpNext;
    MtProperty *mpPrev;
};

/* 120 */
struct __cppobj MtPropertyList
{
    MtPropertyListVtbl *vfptr;
    MtProperty *mpElement;
};

/* 123 */
struct MtDTIVtbl
{
    void *f1;
    MtObject *f2;
};

/* 8664 */
struct MtPropertyCustomPARAM
{
    const char *name;
    const char *value;
};

/* 121 */
struct MtPropertyListVtbl
{
    void *f1;
};

sResource** sDevil4ResourcePtr = (sResource**)0xE552D0;

BOOL FileExists(LPCTSTR szPath)
{
    DWORD dwAttrib = GetFileAttributes(szPath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}


void __stdcall reallyUnsafeStringWrite(char* path) {
    if (strstr(path, "mod"))
    {
        __try {
            path[0] = 'x';
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            return;
        }
    }
}

// we'll try to break the string passed to specialized
// path lookup function in sResource and hope it works
void __stdcall wew(MtDTI* dti, char* path) {
    static sResource* sDevil4Resource = *sDevil4ResourcePtr;
    char buf[MAX_PATH];
    memset(buf, 0, sizeof(buf));

    std::string extension;
    // dont wanna strcmp
    uint32_t type = *(uint32_t*)dti->mName;
    
    switch (type) { // TODO(): more types? too much rWhatever classes ;_;
    case 0x646F4D72: extension = "mod"; break;
    case 0x78655472: extension = "tex"; break;
    }

    // GetFileAttributes needs a full path iirc, unfortunate
    sprintf(buf, "%s\\%s.%s", sDevil4Resource->mResourcePath.value->str, path, extension.c_str());
    if (FileExists(buf)) 
    {
        reallyUnsafeStringWrite(path);
    }
}

//cResource* __userpurge sResource::create_sub_8DF530@<eax>(
// sResource* sDevil4Resource@<eax>, MtDTI* dti, const char* path,
// unsigned int mode)
naked void detour() {
    __asm {
        push ecx // original code
        push ebx
        push ebp
        push esi
        push edi
        pushad  // call our shitty hacks
        push eax // path is passed in eax
        mov ecx, [esp+3Ch] // dti is on the stack 
        push ecx
        call wew
        popad
        jmp dword ptr [jmp_return]
    }
#if 0
	__asm {
        push eax
        mov eax, [esp+0Ch]
        push eax
        call FileExists
        test al, al
        pop eax
        je originalCode

        int 3

    originalCode:
        mov esi, eax
        test esi, esi
        //je dword ptr [jmp_far]
        jmp dword ptr [jmp_return]
	}
#endif
}


void LoadOrder::toggle(bool enable) {
    if (enable) 
    {
        m_hook_iface = m_hook.hookJMP(0x8DFCF0, 5, detour);
        jmp_return = 0x8DFCF0 + 5;
    }
    else 
    {
        if (m_hook_iface) 
        {
            m_hook.unhook(m_hook_iface);
        }
    }
}

std::optional<std::string> LoadOrder::onInitialize() {
    /*if (!install_hook_absolute(0x8DFCF0, m_hook, &detour, &jmp_return, 5))
    {
        HL_LOG_ERR("Failed to init LoadOrder mod\n");
        return "Failed to init LoadOrder mod";
    }*/
	return Mod::onInitialize();
}

// onFrame()
// do something each frame example
//void ModLoadOrder::onFrame(fmilliseconds& dt) {}

// onConfigSave
// save your data into cfg structure.
void LoadOrder::onConfigSave(utils::Config& cfg) 
{ 
    cfg.set<bool>("LoadOrderHDD", m_enabled); 
};

// onConfigLoad
// load data into variables from config structure.
void LoadOrder::onConfigLoad(const utils::Config& cfg) 
{ 
    m_enabled = cfg.get<bool>("LoadOrderHDD").value_or(false);
    toggle(m_enabled);
};

// onGUIframe()
// draw your imgui widgets here, you are inside imgui context.
void LoadOrder::onGUIframe() 
{ 
    if (ImGui::Checkbox("WIP HDD file priority instead of .arc", &m_enabled))
    {
        toggle(m_enabled);
    }
};

// onGamePause()
// do something when toggling a gui
//void ModName::onGamePause(bool toggle) { };

// onMessage()
// handle some window message, return true to pass to the game window
// or false to drop it.
//bool ModName::onMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) { return true; };
