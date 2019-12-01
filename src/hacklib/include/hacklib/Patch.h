#ifndef HACKLIB_PATCH_H
#define HACKLIB_PATCH_H

#include <cstddef>
#include <cstdint>
#include <vector>


namespace hl
{
class Patch
{
public:
    Patch() = default;
    Patch(Patch&& p) = default;
    ~Patch();

    // Applies a patch. Any previous patch done by the instance is reverted before.
    void apply(uintptr_t location, const char* patch, size_t size);

    template <typename T>
    void apply(uintptr_t location, T patch)
    {
        apply(location, (const char*)&patch, sizeof(patch));
    }

    void revert();

private:
    std::vector<unsigned char> m_backup;
    uintptr_t m_location = 0;
    size_t m_size = 0;
};


Patch MakePatch(uintptr_t location, const char* patch, size_t size);

template <typename T>
Patch MakePatch(uintptr_t location, T patch)
{
    return MakePatch(location, (const char*)&patch, sizeof(patch));
}
}

#endif
