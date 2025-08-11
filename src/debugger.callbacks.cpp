
#include "debugger.h"

namespace RoboDBG {

// ==================================================
// HOOKS
// ==================================================
void Debugger::onStart( uintptr_t imageBase, uintptr_t entryPoint ) {
    if(!this->verbose) return;
    // Assume original entry point offset known (e.g., 0x1000)
    //DWORD_PTR entryOffset = 0x1000;

    //LPVOID entryPoint = (LPBYTE)imageBase + entryOffset;
    std::cout << "[*] Debugger::onStart" << std::endl;
    std::cout << "[*] Image base (ASLR-aware): 0x" << std::hex << imageBase << std::endl;
    std::cout << "[*] Entry point resolved to: 0x" << std::hex << entryPoint << std::endl;

    //BYTE original;
    //SIZE_T read;
    /*ReadProcessMemory(hProcessGlobal, entryPoint, &original, 1, &read);
     *   breakpoints[entryPoint] = original;
     *
     *   BYTE int3 = 0xCC;
     *   WriteProcessMemory(hProcessGlobal, entryPoint, &int3, 1, nullptr);
     *   FlushInstructionCache(hProcessGlobal, entryPoint, 1);*/
}

void Debugger::onEnd( DWORD exitCode, DWORD pid ) {
    if(!this->verbose) return;
    std::cout << "[*] Process exited\n";
    std::cout << "    PID       : " << pid << "\n";
    std::cout << "    Exit Code : " << std::hex << "0x" << exitCode << std::dec << " (" << exitCode << ")\n";
}


void Debugger::onAttach() {
    if(!this->verbose) return;
    std::cout << "[*] Debugger::onAttach: Attached" << std::endl;
    //SetBreakpoint(hProcessGlobal, (LPVOID)0x0087243E);
    //SetBreakpoint(hProcessGlobal, (LPVOID)0x008725DA);
}


BreakpointAction Debugger::onBreakpoint(uintptr_t address, HANDLE hThread) {
    std::cout << "[*] Breakpoint hit at: 0x" << std::hex << address << std::endl;
    return RESTORE;
}

bool Debugger::onDLLLoad(uintptr_t address, std::string dllName, uintptr_t entryPoint) {
    if(!this->verbose) return false;
    std::cout << "[*] DLL load at hit at: 0x" << std::hex << address << std::endl;
    return false;
}

void Debugger::onThreadCreate( HANDLE hThread, DWORD threadId, uintptr_t threadBase, uintptr_t startAddress) {
    if(!this->verbose) return;
    std::cout << "[*] Thread created. TID=" << threadId
              << " TEB=0x" << std::hex << threadBase
              << " Start Address=0x" << std::hex << startAddress << "\n";
}

void Debugger::onThreadExit( DWORD threadID ) {
    if(!this->verbose) return;
    std::cout << "[*] Debugger::onThreadExit()\n";
}

void Debugger::onDLLUnload(uintptr_t address, std::string dllName) {
    if(!this->verbose) return;
    std::cout << "[*] Debugger::onDLLUnload()\n";
}

void Debugger::onSinglestep(uintptr_t address, HANDLE hThread) {
    if(!this->verbose) return;
    std::cout << "[*] Debugger::onSinglestep(ctx, extra)\n";
}

void Debugger::onAccessViolation( uintptr_t address, uintptr_t faultingAddress, long accessType) {
    if(!this->verbose) return;
    std::cout << "[!] Access violation at address: " << faultingAddress << std::endl;
    // determine access type
    switch (accessType) {
        case 0: std::cout << "    -> Read violation\n"; break;
        case 1: std::cout << "    -> Write violation\n"; break;
        case 8: std::cout << "    -> Execute violation (NX fault)\n"; break;
        default: std::cout << "    -> Unknown access type: " << accessType << "\n"; break;
    }
}

void Debugger::onDebugString(std::string msg) {
    if(!this->verbose) return;
    std::cout << "[*] Debugger::onDebugString: " << msg << "\n";
}


void Debugger::onRIPError( const RIP_INFO& rip )
{
    if(!this->verbose) return;
    std::cout << "[*] Debugger::onSinglestep(ctx, extra)\n";
}

void Debugger::onUnknownException( uintptr_t addr, DWORD code )
{
    if(!this->verbose) return;
    std::cout << "[*] Unknown exception 0x" << std::hex << code
    << " at 0x" << addr << "\n";
}

void Debugger::onUnknownDebugEvent( DWORD code )
{
    if(!this->verbose) return;
    std::cout << "[*] Unknown debug event: " << code << "\n";
}

BreakpointAction Debugger::onHardwareBreakpoint( uintptr_t address, HANDLE hThread, DRReg reg ) {
    return RESTORE;
}

}
