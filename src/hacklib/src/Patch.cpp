#include "hacklib/Patch.h"
#include "hacklib/PageAllocator.h"
#include <cstring>


using namespace hl;


Patch::~Patch()
{
    revert();
}


void Patch::apply(uintptr_t location, const char* patch, size_t size)
{
    // Can only hold one patch at a time.
    revert();

    // Backup original data.
    m_backup.resize(size);
    memcpy(m_backup.data(), (void*)location, size);

    // Apply the patch.
    hl::PageProtect((void*)location, size, PROTECTION_READ_WRITE_EXECUTE);
    memcpy((void*)location, patch, size);
    hl::PageProtect((void*)location, size, PROTECTION_READ_EXECUTE);

    m_location = location;
    m_size = size;
}

void Patch::revert()
{
    if (m_size)
    {
        hl::PageProtect((void*)m_location, m_size, PROTECTION_READ_WRITE_EXECUTE);
        memcpy((void*)m_location, m_backup.data(), m_size);
        hl::PageProtect((void*)m_location, m_size, PROTECTION_READ_EXECUTE);

        m_size = 0;
    }
}


hl::Patch MakePatch(uintptr_t location, const char* patch, size_t size)
{
    Patch p;
    p.apply(location, patch, size);
    return p;
}
