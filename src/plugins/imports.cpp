// imports.cpp
#include "imports.h"

#include <psapi.h>

#include <cctype>
#include <cstddef> // offsetof
#include <cstdlib> // strtoul
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <algorithm>
#include <unordered_set>

Imports::Imports(HANDLE hProcess)
{
    hProcess_ = hProcess;
}

Imports::~Imports() = default; // does not close hProcess_

bool Imports::collect(bool alsoPrint)
{
    entries_.clear();
    if (!hProcess_)
        return false;

    HMODULE mods[1024];
    DWORD needed = 0;
    if (!EnumProcessModulesEx(hProcess_, mods, sizeof(mods), &needed, LIST_MODULES_ALL))
        return false;

    const unsigned count = needed / sizeof(HMODULE);
    for (unsigned i = 0; i < count; ++i) {
        char path[MAX_PATH] = { 0 };
        GetModuleFileNameExA(hProcess_, mods[i], path, MAX_PATH);
        if (alsoPrint) {
            std::cout << "Module " << path << " Base=0x" << std::hex
                      << (uintptr_t)mods[i] << std::dec << "\n";
        }
        collectModuleIat(mods[i], path, alsoPrint);
    }
    return true;
}

// helper
static inline bool moduleEqualsName(std::string_view path, std::string_view wantBase)
{
    auto baseName = [](std::string_view s){
        size_t p = s.find_last_of("\\/");
        return (p == std::string_view::npos) ? s : s.substr(p + 1);
    };
    std::string_view a = baseName(path);
    std::string_view b = baseName(wantBase);
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::toupper((unsigned char)a[i]) != std::toupper((unsigned char)b[i])) return false;
        return true;
}

std::optional<Imports::FunctionAddress> Imports::findImport(std::string_view module, std::string_view dll, std::string_view func) const
{
    const IatRecord* hit = nullptr;
    for (const auto& r : entries_) {
        if (!moduleEqualsName(r.modulePath, module)) continue;
        if (!dllEquals(r.dllName, dll)) continue;
        if (!funcEquals(r, func)) continue;
        if (hit) return std::nullopt;            // ambiguous within module
        hit = &r;
    }
    if (!hit) return std::nullopt;               // no match
    return FunctionAddress{ hit->iatSlot, hit->target };
}

// return all matches across all modules
std::vector<Imports::FunctionAddress> Imports::findImports(std::string_view dll, std::string_view func) const
{
    std::vector<FunctionAddress> out;
    out.reserve(64);
    for (const auto& r : entries_) {
        if (!dllEquals(r.dllName, dll)) continue;
        if (!funcEquals(r, func)) continue;
        out.push_back(FunctionAddress{ r.iatSlot, r.target });
    }
    return out;
}

// restrict to a module by HMODULE
std::vector<Imports::FunctionAddress> Imports::findImports(HMODULE moduleBase, std::string_view dll, std::string_view func) const
{
    std::vector<FunctionAddress> out;
    out.reserve(32);
    for (const auto& r : entries_) {
        if (r.moduleBase != moduleBase) continue;
        if (!dllEquals(r.dllName, dll)) continue;
        if (!funcEquals(r, func)) continue;
        out.push_back(FunctionAddress{ r.iatSlot, r.target });
    }
    return out;
}

// list unique modules seen during collect()
std::vector<Imports::ModuleInfo> Imports::listModules() const
{
    std::vector<ModuleInfo> out;
    out.reserve(64);
    std::unordered_set<HMODULE> seen;
    seen.reserve(64);

    for (const auto& r : entries_) {
        if (seen.insert(r.moduleBase).second) {
            out.push_back(ModuleInfo{ r.modulePath, r.moduleBase });
        }
    }
    std::sort(out.begin(), out.end(),
              [](const ModuleInfo& a, const ModuleInfo& b){ return a.moduleBase < b.moduleBase; });
    return out;
}

std::vector<Imports::FunctionAddress>
Imports::findImportsByDll(std::string_view dll) const
{
    std::vector<FunctionAddress> out;
    out.reserve(64);
    for (const auto& r : entries_) {
        if (!dllEquals(r.dllName, dll))
            continue;
        out.push_back(FunctionAddress { r.iatSlot, r.target });
    }
    return out;
}

std::vector<Imports::FunctionAddress>
Imports::findImportsByName(std::string_view func) const
{
    std::vector<FunctionAddress> out;
    out.reserve(64);
    for (const auto& r : entries_) {
        if (!funcEquals(r, func))
            continue;
        out.push_back(FunctionAddress { r.iatSlot, r.target });
    }
    return out;
}

// ---- internals ----

std::string Imports::readCString(uintptr_t addr, SIZE_T maxLen)
{
    std::string s;
    s.reserve(64);
    BYTE chunk[256];
    SIZE_T off = 0;
    while (off < maxLen) {
        const SIZE_T toRead = std::min<SIZE_T>(sizeof(chunk), maxLen - off);
        if (!this->readMemory(addr + off, chunk, toRead)) break;
        for (SIZE_T i = 0; i < toRead; ++i) {
            if (chunk[i] == 0) {
                s.append(reinterpret_cast<const char*>(chunk), i);
                return s;
            }
        }
        s.append(reinterpret_cast<const char*>(chunk), toRead);
        off += toRead;
    }
    return s;
}

bool Imports::getNtInfo(HMODULE base, NtInfo& out)
{
    auto dos = this->readMemory<IMAGE_DOS_HEADER>(reinterpret_cast<uintptr_t>(base));
    if (dos.e_magic != IMAGE_DOS_SIGNATURE)
        return false;

    auto nt = reinterpret_cast<uintptr_t>(base) + dos.e_lfanew;
    auto sig = this->readMemory<DWORD>(nt);
    if (sig != IMAGE_NT_SIGNATURE)
        return false;

    auto fh = this->readMemory<IMAGE_FILE_HEADER>(nt + sizeof(DWORD));
    WORD magic = this->readMemory<WORD>(nt + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER));

    if (magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        auto nth = this->readMemory<IMAGE_NT_HEADERS64>(nt);
        out.is64 = true;
        out.sizeOfImage = nth.OptionalHeader.SizeOfImage;
        out.importDir = nth.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    } else if (magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        auto nth = this->readMemory<IMAGE_NT_HEADERS32>(nt);
        out.is64 = false;
        out.sizeOfImage = nth.OptionalHeader.SizeOfImage;
        out.importDir = nth.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    } else {
        return false;
    }
    return out.importDir.VirtualAddress && out.importDir.Size;
}

std::string_view Imports::baseName(std::string_view s)
{
    size_t p = s.find_last_of("\\/");
    return (p == std::string_view::npos) ? s : s.substr(p + 1);
}

bool Imports::iequals(std::string_view a, std::string_view b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i) {
        unsigned char x = static_cast<unsigned char>(a[i]);
        unsigned char y = static_cast<unsigned char>(b[i]);
        if (std::toupper(x) != std::toupper(y))
            return false;
    }
    return true;
}

bool Imports::dllEquals(std::string_view a, std::string_view b)
{
    return iequals(baseName(a), baseName(b));
}

bool Imports::funcEquals(const IatRecord& r, std::string_view func)
{
    if (!func.empty() && func.front() == '#') {
        unsigned long v = std::strtoul(std::string(func.substr(1)).c_str(), nullptr, 10);
        return r.byOrdinal && r.ordinal == static_cast<WORD>(v);
    }
    if (r.byOrdinal)
        return false;
    return iequals(r.funcName, func);
}

void Imports::collectModuleIat(HMODULE base, const char* modulePath, bool alsoPrint)
{
    NtInfo ni {};
    if (!getNtInfo(base, ni))
        return;

    const SIZE_T psize = ni.is64 ? 8 : 4;
    const ULONGLONG ordFlag = ni.is64 ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32;

    BYTE* impBase = rvaToVa(base, ni.importDir.VirtualAddress);

    for (DWORD idx = 0;; ++idx) {
        IMAGE_IMPORT_DESCRIPTOR imp {};
        BYTE* impAddr = impBase + idx * sizeof(imp);
        imp = this->readMemory<IMAGE_IMPORT_DESCRIPTOR>(reinterpret_cast<uintptr_t>(impAddr));
        if (imp.Name == 0 && imp.FirstThunk == 0 && imp.OriginalFirstThunk == 0)
            break;

        std::string dllName;
        if (imp.Name) {
            const uintptr_t dllVA = reinterpret_cast<uintptr_t>(rvaToVa(base, imp.Name));
            const SIZE_T maxLen   = spanLeftInModule(base, dllVA, ni.sizeOfImage);
            dllName = readCString(dllVA, maxLen);
        } else {
            dllName = "<no-name>";
        }
        DWORD oftRva = imp.OriginalFirstThunk ? imp.OriginalFirstThunk : imp.FirstThunk;
        DWORD ftRva = imp.FirstThunk;
        if (oftRva >= ni.sizeOfImage || ftRva >= ni.sizeOfImage)
            continue;

        BYTE* oft = rvaToVa(base, oftRva);
        BYTE* ft = rvaToVa(base, ftRva);

        if (alsoPrint) {
            std::cout << "=== DLL=" << dllName
                      << "  IAT_BASE=0x" << std::hex << std::setw(ni.is64 ? 16 : 8)
                      << std::setfill('0') << (uintptr_t)ft << std::dec << " ===\n";
        }

        for (SIZE_T i = 0;; ++i) {
            ULONGLONG thunkVal = 0;
            if (ni.is64)
                thunkVal = this->readMemory<ULONGLONG>(reinterpret_cast<uintptr_t>(oft + i * psize));
            else
                thunkVal = this->readMemory<DWORD>(reinterpret_cast<uintptr_t>(oft + i * psize));

            if (thunkVal == 0)
                break;

            BYTE* slot = ft + i * psize;
            ULONGLONG target = 0;
            if (ni.is64)
                target = this->readMemory<ULONGLONG>(reinterpret_cast<uintptr_t>(slot));
            else
                target = this->readMemory<DWORD>(reinterpret_cast<uintptr_t>(slot));

            IatRecord rec {};
            rec.modulePath = modulePath ? modulePath : "";
            rec.moduleBase = base;
            rec.dllName = dllName;
            rec.iatBase = (uintptr_t)ft;
            rec.index = i;
            rec.iatSlot = (uintptr_t)slot;
            rec.target = (uintptr_t)target;

            if (thunkVal & ordFlag) {
                rec.byOrdinal = true;
                rec.ordinal = static_cast<WORD>(thunkVal & 0xFFFFULL);
            } else {
                const DWORD ibnRva = static_cast<DWORD>(thunkVal & ~ordFlag);
                const uintptr_t nameVA = reinterpret_cast<uintptr_t>(
                    rvaToVa(base, ibnRva)) + offsetof(IMAGE_IMPORT_BY_NAME, Name);
                rec.byOrdinal = false;
                rec.ordinal = 0;
                // Try to read name; empty => failed.
                const SIZE_T nameMax = spanLeftInModule(base, nameVA, ni.sizeOfImage);
                auto name = readCString(nameVA, nameMax);
                rec.funcName = name.empty() ? "<name-read-failed>" : std::move(name);
            }

            if (alsoPrint) {
                std::cout << std::hex << std::setfill('0')
                << "DLL=" << rec.dllName
                << "  Func=" << (rec.byOrdinal ? ("#" + std::to_string(rec.ordinal)) : rec.funcName)
                << "  IAT_SLOT=0x" << std::setw(ni.is64 ? 16 : 8) << rec.iatSlot
                << "  TARGET=0x" << std::setw(ni.is64 ? 16 : 8) << rec.target
                << std::dec << "\n";
            }

            entries_.emplace_back(std::move(rec));
        }
    }
}
