/**
 * @file imports.h
 * @brief Plugin to handle DLL imports
 * @author Milkshake
 */
// Build example (with a separate main): cl /std:c++20 /EHsc main.cpp imports.cpp /link Psapi.lib
#pragma once

#include <windows.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "basePlugin.h"

class Imports : BasePlugin {
public:
    struct ModuleInfo {
        std::string modulePath;
        HMODULE     moduleBase;
    };

    struct IatRecord {
        std::string modulePath;
        HMODULE moduleBase;
        std::string dllName;
        bool byOrdinal;
        WORD ordinal;
        std::string funcName;
        uintptr_t iatSlot;
        uintptr_t target;
        uintptr_t iatBase;
        size_t index;
    };

    struct FunctionAddress {
        uintptr_t iatSlot;
        uintptr_t target;
    };

    Imports(HANDLE hProcess);
    ~Imports();

    bool collect(bool alsoPrint = false);

    const std::vector<IatRecord>& entries() const { return entries_; }

    std::optional<FunctionAddress> findImport(std::string_view module, std::string_view dll, std::string_view func) const;
    std::vector<FunctionAddress> findImports(std::string_view dll, std::string_view func) const;
    std::vector<FunctionAddress> findImports(HMODULE moduleBase, std::string_view dll, std::string_view func) const;
    std::vector<FunctionAddress> findImportsByDll(std::string_view dll) const;
    std::vector<FunctionAddress> findImportsByName(std::string_view func) const;
    std::vector<ModuleInfo> listModules() const;

private:
    struct NtInfo {
        bool is64 = false;
        DWORD sizeOfImage = 0;
        IMAGE_DATA_DIRECTORY importDir {};
    };

    // helper to find out how many bytes remain from va to end of module
    static inline SIZE_T spanLeftInModule(HMODULE base, uintptr_t va, DWORD sizeOfImage) {
        const uintptr_t start = reinterpret_cast<uintptr_t>(base);
        if (va < start) return 0;
        const uintptr_t off = va - start;
        if (off >= sizeOfImage) return 0;
        return static_cast<SIZE_T>(sizeOfImage - off);
    }

    std::string readCString(uintptr_t addr, SIZE_T maxLen);
    inline BYTE* rvaToVa(HMODULE base, DWORD rva) { return (BYTE*)base + rva; }
    bool getNtInfo(HMODULE base, NtInfo& out);

    static std::string_view baseName(std::string_view s);
    static bool iequals(std::string_view a, std::string_view b);
    static bool dllEquals(std::string_view a, std::string_view b);
    static bool funcEquals(const IatRecord& r, std::string_view func);

    void collectModuleIat(HMODULE base, const char* modulePath, bool alsoPrint);

private:
    std::vector<IatRecord> entries_;
};
