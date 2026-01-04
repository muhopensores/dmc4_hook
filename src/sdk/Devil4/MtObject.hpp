#pragma once

class MtDTI;

class MtObject {
public:

    MtObject(const MtObject&)            = default;
    MtObject(MtObject&&)                 = delete;
    MtObject& operator=(const MtObject&) = default;
    MtObject& operator=(MtObject&&)      = delete;

    virtual ~MtObject()                  = default;
    virtual void   vec_del_dtor(unsigned int i) {};            // 0x00
    virtual void   create_ui(void* prop) {};                   // 0x04
    virtual bool   is_enable_instance() { return true; };      // 0x08
    virtual void   create_property(void* prop) {};             // 0x0C
    virtual MtDTI* get_dti() { return (MtDTI*)0x00E5C5A8; };   // 0x10

}; // Size: 0x0004
static_assert(sizeof(MtObject) == 0x04);

