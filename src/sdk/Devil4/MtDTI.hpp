#pragma once 

class MtDTI
{
public:
    using uint32_t = unsigned int;

    virtual ~MtDTI() = default;
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
static_assert(sizeof(MtDTI) == 0x0020);
