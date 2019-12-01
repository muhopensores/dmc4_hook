#include "hacklib/ExeFile.h"
#include <algorithm>
#include <fstream>


bool hl::ExeFile::loadFromFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    size_t size = (size_t)file.tellg();
    file.seekg(0, std::ios::beg);
    if (size == (size_t)-1)
    {
        return false;
    }

    std::vector<char> buf(size);
    if (file.read(buf.data(), size))
    {
        return loadFromMem((uintptr_t)buf.data());
    }
    return false;
}

bool hl::ExeFile::hasRelocs() const
{
    if (!m_valid)
    {
        throw std::runtime_error("no exe file loaded");
    }

    return !m_relocs.empty();
}

bool hl::ExeFile::isReloc(uintptr_t rva) const
{
    if (!m_valid)
    {
        throw std::runtime_error("no exe file loaded");
    }

    auto it = std::find(m_relocs.begin(), m_relocs.end(), rva);
    return it != m_relocs.end();
}

uintptr_t hl::ExeFile::getExport(const std::string& name) const
{
    auto it = m_exports.find(name);
    if (it != m_exports.end())
    {
        return it->second;
    }

    return 0;
}
