#pragma once
#include <robodbg/debugger.h>
#include <robodbg/util.h>
#include <iostream>

using namespace RoboDBG;

class MyDebugger : public Debugger {
public:
    MyDebugger() = default;

    // Called when the process starts
    virtual void onStart(uintptr_t imageBase, uintptr_t entryPoint) override {
        std::cout << "[onStart] imageBase: " << std::hex << imageBase
        << ", entryPoint: " << entryPoint << std::endl;
    }

    // Called when the process exits
    virtual void onEnd(DWORD exitCode, DWORD pid) override {
        std::cout << "[onEnd] exitCode: " << std::dec << exitCode
        << ", pid: " << pid << std::endl;
    }

    // Called when attach succeeds
    virtual void onAttach() override {
        std::cout << "[onAttach] Successfully attached to process." << std::endl;
    }

    // New thread created
    virtual void onThreadCreate(HANDLE hThread, DWORD threadId, uintptr_t threadBase, uintptr_t startAddress) override {
        std::cout << "[onThreadCreate] TID: " << threadId << ", base: " << std::hex << threadBase
        << ", start: " << startAddress << std::endl;
    }

    // Thread exited
    virtual void onThreadExit(DWORD threadID) override {
        std::cout << "[onThreadExit] TID: " << threadID << std::endl;
    }

    // DLL loaded
    virtual bool onDLLLoad(uintptr_t address, std::string dllName, uintptr_t entryPoint) override {
        std::cout << "[onDLLLoad] " << dllName << " at " << std::hex << address
        << ", entryPoint: " << entryPoint << std::endl;
        return true;
    }

    // DLL unloaded
    virtual void onDLLUnload(uintptr_t address, std::string dllName) override {
        std::cout << "[onDLLUnload] " << dllName << " from " << std::hex << address << std::endl;
    }

    // Software breakpoint hit
    virtual BreakpointAction onBreakpoint(uintptr_t address, HANDLE hThread) override {
        std::cout << "[onBreakpoint] BP at " << std::hex << address << std::endl;
        return BreakpointAction::BREAK;
    }

    // Hardware breakpoint hit
    virtual BreakpointAction onHardwareBreakpoint(uintptr_t address, HANDLE hThread, DRReg reg) override {
        std::cout << "[onHardwareBreakpoint] DR" << static_cast<int>(reg)
                  << " hit at " << std::hex << address << std::endl;
        return BreakpointAction::BREAK;
    }

    // Single-step trap
    virtual void onSinglestep(uintptr_t address, HANDLE hThread) override {
        std::cout << "[onSinglestep] " << std::hex << address << std::endl;
    }

    // Debug strings (OutputDebugString)
    virtual void onDebugString(std::string dbgString) override {
        std::cout << "[onDebugString] " << dbgString << std::endl;
    }

    // Access violation handler
    virtual void onAccessViolation(uintptr_t address, uintptr_t faultingAddress, long accessType) override {
        std::cout << "[onAccessViolation] " << std::hex << address << " -> "
                  << faultingAddress << ", accessType: " << std::dec << accessType << std::endl;
    }

    // RIP errors (x64 structured exception)
    virtual void onRIPError(const RIP_INFO& rip) override {
        std::cout << "[onRIPError] RIP code: " << std::hex << rip.ErrorCode << std::endl;
    }

    // Unknown exceptions
    virtual void onUnknownException(uintptr_t addr, DWORD code) override {
        std::cout << "[onUnknownException] At: " << std::hex << addr << ", code: " << std::hex << code << std::endl;
    }

    // Unknown debug events
    virtual void onUnknownDebugEvent(DWORD code) override {
        std::cout << "[onUnknownDebugEvent] Code: " << std::hex << code << std::endl;
    }
};
