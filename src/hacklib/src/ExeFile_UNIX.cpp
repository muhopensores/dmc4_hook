#include "hacklib/ExeFile.h"
#include <elf.h>
#include <string.h>


#ifdef ARCH_64BIT
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Phdr Elf_Phdr;
typedef Elf64_Shdr Elf_Shdr;
typedef Elf64_Sym Elf_Sym;
#else
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Phdr Elf_Phdr;
typedef Elf32_Shdr Elf_Shdr;
typedef Elf32_Sym Elf_Sym;
#endif


class hl::ExeFileImpl
{
public:
    Elf_Ehdr* elfHeader = nullptr;
    Elf_Shdr* sectionHeaders = nullptr;
    Elf_Shdr* strTableHeader = nullptr;
    char* strTable = nullptr;
};


static std::unordered_map<std::string, uintptr_t> LoadSymbols(Elf_Sym* symTable, size_t symTableNum, char* strTable)
{
    std::unordered_map<std::string, uintptr_t> symbols;

    for (size_t i = 0; i < symTableNum; i++)
    {
        auto sym = &symTable[i];
        const char* name = sym->st_name > 0 ? &strTable[sym->st_name] : "";
        if (name)
        {
            symbols[name] = (uintptr_t)sym->st_value;
        }
    }

    return symbols;
}


hl::ExeFile::ExeFile()
{
    m_impl = std::make_unique<ExeFileImpl>();
}

hl::ExeFile::~ExeFile() {}


bool hl::ExeFile::loadFromMem(uintptr_t moduleBase)
{
    m_valid = false;

    m_impl->elfHeader = (Elf_Ehdr*)moduleBase;
    if (m_impl->elfHeader->e_ident[EI_MAG0] != ELFMAG0 || m_impl->elfHeader->e_ident[EI_MAG1] != ELFMAG1 ||
        m_impl->elfHeader->e_ident[EI_MAG2] != ELFMAG2 || m_impl->elfHeader->e_ident[EI_MAG3] != ELFMAG3)
    {
        return false;
    }

    if (m_impl->elfHeader->e_ident[EI_VERSION] != EV_CURRENT)
    {
        return false;
    }

#ifdef ARCH_64BIT
    if (m_impl->elfHeader->e_ident[EI_CLASS] != ELFCLASS64 && m_impl->elfHeader->e_machine != EM_X86_64)
#else
    if (m_impl->elfHeader->e_ident[EI_CLASS] != ELFCLASS32 && m_impl->elfHeader->e_machine != EM_386)
#endif
    {
        return false;
    }

    if (m_impl->elfHeader->e_ident[EI_OSABI] != ELFOSABI_LINUX && m_impl->elfHeader->e_ident[EI_OSABI] != ELFOSABI_SYSV)
    {
        return false;
    }

    size_t strTableSectionIndex = m_impl->elfHeader->e_shstrndx;
    m_impl->sectionHeaders = (Elf_Shdr*)(moduleBase + m_impl->elfHeader->e_shoff);
    m_impl->strTableHeader = &m_impl->sectionHeaders[strTableSectionIndex];
    m_impl->strTable = (char*)(moduleBase + m_impl->strTableHeader->sh_offset);

    size_t numSections = m_impl->elfHeader->e_shnum;
    size_t symTableSectionIndex = -1;
    for (size_t i = 0; i < numSections; i++)
    {
        auto section = &m_impl->sectionHeaders[i];

        switch (section->sh_type)
        {
        case SHT_SYMTAB:
        case SHT_DYNSYM:
            symTableSectionIndex = i;
            break;

        case SHT_STRTAB:
            if (i != strTableSectionIndex && symTableSectionIndex != (size_t)-1)
            {
                char* strTable = (char*)(moduleBase + m_impl->sectionHeaders[i].sh_offset);
                Elf_Shdr* symTableSectionHeader = &m_impl->sectionHeaders[symTableSectionIndex];
                Elf_Sym* symTable = (Elf_Sym*)(moduleBase + symTableSectionHeader->sh_offset);
                size_t symTableNum = symTableSectionHeader->sh_size / symTableSectionHeader->sh_entsize;
                auto symbols = LoadSymbols(symTable, symTableNum, strTable);
                m_exports.insert(symbols.begin(), symbols.end());
                symTableSectionIndex = -1;
            }
        }
    }

    m_valid = true;
    return true;
}
