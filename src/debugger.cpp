#include "debugger.h"

namespace RoboDBG {

Debugger::Debugger( ) {
    this->verbose = false;
}

Debugger::Debugger( bool verbose) {
    this->verbose = verbose;
}

bool Debugger::hideDebugger( ) {
    PROCESS_BASIC_INFORMATION pbi = {};
    ULONG retLen;

    NTSTATUS status = NtQueryInformationProcess(
        hProcessGlobal,
        ProcessBasicInformation,
        &pbi,
        sizeof(pbi),&retLen
    );

    if (status != 0) {
        // std::cerr << "[-] NtQueryInformationProcess failed\n";
        return false;
    }

    // PEB is at this address in remote process
    BYTE beingDebugged = 0;
    SIZE_T bytesWritten = 0;

    // BeingDebugged is at offset 0x2 of the PEB
    LPVOID pebDebugFlagAddr = (LPBYTE)pbi.PebBaseAddress + 2;

    // Write zero to the flag
    if (!WriteProcessMemory(hProcessGlobal, pebDebugFlagAddr, &beingDebugged, sizeof(beingDebugged), &bytesWritten)) {
        std::cerr << "[-] WriteProcessMemory failed: " << GetLastError() << "\n";
        return false;
    }

    return true;
}

uintptr_t Debugger::ASLR(LPVOID address) {
    return imageBase + reinterpret_cast<uintptr_t>(address);
}

uintptr_t Debugger::ASLR(uintptr_t address) {
    return imageBase + address;
}

// Attach to an existing process
int Debugger::attachToProcess(std::string exeName) {
    DWORD pid = Util::findProcessId(exeName);
    if (pid == 0) {
        std::cerr << "Process not found: " << exeName << std::endl;
        return -1;
    }
    std::cout << "Attach to " << pid << std::endl;
    if (!DebugActiveProcess(pid)) {
        std::cerr << "Failed to attach to process: " << GetLastError() << std::endl;
        return -1;
    }

    hProcessGlobal = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (hProcessGlobal == NULL) {
        std::cerr << "Failed to open process: " << GetLastError() << std::endl;
        return -1;
    }
    debuggedPid = pid;
    onAttach( );
    return 0;
}

int Debugger::detachFromProcess(  ) {
    // Detach the debugger
    if (!DebugActiveProcessStop(debuggedPid)) {
        std::cerr << "Failed to detach debugger. Error: " << GetLastError() << std::endl;
        return 1;
    }
    return 0;
}

// Start a new process under debug control
int Debugger::startProcess(std::string exeName) {
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    if (!CreateProcess(
        NULL,
        const_cast<LPSTR>(exeName.c_str()),
                       NULL,
                       NULL,
                       FALSE,
                       DEBUG_ONLY_THIS_PROCESS,
                       NULL,
                       NULL,
                       &si,
                       &pi)) {
        std::cerr << "CreateProcess failed: " << GetLastError() << std::endl;
        return -1;
                       }

    hProcessGlobal = pi.hProcess;
    hThreadGlobal  = pi.hThread;
    return 0;
}

int Debugger::startProcess(std::string exeName, const std::vector<std::string>& args) {
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi{};

    // Build the full command line
    std::string cmdLine = "\"" + exeName + "\""; // quote exe path in case it has spaces
    for (const auto& arg : args) {
        cmdLine += " " + arg;
    }

    // CreateProcess requires a modifiable LPSTR
    std::vector<char> cmdLineMutable(cmdLine.begin(), cmdLine.end());
    cmdLineMutable.push_back('\0');

    if (!CreateProcessA(
        nullptr,
        cmdLineMutable.data(),
                        nullptr,
                        nullptr,
                        FALSE,
                        DEBUG_ONLY_THIS_PROCESS,
                        nullptr,
                        nullptr,
                        &si,
                        &pi
    )) {
        std::cerr << "CreateProcess failed: " << GetLastError() << std::endl;
        return -1;
    }

    hProcessGlobal = pi.hProcess;
    hThreadGlobal  = pi.hThread;
    return 0;
}



void Debugger::enableSingleStep(HANDLE hThread) {
    CONTEXT ctx = {};
    ctx.ContextFlags = CONTEXT_CONTROL;

    if (GetThreadContext(hThread, &ctx)) {
        ctx.EFlags |= 0x100; // Trap Flag
        SetThreadContext(hThread, &ctx);
    }
}

void Debugger::decrementIP(HANDLE hThread) {
    if (SuspendThread(hThread) == (DWORD)-1) {
        std::cerr << "[-] SuspendThread failed: " << GetLastError() << "\n";
    }
    CONTEXT ctx = {};
    ctx.ContextFlags = CONTEXT_CONTROL;
    if (GetThreadContext(hThread, &ctx)) {
        #ifdef _WIN64
            ctx.Rip--;
        #else
            ctx.Eip--;
        #endif
        SetThreadContext(hThread, &ctx);
    }
    // Always try to resume
    if (ResumeThread(hThread) == (DWORD)-1) {
        std::cerr << "[-] ResumeThread failed: " << GetLastError() << "\n";
    }
}



void Debugger::printIP(HANDLE hThread) {
    CONTEXT ctx = {};
    ctx.ContextFlags = CONTEXT_CONTROL;
    if (GetThreadContext(hThread, &ctx)) {
        #ifdef _WIN64
        std::cout << "[!] RIP = 0x" << std::hex << ctx.Rip << std::endl;
        #else
        std::cout << "[!] EIP = 0x" << std::hex << ctx.Eip << std::endl;
        #endif
    }
}

//actualize a thread list. e.g. after attaching to an existing running application;
void Debugger::actualizeThreadList() {
    threads.clear(); // Clear existing thread list

    DWORD processId = GetProcessId(hProcessGlobal);
    if (processId == 0) {
        std::cerr << "Failed to get process ID from handle. Error: " << GetLastError() << std::endl;
        return;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create thread snapshot. Error: " << GetLastError() << std::endl;
        return;
    }

    THREADENTRY32 te32 = {};
    te32.dwSize = sizeof(te32);

    if (Thread32First(snapshot, &te32)) {
        do {
            if (te32.th32OwnerProcessID == processId) {
                HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te32.th32ThreadID);
                if (!hThread) {
                    std::cerr << "Failed to open thread " << te32.th32ThreadID << ". Error: " << GetLastError() << std::endl;
                    continue;
                }

                // Optionally get thread start address (Windows internal APIs, see note below)
                // For now we fill with nullptr placeholders
                thread_t t;
                t.hThread = hThread;
                t.threadId = te32.th32ThreadID;
                t.threadBase = nullptr;     // Not easily available without NtQueryInformationThread
                t.startAddress = nullptr;   // Optional: can use NtQueryInformationThread with ThreadQuerySetWin32StartAddress

                threads.push_back(t);
            }
        } while (Thread32Next(snapshot, &te32));
    }

    CloseHandle(snapshot);
}

int Debugger::loop() {
    bool stepping = false;

    DEBUG_EVENT dbgEvent;
    while (true) {
        if (!WaitForDebugEvent(&dbgEvent, INFINITE)) break;

        DWORD cont = DBG_CONTINUE;

        switch (dbgEvent.dwDebugEventCode) {
            case CREATE_PROCESS_DEBUG_EVENT: {
                imageBase = reinterpret_cast<uintptr_t>(dbgEvent.u.CreateProcessInfo.lpBaseOfImage);
                DWORD_PTR entryPoint = Util::getEntryPoint(hProcessGlobal, reinterpret_cast<LPVOID>(imageBase));
                onStart(imageBase,static_cast<uintptr_t>(entryPoint));
                break;
            }

            case EXIT_PROCESS_DEBUG_EVENT: {
                DWORD exitCode = dbgEvent.u.ExitProcess.dwExitCode;
                DWORD pid = dbgEvent.dwProcessId;
                onEnd(exitCode, pid);
                return 0;
            }
            case CREATE_THREAD_DEBUG_EVENT: {
                //std::cout << "[*] Thread created. TID=" << dbgEvent.dwThreadId
                //<< " TEB=0x" << std::hex << (DWORD_PTR)threadBase
                // << " Start Address=0x" << std::hex << (DWORD_PTR)threadStartAddr << "\n";

                LPVOID threadBase = dbgEvent.u.CreateThread.lpThreadLocalBase;
                LPVOID threadStartAddr = dbgEvent.u.CreateThread.lpStartAddress;

                HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, dbgEvent.dwThreadId);
                if (!hThread) {
                    std::cerr << "[-] Failed to open thread: " << GetLastError() << "\n";
                    break;
                }
                onThreadCreate( hThread, dbgEvent.dwThreadId, reinterpret_cast<uintptr_t>(threadBase), reinterpret_cast<uintptr_t>(threadStartAddr));

                // Store the thread info
                thread_t newThread = {
                    .hThread = hThread,
                    .threadId = dbgEvent.dwThreadId,
                    .threadBase = threadBase,
                    .startAddress = threadStartAddr
                };
                threads.push_back(newThread);

                break;
            }

            case EXIT_THREAD_DEBUG_EVENT: {
                //std::cout << "[*] Thread exited. TID=" << dbgEvent.dwThreadId << "\n";
                onThreadExit( dbgEvent.dwThreadId );
                break;
            }

            case LOAD_DLL_DEBUG_EVENT: {
                LPVOID base = dbgEvent.u.LoadDll.lpBaseOfDll;
                auto name = Util::getDllName(hProcessGlobal, dbgEvent.u.LoadDll.lpImageName, dbgEvent.u.LoadDll.fUnicode);
                DWORD_PTR entryPoint = Util::getEntryPoint(hProcessGlobal, base);
                onDLLLoad(reinterpret_cast<uintptr_t>(base), name, static_cast<uintptr_t>(entryPoint));

                //std::cout << "[*] DLL loaded at 0x" << std::hex << (DWORD_PTR)base
                //<< " Name: " << name << "\n";
                break;
            }

            case UNLOAD_DLL_DEBUG_EVENT: {
                LPVOID base = dbgEvent.u.UnloadDll.lpBaseOfDll;
                auto name = Util::getDllName(hProcessGlobal, dbgEvent.u.LoadDll.lpImageName, dbgEvent.u.LoadDll.fUnicode);
                onDLLUnload(reinterpret_cast<uintptr_t>(base), name);
                //std::cout << "[*] DLL unloaded from 0x" << std::hex << (DWORD_PTR)base << "\n";
                break;
            }

            case EXCEPTION_DEBUG_EVENT: {
                DWORD code = dbgEvent.u.Exception.ExceptionRecord.ExceptionCode;
                LPVOID addr = dbgEvent.u.Exception.ExceptionRecord.ExceptionAddress;
                HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, dbgEvent.dwThreadId);
                if (!hThread) break;

                if (code == EXCEPTION_BREAKPOINT) {
                    if(this->verbose) std::cout << "[~] Breakpoint Hit" << std::endl;
                    auto it = breakpoints.find(addr);
                    if (it != breakpoints.end()) {
                        //std::cout << "[*] Loop Breakpoint hit at: 0x" << std::hex << (DWORD_PTR)addr << "\n";
                        restoreBreakpoint(addr);
                        decrementIP(hThread);
                        BreakpointAction bpType = onBreakpoint(reinterpret_cast<uintptr_t>(addr),hThread);
                        if (bpType != BREAK ) {
                            enableSingleStep(hThread);
                            stepping = true;
                            if(bpType == RESTORE )
                            {
                                bpAddrToRestore = addr;
                                lastBpType = bpType;
                                needToRestoreBreakpoint = true;
                            }
                        } else { //BP_DO_NOP
                            stepping = false;
                        }
                    }
                } else if (code == EXCEPTION_SINGLE_STEP && stepping) {
                    if(needToRestoreBreakpoint) //just hit a breakpoint. Now restore.
                    {
                        if(restoreHwBP)
                        {
                            if(this->verbose) std::cout << "[~] Restoring hardware breakpoint" << std::endl;
                            setHardwareBreakpoint(hwBPToRestore);
                            restoreHwBP = false;
                        } else {
                            setBreakpoint(bpAddrToRestore); //TODO
                        }
                        needToRestoreBreakpoint = false;

                    }

                    if(lastBpType == SINGLE_STEP ) {
                        BreakpointAction bpType = onBreakpoint(reinterpret_cast<uintptr_t>(addr),hThread);
                        if (bpType != BREAK) {
                            bpAddrToRestore = addr;
                            needToRestoreBreakpoint = true;

                            lastBpType = bpType;
                            enableSingleStep(hThread);
                            stepping = true;
                        }
                    } else {
                        stepping = false;
                    }
                } else if (code == EXCEPTION_SINGLE_STEP ) {
                    if(this->verbose) std::cout << "[~] Single step: 0x" << std::hex << (DWORD_PTR)addr << "\n";
                    DRReg reg = isHardwareBreakpointAt( addr );
                    if(reg != DRReg::NOP) {
                        BreakpointAction bpType = onHardwareBreakpoint(reinterpret_cast<uintptr_t>(addr), hThread, reg);
                        if(bpType == BREAK) { //Software Breakpoint
                            clearHardwareBreakpoint( reg );
                        } else if(bpType == SINGLE_STEP)
                        {
                            enableSingleStep(hThread);
                            stepping = true;
                            clearHardwareBreakpoint(reg);
                        } else if(bpType == RESTORE)
                        {
                            hwBp_t bp = getBreakpointByReg( reg );
                            needToRestoreBreakpoint = true;
                            restoreHwBP = true;
                            hwBPToRestore = bp;
                            bpAddrToRestore = addr;
                            hwBPThreadToRestore = hThread;
                            lastBpType = bpType;

                            //getHardwareBreakpoints();
                            clearHardwareBreakpointOnThread(hwBPThreadToRestore, reg);
                            enableSingleStep(hThread);
                            stepping = true;
                        }
                    } else {
                        onUnknownException(reinterpret_cast<uintptr_t>(addr), code);

                    }
                } else if ( code == EXCEPTION_ACCESS_VIOLATION ) {
                    ULONG_PTR rawAddr = dbgEvent.u.Exception.ExceptionRecord.ExceptionInformation[1];
                    LPVOID faultingAddress = reinterpret_cast<LPVOID>(rawAddr);
                    ULONG_PTR accessType = dbgEvent.u.Exception.ExceptionRecord.ExceptionInformation[0];
                    //MemoryRegion_t mr = getPageByAddress( faultingAddress);
                    //changeMemoryProtection(mr, PAGE_EXECUTE_READWRITE);
                    onAccessViolation( static_cast<uintptr_t>(rawAddr), reinterpret_cast<uintptr_t>(faultingAddress), static_cast<long>(accessType));
                    //enableSingleStep(hThread);
                    //stepping = true;

                } else {
                    onUnknownException( reinterpret_cast<uintptr_t>(addr), code );
                }

                CloseHandle(hThread);
                break;
            }

                        case OUTPUT_DEBUG_STRING_EVENT: {
                            OUTPUT_DEBUG_STRING_INFO& info = dbgEvent.u.DebugString;
                            SIZE_T bytesRead;

                            if (!info.fUnicode) {
                                // ANSI string
                                char buffer[1024] = {0};
                                ReadProcessMemory(
                                    hProcessGlobal,
                                    info.lpDebugStringData,
                                    buffer,
                                    std::min<DWORD>(info.nDebugStringLength, sizeof(buffer) - 1),
                                                  &bytesRead
                                );

                                std::string msg(buffer);
                                onDebugString(msg);
                            } else {
                                // Unicode string
                                wchar_t wbuffer[1024] = {0};
                                ReadProcessMemory(
                                    hProcessGlobal,
                                    info.lpDebugStringData,
                                    wbuffer,
                                    std::min<DWORD>(info.nDebugStringLength * sizeof(wchar_t), sizeof(wbuffer) - sizeof(wchar_t)),
                                                  &bytesRead
                                );

                                // Convert wchar_t* to std::string using WideCharToMultiByte (UTF-8)
                                int size_needed = WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, nullptr, 0, nullptr, nullptr);
                                std::string msg(size_needed, 0);
                                WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, &msg[0], size_needed, nullptr, nullptr);

                                onDebugString(msg);
                            }

                            break;
                        }

            case RIP_EVENT: {
                const RIP_INFO& rip = dbgEvent.u.RipInfo;
                onRIPError( rip );
                break;
            }

            default: {
                onUnknownDebugEvent( dbgEvent.dwDebugEventCode );
                break;
            }
        }
        ContinueDebugEvent(dbgEvent.dwProcessId, dbgEvent.dwThreadId, cont);
    }

    CloseHandle(hProcessGlobal);
    CloseHandle(hThreadGlobal);
    return 0;
}

}
