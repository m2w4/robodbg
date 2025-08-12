#include "debugger.h"
#include <iostream>   // ensure iostream is visible

namespace RoboDBG {

    // ==================================================
    // HOOKS
    // ==================================================
    void Debugger::onStart(uintptr_t imageBase, uintptr_t entryPoint) {
        if (!this->verbose) return;

        std::cout << "[*] Debugger::onStart\n";
        std::cout << "    ImageBase : 0x" << std::hex << imageBase << "\n";
        std::cout << "    EntryPoint: 0x" << std::hex << entryPoint << std::dec << "\n";
    }

    void Debugger::onEnd(DWORD exitCode, DWORD pid) {
        if (!this->verbose) return;

        std::cout << "[*] Process exited\n";
        std::cout << "    PID       : " << pid << "\n";
        std::cout << "    Exit Code : 0x" << std::hex << exitCode
        << std::dec << " (" << exitCode << ")\n";
    }

    void Debugger::onAttach() {
        if (!this->verbose) return;

        std::cout << "[*] Debugger::onAttach: Attached\n";
    }

    BreakpointAction Debugger::onBreakpoint(uintptr_t address, HANDLE hThread) {
        if (this->verbose) {
            std::cout << "[*] Breakpoint hit\n";
            std::cout << "    Address: 0x" << std::hex << address
            << "  Thread: 0x" << reinterpret_cast<uintptr_t>(hThread)
            << std::dec << "\n";
        }
        return RESTORE;
    }

    bool Debugger::onDLLLoad(uintptr_t address, std::string dllName, uintptr_t entryPoint) {
        if (!this->verbose) return false;

        std::cout << "[*] DLL Load\n";
        std::cout << "    Address    : 0x" << std::hex << address << "\n";
        std::cout << "    DLL Name   : " << std::dec << dllName << "\n";
        std::cout << "    Entry Point: 0x" << std::hex << entryPoint << std::dec << "\n";
        return false;
    }

    void Debugger::onThreadCreate(HANDLE hThread, DWORD threadId, uintptr_t threadBase, uintptr_t startAddress) {
        if (!this->verbose) return;

        std::cout << "[*] Thread created\n";
        std::cout << "    TID          : " << std::dec << threadId << "\n";
        std::cout << "    Handle       : 0x" << std::hex << reinterpret_cast<uintptr_t>(hThread) << "\n";
        std::cout << "    TEB/Base     : 0x" << std::hex << threadBase << "\n";
        std::cout << "    StartAddress : 0x" << std::hex << startAddress << std::dec << "\n";
    }

    void Debugger::onThreadExit(DWORD threadID) {
        if (!this->verbose) return;

        std::cout << "[*] Thread exited\n";
        std::cout << "    TID: " << std::dec << threadID << "\n";
    }

    void Debugger::onDLLUnload(uintptr_t address, std::string dllName) {
        if (!this->verbose) return;

        std::cout << "[*] DLL Unload\n";
        std::cout << "    Address : 0x" << std::hex << address << "\n";
        std::cout << "    DLL Name: " << std::dec << dllName << "\n";
    }

    void Debugger::onSinglestep(uintptr_t address, HANDLE hThread) {
        if (!this->verbose) return;

        std::cout << "[*] Single-step\n";
        std::cout << "    Address: 0x" << std::hex << address
        << "  Thread: 0x" << reinterpret_cast<uintptr_t>(hThread)
        << std::dec << "\n";
    }

    void Debugger::onAccessViolation(uintptr_t address, uintptr_t faultingAddress, long accessType) {
        if (!this->verbose) return;

        std::cout << "[!] Access violation\n";
        std::cout << "    At Address    : 0x" << std::hex << address << "\n";
        std::cout << "    Faulting Addr : 0x" << std::hex << faultingAddress << "\n";
        std::cout << "    Access Type   : " << std::dec << accessType << " -> ";

        switch (accessType) {
            case 0: std::cout << "Read\n"; break;
            case 1: std::cout << "Write\n"; break;
            case 8: std::cout << "Execute (NX)\n"; break;
            default: std::cout << "Unknown\n"; break;
        }
    }

    void Debugger::onDebugString(std::string msg) {
        if (!this->verbose) return;

        std::cout << "[*] Debug string\n";
        std::cout << "    \"" << msg << "\"\n";
    }

    void Debugger::onRIPError(const RIP_INFO& rip) {
        if (!this->verbose) return;

        // We don't assume fields; print the address of the struct so it's referenced.
        std::cout << "[*] RIP error\n";
        std::cout << "    RIP_INFO@0x" << std::hex << reinterpret_cast<uintptr_t>(&rip) << std::dec << "\n";
    }

    void Debugger::onUnknownException(uintptr_t addr, DWORD code) {
        if (!this->verbose) return;

        std::cout << "[*] Unknown exception\n";
        std::cout << "    Code : 0x" << std::hex << code << "\n";
        std::cout << "    Addr : 0x" << std::hex << addr << std::dec << "\n";
    }

    void Debugger::onUnknownDebugEvent(DWORD code) {
        if (!this->verbose) return;

        std::cout << "[*] Unknown debug event\n";
        std::cout << "    Code: " << std::dec << code << "\n";
    }

    BreakpointAction Debugger::onHardwareBreakpoint(uintptr_t address, HANDLE hThread, DRReg reg) {
        if (this->verbose) {
            std::cout << "[*] Hardware Breakpoint\n";
            std::cout << "    Address: 0x" << std::hex << address
            << "  Thread: 0x" << reinterpret_cast<uintptr_t>(hThread)
            << "  DR: " << static_cast<int>(reg)
            << std::dec << "\n";
        }
        return RESTORE;
    }

} // namespace RoboDBG

