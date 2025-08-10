#include "debugger.h"
#include <vector>
namespace RoboDBG {

void Debugger::setBreakpoint(LPVOID address) {
    BYTE original;
    SIZE_T read;

    if(this->verbose){
    std::cout << "[*] Breakpoint set at " << std::hex << address << std::endl;
    }

    ReadProcessMemory(hProcessGlobal, address, &original, 1, &read);
    if(original != 0xCC) {
        BYTE int3 = 0xCC;
        breakpoints[address] = original; //TODO
        WriteProcessMemory(hProcessGlobal, address, &int3, 1, nullptr);
    }
    FlushInstructionCache(hProcessGlobal, address, 1);
}

bool Debugger::setHardwareBreakpointOnThread(hwBp_t bp) {
    HANDLE hThread = bp.hThread;

    // Validate length

    // Execute breakpoints must be 1 byte
    if (bp.type == AccessType::EXECUTE && bp.len != BreakpointLength::BYTE) {
        std::cerr << "[-] Execute breakpoints must be 1 byte (len=0)\n";
        return false;
    }

    CONTEXT ctx = {};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(hThread, &ctx)) {
        std::cerr << "[-] GetThreadContext failed: " << GetLastError() << "\n";
        return false;
    }

    // Assign address to correct DRx
    switch (bp.reg) {
        case DRReg::DR0: ctx.Dr0 = reinterpret_cast<DWORD_PTR>(bp.address); break;
        case DRReg::DR1: ctx.Dr1 = reinterpret_cast<DWORD_PTR>(bp.address); break;
        case DRReg::DR2: ctx.Dr2 = reinterpret_cast<DWORD_PTR>(bp.address); break;
        case DRReg::DR3: ctx.Dr3 = reinterpret_cast<DWORD_PTR>(bp.address); break;
        default:
            std::cerr << "[-] Invalid debug register index: " << static_cast<int>(bp.reg) << "\n";
            return false;
    }

    // Enable local breakpoint (L0–L3)
    ctx.Dr7 |= (1 << ( static_cast<int>(bp.reg) * 2));

    // Set type and size in DR7
    // Bits 16-31 control type and length for DR0-DR3
    // Each DR uses 4 bits: [LEN1][LEN0][R/W1][R/W0]
    const int shift = 16 + static_cast<int>(bp.reg) * 4;
    ctx.Dr7 &= ~(0xF << shift); // Clear previous settings
    ctx.Dr7 |= ((static_cast<int>(bp.type) | ( static_cast<int>(bp.len) << 2)) << shift); // Set new type and length

    if (SuspendThread(hThread) == (DWORD)-1) {
        std::cerr << "[-] SuspendThread failed: " << GetLastError() << "\n";
        return false;
    }

    if (!SetThreadContext(hThread, &ctx)) {
        std::cerr << "[-] SetThreadContext failed: " << GetLastError() << "\n";
        ResumeThread(hThread);
        return false;
    }

    if (ResumeThread(hThread) == (DWORD)-1) {
        std::cerr << "[-] ResumeThread failed: " << GetLastError() << "\n";
        return false;
    }

    // Save to global tracker
    hwBreakpoints[bp.address] = bp;
    return true;
}

bool Debugger::setHardwareBreakpoint(hwBp_t bp) {
    bool allSucceeded = true;

    actualizeThreadList( );
    // Validate register index
    if (static_cast<int>(bp.reg) < 0 || static_cast<int>(bp.reg) > 3) {
        std::cerr << "[-] Invalid debug register DR" << static_cast<int>(bp.reg) << "\n";
        return false;
    }

    const char* typeStr;
    switch (bp.type) {
        case AccessType::EXECUTE  : typeStr = "execute"; break;
        case AccessType::WRITE    : typeStr = "write"; break;
        case AccessType::READWRITE: typeStr = "read/write"; break;
        default: typeStr = "unknown"; break;
    }

    const char* lenStr;
    switch (static_cast<int>(bp.len)) {
        case 0: lenStr = "1 byte"; break;
        case 1: lenStr = "2 bytes"; break;
        case 2: lenStr = "8 bytes"; break;
        case 3: lenStr = "4 bytes"; break;
        default: lenStr = "unknown"; break;
    }
    actualizeThreadList();
    for (const auto& thread : threads) {
        hwBp_t perThreadBp = bp;
        perThreadBp.hThread = thread.hThread;

        if (!setHardwareBreakpointOnThread(perThreadBp)) {
            std::cerr << "[-] Failed to set hardware breakpoint on TID=" << thread.threadId << "\n";
            allSucceeded = false;
        } else {
            //std::cout << "[+] Hardware breakpoint set for TID=" << thread.threadId
            //<< " at address 0x" << std::hex << (DWORD_PTR)bp.address
            //<< " using DR" << perThreadBp.reg
            //<< " (" << typeStr << ", " << lenStr << ")\n";
        }
    }
    return allSucceeded;
}

// Helper function to clear a hardware breakpoint
bool Debugger::clearHardwareBreakpointOnThread(HANDLE hThread, DRReg reg) {
    if (static_cast<int>(reg) < 0 || static_cast<int>(reg) > 3) {
        std::cerr << "[-] Invalid debug register DR" << static_cast<int>(reg) << "\n";
        return false;
    }

    CONTEXT ctx = {};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    if (!GetThreadContext(hThread, &ctx)) {
        std::cerr << "[-] GetThreadContext failed: " << GetLastError() << "\n";
        return false;
    }

    LPVOID oldAddr = nullptr;
    switch (reg) {
        case DRReg::DR0: oldAddr = (LPVOID)ctx.Dr0; ctx.Dr0 = 0; break;
        case DRReg::DR1: oldAddr = (LPVOID)ctx.Dr1; ctx.Dr1 = 0; break;
        case DRReg::DR2: oldAddr = (LPVOID)ctx.Dr2; ctx.Dr2 = 0; break;
        case DRReg::DR3: oldAddr = (LPVOID)ctx.Dr3; ctx.Dr3 = 0; break;
    }

    ctx.Dr7 &= ~(1 << (static_cast<int>(reg) * 2));          // Disable local enable
    ctx.Dr7 &= ~(0xF << (16 + static_cast<int>(reg) * 4));   // Clear type/length

    if (SuspendThread(hThread) == (DWORD)-1) {
        std::cerr << "[-] SuspendThread failed: " << GetLastError() << "\n";
        return false;
    }

    bool success = true;

    if (!SetThreadContext(hThread, &ctx)) {
        std::cerr << "[-] SetThreadContext failed: " << GetLastError() << "\n";
        success = false;
    }

    if (ResumeThread(hThread) == (DWORD)-1) {
        std::cerr << "[-] ResumeThread failed: " << GetLastError() << "\n";
        success = false;
    }

    if (oldAddr && hwBreakpoints.count(oldAddr)) {
        const hwBp_t& stored = hwBreakpoints[oldAddr];
        if (stored.hThread == hThread && stored.reg == reg)
            hwBreakpoints.erase(oldAddr);
    }

    return success;
}

bool Debugger::clearHardwareBreakpoint(DRReg reg) {
    bool allSucceeded = true;

    actualizeThreadList();
    for (const auto& thread : threads) {
        if (!clearHardwareBreakpointOnThread(thread.hThread, reg))
            allSucceeded = false;
    }

    return allSucceeded;
}

std::vector<hwBp_t> Debugger::getHardwareBreakpoints() {
    std::vector<hwBp_t> result;

    for (const auto& thread : threads) {
        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

        if (!GetThreadContext(thread.hThread, &ctx)) continue;

        for (int i = 0; i < 4; ++i) {
            bool enabled = ctx.Dr7 & (1 << (i * 2));
            if (!enabled) continue;

            LPVOID addr = nullptr;
            switch (i) {
                case 0: addr = (LPVOID)ctx.Dr0; break;
                case 1: addr = (LPVOID)ctx.Dr1; break;
                case 2: addr = (LPVOID)ctx.Dr2; break;
                case 3: addr = (LPVOID)ctx.Dr3; break;
            }

            int type = (ctx.Dr7 >> (16 + i * 4)) & 0b11;
            int len  = (ctx.Dr7 >> (18 + i * 4)) & 0b11;

            hwBp_t bp = {
                thread.hThread,
                addr,
                static_cast<DRReg>(i),
                static_cast<AccessType>(type),
                static_cast<BreakpointLength>(len)
            };
            result.push_back(bp);
        }
    }

    return result;
}

hwBp_t Debugger::getBreakpointByReg(DRReg reg) {
    for (const auto& thread : threads) {
        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

        if (!GetThreadContext(thread.hThread, &ctx)) continue;

        bool enabled = ctx.Dr7 & (1 << (static_cast<int>(reg) * 2));
        if (!enabled) continue;

        LPVOID addr = nullptr;
        switch (reg) {
            case DRReg::DR0: addr = (LPVOID)ctx.Dr0; break;
            case DRReg::DR1: addr = (LPVOID)ctx.Dr1; break;
            case DRReg::DR2: addr = (LPVOID)ctx.Dr2; break;
            case DRReg::DR3: addr = (LPVOID)ctx.Dr3; break;
            default: return {}; // Invalid register index
        }

        int type = (ctx.Dr7 >> (16 + static_cast<int>(reg) * 4)) & 0b11;
        int len  = (ctx.Dr7 >> (18 + static_cast<int>(reg) * 4)) & 0b11;
        if(len<0 || len > 3) len = 0;

        hwBp_t bp = {
            thread.hThread,
            addr,
            reg,
            static_cast<AccessType>(type),
            static_cast<BreakpointLength>(len)
        };
        return bp;
    }

    return {}; // Not found
}

DRReg Debugger::isHardwareBreakpointAt(LPVOID address) {
    for (const auto& thread : threads) {
        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

        if (!GetThreadContext(thread.hThread, &ctx)) continue;

        if ((ctx.Dr7 & 0x1 ) && ctx.Dr0 == (DWORD_PTR)address) return DRReg::DR0;
        if ((ctx.Dr7 & 0x4 ) && ctx.Dr1 == (DWORD_PTR)address) return DRReg::DR1;
        if ((ctx.Dr7 & 0x10) && ctx.Dr2 == (DWORD_PTR)address) return DRReg::DR2;
        if ((ctx.Dr7 & 0x40) && ctx.Dr3 == (DWORD_PTR)address) return DRReg::DR3;
    }

    return DRReg::NOP;
}


void Debugger::restoreBreakpoint(LPVOID address) {
    auto it = breakpoints.find(address);
    if (it != breakpoints.end()) {
        if(this->verbose) std::cout << "[~] Replacing breakpoint " << std::hex << static_cast<int>(it->second) << " at " << std::hex << address << std::endl;
        WriteProcessMemory(hProcessGlobal, address, &it->second, 1, nullptr);
        FlushInstructionCache(hProcessGlobal, address, 1);
    } else {
        printf("WARNING: BREAKPOINT NOT FOUND!\n");
    }
}

}
