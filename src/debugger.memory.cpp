#include "debugger.h"

namespace RoboDBG {

bool Debugger::writeMemory(LPVOID address, const void* buffer, SIZE_T size) {
    SIZE_T bytesWritten = 0;
    if (!WriteProcessMemory(hProcessGlobal,address, buffer, size, &bytesWritten) || bytesWritten != size) {
        std::cerr << "WriteProcessMemory failed: " << GetLastError() << std::endl;
        return false;
    }
    FlushInstructionCache(hProcessGlobal, address, size);
    return true;
}

bool Debugger::readMemory( LPVOID address, void* buffer, SIZE_T size) {
    SIZE_T bytesRead = 0;
    if (!ReadProcessMemory(hProcessGlobal, address, buffer, size, &bytesRead) || bytesRead != size) {
        std::cerr << "ReadProcessMemory failed: " << GetLastError() << std::endl;
        return false;
    }
    return true;
}

MemoryRegion_t Debugger::getPageByAddress(LPVOID baseAddress) {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    LPVOID addr = sysInfo.lpMinimumApplicationAddress;
    MEMORY_BASIC_INFORMATION mbi;

    while (addr < sysInfo.lpMaximumApplicationAddress) {
        if (VirtualQueryEx(hProcessGlobal, addr, &mbi, sizeof(mbi)) == 0)
            break;

        BYTE* regionStart = static_cast<BYTE*>(mbi.BaseAddress);
        BYTE* regionEnd   = regionStart + mbi.RegionSize;
        BYTE* target      = static_cast<BYTE*>(baseAddress);

        if (target >= regionStart && target < regionEnd) {
            return MemoryRegion_t{
                mbi.BaseAddress,
                mbi.RegionSize,
                mbi.State,
                mbi.Protect,
                mbi.Type
            };
        }

        addr = regionEnd;
    }

    // Return invalid result
    return MemoryRegion_t{ nullptr, 0, 0, 0, 0 };
}

bool Debugger::changeMemoryProtection(MemoryRegion_t page, DWORD newProtect) { //TODO: Use AccessRights enum instead of Memory Protection Constants (https://learn.microsoft.com/en-us/windows/win32/Memory/memory-protection-constants)
    return changeMemoryProtection(page.BaseAddress, page.RegionSize, newProtect);
}

// -------------------------------------------------------------
// Changes Memory Protection of a Memory Region
// -------------------------------------------------------------
bool Debugger::changeMemoryProtection(LPVOID baseAddress, SIZE_T regionSize, DWORD newProtect) {
    DWORD oldProtect;
    if (VirtualProtectEx(hProcessGlobal, baseAddress, regionSize, newProtect, &oldProtect)) {
        std::cout << "[+] Changed protection at " << baseAddress
        << " from 0x" << std::hex << oldProtect
        << " to 0x" << newProtect << std::endl;
        return true;
    } else {
        std::cerr << "[-] Failed to change protection at " << baseAddress
        << " - Error: " << GetLastError() << std::endl;
        return false;
    }
}


// -------------------------------------------------------------
// Returns all readable, committed memory regions
// -------------------------------------------------------------
std::vector<MemoryRegion_t> Debugger::getMemoryPages() {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    LPVOID addr = sysInfo.lpMinimumApplicationAddress;
    MEMORY_BASIC_INFORMATION mbi;
    std::vector<MemoryRegion_t> regions;

    while (addr < sysInfo.lpMaximumApplicationAddress) {
        if (VirtualQueryEx(hProcessGlobal, addr, &mbi, sizeof(mbi)) == 0)
            break;

        MemoryRegion_t region;
        region.BaseAddress = mbi.BaseAddress;
        region.RegionSize = mbi.RegionSize;
        region.State = mbi.State;
        region.Protect = mbi.Protect;
        region.Type = mbi.Type;

        regions.push_back(region);

        addr = static_cast<BYTE*>(mbi.BaseAddress) + mbi.RegionSize;
    }

    return regions;
}

// -------------------------------------------------------------
// Searches for a binary pattern in all valid memory regions
// -------------------------------------------------------------
std::vector<uintptr_t> Debugger::searchInMemory(const std::vector<BYTE>& pattern) {
    std::vector<uintptr_t> matches;
    auto regions = getMemoryPages();  // Ensure correct capitalization if needed

    for (const auto& region : regions) {
        // Skip regions that are not committed or inaccessible
        if (region.State != MEM_COMMIT || (region.Protect & PAGE_GUARD) || (region.Protect == PAGE_NOACCESS))
            continue;

        std::vector<BYTE> buffer(region.RegionSize);
        SIZE_T bytesRead;

        if (ReadProcessMemory(hProcessGlobal, region.BaseAddress, buffer.data(), region.RegionSize, &bytesRead)) {
            for (SIZE_T i = 0; i + pattern.size() <= bytesRead; ++i) {
                if (memcmp(buffer.data() + i, pattern.data(), pattern.size()) == 0) {
                    matches.push_back(reinterpret_cast<uintptr_t>(region.BaseAddress) + i);
                }
            }
        }
    }

    return matches;
}

// -------------------------------------------------------------
// prints all memory pages for debugging reasonss
// -------------------------------------------------------------
void Debugger::PrintMemoryPages() {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    LPVOID addr = sysInfo.lpMinimumApplicationAddress;
    MEMORY_BASIC_INFORMATION mbi;

    while (addr < sysInfo.lpMaximumApplicationAddress) {
        if (VirtualQueryEx(hProcessGlobal, addr, &mbi, sizeof(mbi)) == 0)
            break;

        std::cout << "BaseAddr: " << mbi.BaseAddress
        << " | RegionSize: " << mbi.RegionSize
        << " | State: " << std::hex << mbi.State
        << " | Protect: " << mbi.Protect
        << " | Type: " << mbi.Type << std::endl;

        addr = static_cast<BYTE*>(mbi.BaseAddress) + mbi.RegionSize;
    }
}

}
