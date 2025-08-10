/**
 * @file debugger.h
 * @brief Main Debugger file
 * @author Milkshake
 */

#ifndef DEBUGGER_H
#define DEBUGGER_H

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "ntdll.lib")

#include <windows.h>
#include <string>
#include <vector>
#include <tlhelp32.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <tchar.h>
#include <stdio.h>
#include <psapi.h>
#include <map>
#include <fstream>
#include <winternl.h>
#include <intrin.h>

#include "util.h"

namespace RoboDBG {

    /**
     * @enum BreakpointAction
     * @brief Specifies the action to take when a breakpoint is hit.
     */
    enum BreakpointAction {
        BREAK,       ///< Stop execution at the breakpoint.
        RESTORE,     ///< Restore the original instruction at the breakpoint.
        SINGLE_STEP  ///< Perform a single-step execution after hitting the breakpoint.
    };

    /**
     * @enum AccessType
     * @brief Specifies the type of memory access that triggers a hardware breakpoint.
     */
    enum class AccessType {
        EXECUTE,   ///< Trigger when executing instructions at the address.
        WRITE,     ///< Trigger when writing to the address.
        READWRITE  ///< Trigger when reading from or writing to the address.
    };

    /**
     * @enum DRReg
     * @brief Hardware debug registers used for breakpoints.
     */
    enum class DRReg {
        NOP = -1, ///< No register assigned.
        DR0 = 0,  ///< Debug register 0.
        DR1 = 1,  ///< Debug register 1.
        DR2 = 2,  ///< Debug register 2.
        DR3 = 3   ///< Debug register 3.
    };

    /**
     * @enum BreakpointLength
     * @brief Length of the hardware breakpoint watch.
     */
    enum class BreakpointLength {
        BYTE  = 0, ///< 1 byte.
        WORD  = 1, ///< 2 bytes.
        DWORD = 2, ///< 4 bytes.
        QWORD = 3  ///< 8 bytes.
    };

    /**
     * @struct thread_t
     * @brief Represents a thread in a debugged process.
     */
    struct thread_t {
        HANDLE hThread;      ///< Thread handle.
        DWORD threadId;      ///< Thread ID.
        LPVOID threadBase;   ///< Base address of the thread.
        LPVOID startAddress; ///< Start address of the thread.
    };

    /**
     * @struct hwBp_t
     * @brief Hardware breakpoint configuration.
     */
    struct hwBp_t {
        HANDLE          hThread; ///< Target thread handle.
        LPVOID          address; ///< Address to watch.
        DRReg           reg;     ///< Debug register used.
        AccessType      type;    ///< Type of access that triggers the breakpoint.
        BreakpointLength len;    ///< Length of the watch region.
    };

    /**
     * @struct MemoryRegion_t
     * @brief Represents a memory region in a process.
     */
    struct MemoryRegion_t {
        LPVOID BaseAddress; ///< Base address of the memory region.
        SIZE_T RegionSize;  ///< Size of the memory region in bytes.
        DWORD State;        ///< State (MEM_COMMIT, MEM_FREE, MEM_RESERVE).
        DWORD Protect;      ///< Protection flags (e.g., PAGE_READWRITE).
        DWORD Type;         ///< Type (MEM_IMAGE, MEM_MAPPED, MEM_PRIVATE).
    };

    #ifdef _WIN64
    /**
     * @enum Flags64
     * @brief x86-64 CPU status flags.
     */
    enum class Flags64 : DWORD64 {
        CF = 1ull << 0,  ///< Carry Flag.
        PF = 1ull << 2,  ///< Parity Flag.
        AF = 1ull << 4,  ///< Auxiliary Carry Flag.
        ZF = 1ull << 6,  ///< Zero Flag.
        SF = 1ull << 7,  ///< Sign Flag.
        TF = 1ull << 8,  ///< Trap Flag.
        IF = 1ull << 9,  ///< Interrupt Enable Flag.
        DF = 1ull << 10, ///< Direction Flag.
        OF = 1ull << 11  ///< Overflow Flag.
    };
    #else
    /**
     * @enum Flags32
     * @brief x86 CPU status flags.
     */
    enum class Flags32 : DWORD {
        CF = 1 << 0,   ///< Carry Flag.
        PF = 1 << 2,   ///< Parity Flag.
        AF = 1 << 4,   ///< Auxiliary Carry Flag.
        ZF = 1 << 6,   ///< Zero Flag.
        SF = 1 << 7,   ///< Sign Flag.
        TF = 1 << 8,   ///< Trap Flag.
        IF = 1 << 9,   ///< Interrupt Enable Flag.
        DF = 1 << 10,  ///< Direction Flag.
        OF = 1 << 11   ///< Overflow Flag.
    };
    #endif

    #ifdef _WIN64
    /**
     * @enum Register64
     * @brief 64-bit x86-64 general-purpose registers.
     */
    enum class Register64 {
        RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP,
        R8, R9, R10, R11, R12, R13, R14, R15,
        RIP ///< Instruction Pointer.
    };
    #else
    /**
     * @enum Register32
     * @brief 32-bit x86 general-purpose registers.
     */
    enum class Register32 {
        EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP,
        EIP ///< Instruction Pointer.
    };
    #endif


/**
 * @class Debugger
 * @brief Main Debugger class
 *
 * Bla
 */
class Debugger {
private:
    LPVOID bpAddrToRestore;
    bool needToRestoreBreakpoint; // TODO: rename to bpNeedToRestore
    int lastBpType;

    bool restoreHwBP;
    hwBp_t hwBPToRestore;
    HANDLE hwBPThreadToRestore;

    bool dbgLoop = true;

    // internal callbacks
    void onPreStart();
    void onPreAttach();

protected:
    bool verbose;
    uintptr_t baseAddressOffset = 0; // TODO: Add a base image address, e.g: 0x00400000U

#ifdef __WIN64
    uintptr_t imageBase = 0x0000000140000000ULL; ///< Typical image base (with ASLR).
#else
    uintptr_t imageBase = 0x00400000U;           ///< Typical image base (with ASLR).
#endif
    std::map<LPVOID, BYTE> breakpoints;
    std::map<LPVOID, hwBp_t> hwBreakpoints;
    std::map<LPVOID, BYTE> dlls;
    std::vector<thread_t> threads;
    int debuggedPid;
    HANDLE hProcessGlobal = nullptr;
    HANDLE hThreadGlobal = nullptr;

    // ===== Lifecycle & events =====

    /**
     * @brief Called when a new debuggee process is started.
     * @param imageBase Base address of the main image.
     * @param entryPoint Entry point address of the process.
     */
    virtual void onStart(uintptr_t imageBase, uintptr_t entryPoint);

    /**
     * @brief Called when the debuggee exits.
     * @param exitCode Process exit code.
     * @param pid Process ID.
     */
    virtual void onEnd(DWORD exitCode, DWORD pid);

    /**
     * @brief Called after successfully attaching to an already running process.
     */
    virtual void onAttach();

    /**
     * @brief Called when a thread is created in the debuggee.
     * @param hThread Thread handle.
     * @param threadId Thread ID.
     * @param threadBase Thread base address.
     * @param startAddress Thread start address (initial IP).
     */
    virtual void onThreadCreate(HANDLE hThread, DWORD threadId, uintptr_t threadBase, uintptr_t startAddress);

    /**
     * @brief Called when a thread exits.
     * @param threadID Exiting thread ID.
     */
    virtual void onThreadExit(DWORD threadID);

    /**
     * @brief Called when a DLL is loaded.
     * @param address Base address of the module.
     * @param dllName File name of the DLL.
     * @param entryPoint Module entry point.
     * @return true to continue; false to break into the debugger.
     */
    virtual bool onDLLLoad(uintptr_t address, std::string dllName, uintptr_t entryPoint);

    /**
     * @brief Called when a DLL is unloaded.
     * @param address Base address prior to unload.
     * @param dllName File name of the DLL.
     */
    virtual void onDLLUnload(uintptr_t address, std::string dllName);

    /**
     * @brief Called on software breakpoint (INT3).
     * @param address Address of the breakpoint.
     * @param hThread Current thread handle.
     * @return Action to take (break, restore, single-step).
     */
    virtual BreakpointAction onBreakpoint(uintptr_t address, HANDLE hThread);

    /**
     * @brief Called on hardware breakpoint hit.
     * @param address Address being watched.
     * @param hThread Current thread handle.
     * @param reg Debug register that fired.
     * @return Action to take (break, restore, single-step).
     */
    virtual BreakpointAction onHardwareBreakpoint(uintptr_t address, HANDLE hThread, DRReg reg);

    /**
     * @brief Called on single-step exception.
     * @param address Current instruction pointer.
     * @param hThread Current thread handle.
     */
    virtual void onSinglestep(uintptr_t address, HANDLE hThread);

    /**
     * @brief Called when OutputDebugString is emitted by the debuggee.
     * @param dbgString The debug string payload.
     */
    virtual void onDebugString(std::string dbgString);

    /**
     * @brief Called on access violation (AV).
     * @param address Faulting instruction address.
     * @param faultingAddress Memory address being accessed.
     * @param accessType Access type (read/write/execute).
     */
    virtual void onAccessViolation(uintptr_t address, uintptr_t faultingAddress, long accessType);

    /**
     * @brief Called on RIP error (native debug port issues).
     * @param rip RIP_INFO structure with details.
     */
    virtual void onRIPError(const RIP_INFO& rip);

    /**
     * @brief Called on unknown exception.
     * @param addr Faulting address.
     * @param code Exception code.
     */
    virtual void onUnknownException(uintptr_t addr, DWORD code);

    /**
     * @brief Called on unhandled/unknown debug events.
     * @param code Event code.
     */
    virtual void onUnknownDebugEvent(DWORD code);

    // ===== Threading =====

    /**
     * @brief Refreshes the internal thread list by querying the target process.
     */
    void actualizeThreadList();

    // ===== Breakpoints (low-level) =====

    /**
     * @brief Sets a software INT3 breakpoint at an address.
     * @param address Target address in the debuggee.
     */
    void setBreakpoint(LPVOID address);

    /**
     * @brief Checks if a hardware breakpoint exists at an address.
     * @param address Address to probe.
     * @return DR register if present; DRReg::NOP otherwise.
     */
    DRReg isHardwareBreakpointAt(LPVOID address);

    /**
     * @brief Restores the original byte at a software breakpoint address.
     * @param address Breakpoint address to restore.
     */
    void restoreBreakpoint(LPVOID address);

    /**
     * @brief Sets a hardware breakpoint for a specific thread.
     * @param bp Hardware breakpoint configuration (per-thread).
     * @return true on success; false otherwise.
     */
    bool setHardwareBreakpointOnThread(hwBp_t bp);

    /**
     * @brief Sets a hardware breakpoint for all existing (and future) threads where applicable.
     * @param bp Hardware breakpoint configuration (process-wide intent).
     * @return true on success; false otherwise.
     */
    bool setHardwareBreakpoint(hwBp_t bp);

    /**
     * @brief Enumerates current hardware breakpoints.
     * @return Vector of active hardware breakpoints.
     */
    std::vector<hwBp_t> getHardwareBreakpoints();

    /**
     * @brief Enables trap flag (single-step) for a thread.
     * @param hThread Thread handle.
     */
    void enableSingleStep(HANDLE hThread);

    /**
     * @brief Moves the instruction pointer one instruction backward (post-breakpoint fixup).
     * @param hThread Thread handle.
     */
    void decrementIP(HANDLE hThread);

    /**
     * @brief Clears a DRx slot across threads.
     * @param reg Register to clear (DR0–DR3).
     * @return true on success; false otherwise.
     */
    bool clearHardwareBreakpoint(DRReg reg);

    /**
     * @brief Clears a DRx slot on a single thread.
     * @param hThread Thread handle.
     * @param reg Register to clear (DR0–DR3).
     * @return true on success; false otherwise.
     */
    bool clearHardwareBreakpointOnThread(HANDLE hThread, DRReg reg);

    /**
     * @brief Gets the breakpoint definition bound to a DRx register.
     * @param reg Register (DR0–DR3).
     * @return Hardware breakpoint record.
     */
    hwBp_t getBreakpointByReg(DRReg reg);

    // ===== Memory =====

    /**
     * @brief Writes raw bytes to target memory.
     * @param address Destination address in target.
     * @param buffer Source buffer.
     * @param size Number of bytes to write.
     * @return true on success; false otherwise.
     */
    bool writeMemory(LPVOID address, const void* buffer, SIZE_T size);

    /**
     * @brief Reads raw bytes from target memory.
     * @param address Source address in target.
     * @param buffer Destination buffer.
     * @param size Number of bytes to read.
     * @return true on success; false otherwise.
     */
    bool readMemory(LPVOID address, void* buffer, SIZE_T size);

    /**
     * @brief Changes memory protection on a region.
     * @param baseAddress Region base.
     * @param regionSize Region length in bytes.
     * @param newProtect PAGE_* flags.
     * @return true on success; false otherwise.
     */
    bool changeMemoryProtection(LPVOID baseAddress, SIZE_T regionSize, DWORD newProtect);

    /**
     * @brief Gets information for the page containing an address.
     * @param baseAddress Address inside the page.
     * @return Memory region descriptor.
     */
    MemoryRegion_t getPageByAddress(LPVOID baseAddress);

    /**
     * @brief Enumerates readable/committed pages of the process.
     * @return Vector of memory regions.
     */
    std::vector<MemoryRegion_t> getMemoryPages();

    /**
     * @brief Changes protection for a specific page descriptor.
     * @param page Page descriptor from getMemoryPages().
     * @param newProtect PAGE_* flags.
     * @return true on success; false otherwise.
     */
    bool changeMemoryProtection(MemoryRegion_t page, DWORD newProtect);

    /**
     * @brief Scans process memory for a byte pattern.
     * @param pattern Byte sequence to match.
     * @return Vector of matched start addresses.
     */
    std::vector<uintptr_t> searchInMemory(const std::vector<BYTE>& pattern);

    // ===== Misc =====

    /**
     * @brief Applies the module ASLR slide to an LPVOID.
     * @param address Unslid address.
     * @return Slid (runtime) address.
     */
    uintptr_t ASLR(LPVOID address);

    /**
     * @brief Applies the module ASLR slide to a uintptr_t.
     * @param address Unslid address.
     * @return Slid (runtime) address.
     */
    uintptr_t ASLR(uintptr_t address);

    /**
     * @brief Attempts to hide the debugger from basic anti-debug checks.
     * @return true if hiding steps were applied; false otherwise.
     */
    bool hideDebugger();

    /**
     * @brief Prints the current instruction pointer (IP/EIP/RIP) of a thread.
     * @param hThread Thread handle.
     */
    void printIP(HANDLE hThread);

#ifdef _WIN64
    /**
     * @brief Reads a status flag from RFLAGS.
     * @param hThread Thread handle.
     * @param flag Flag to test.
     * @return true if set; false otherwise.
     */
    bool getFlag(HANDLE hThread, Flags64 flag);

    /**
     * @brief Sets or clears a status flag in RFLAGS.
     * @param hThread Thread handle.
     * @param flag Flag to mutate.
     * @param enabled true to set; false to clear.
     */
    void setFlag(HANDLE hThread, Flags64 flag, bool enabled);

    /**
     * @brief Reads a 64-bit general-purpose register.
     * @param hThread Thread handle.
     * @param reg Register selector.
     * @return Register value.
     */
    int64_t getRegister(HANDLE hThread, Register64 reg);

    /**
     * @brief Writes a 64-bit general-purpose register.
     * @param hThread Thread handle.
     * @param reg Register selector.
     * @param value New value.
     */
    void setRegister(HANDLE hThread, Register64 reg, int64_t value);
#else
    /**
     * @brief Reads a status flag from EFLAGS.
     * @param hThread Thread handle.
     * @param flag Flag to test.
     * @return true if set; false otherwise.
     */
    bool getFlag(HANDLE hThread, Flags32 flag);

    /**
     * @brief Sets or clears a status flag in EFLAGS.
     * @param hThread Thread handle.
     * @param flag Flag to mutate.
     * @param enabled true to set; false to clear.
     */
    void setFlag(HANDLE hThread, Flags32 flag, bool enabled);

    /**
     * @brief Reads a 32-bit general-purpose register.
     * @param hThread Thread handle.
     * @param reg Register selector.
     * @return Register value.
     */
    int32_t getRegister(HANDLE hThread, Register32 reg);

    /**
     * @brief Writes a 32-bit general-purpose register.
     * @param hThread Thread handle.
     * @param reg Register selector.
     * @param value New value.
     */
    void setRegister(HANDLE hThread, Register32 reg, int32_t value);
#endif

    /**
     * @brief Prints a formatted list of memory pages (debug helper).
     */
    void PrintMemoryPages();

    // ===== Wrappers (inline, app-friendly) =====

    /**
     * @brief Sets a software INT3 breakpoint.
     * @param address Target runtime address.
     */
    inline void setBreakpoint(uintptr_t address) {
        setBreakpoint(reinterpret_cast<LPVOID>(address));
    }

    /**
     * @brief Checks if a hardware breakpoint exists at an address.
     * @param address Address to probe.
     * @return DR register if present; DRReg::NOP otherwise.
     */
    inline DRReg isHardwareBreakpointAt(uintptr_t address) {
        return isHardwareBreakpointAt(reinterpret_cast<LPVOID>(address));
    }

    /**
     * @brief Gets page information for an address.
     * @param baseAddress Address inside the page.
     * @return Memory region descriptor.
     */
    inline MemoryRegion_t getPageByAddress(uintptr_t baseAddress) {
        return getPageByAddress(reinterpret_cast<LPVOID>(baseAddress));
    }

    /**
     * @brief Restores the original byte at a software breakpoint.
     * @param address Breakpoint address.
     */
    inline void restoreBreakpoint(uintptr_t address) {
        restoreBreakpoint(reinterpret_cast<LPVOID>(address));
    }

    /**
     * @brief Sets a hardware breakpoint on a specific thread.
     * @param hThread Target thread.
     * @param address Watched address.
     * @param reg DR slot (DR0–DR3).
     * @param type Access type.
     * @param len Operand length.
     * @return true on success; false otherwise.
     */
    inline bool setHardwareBreakpointOnThread(HANDLE hThread, LPVOID address, DRReg reg, AccessType type, BreakpointLength len) {
        hwBp_t bp = { hThread, address, reg, type, len };
        return setHardwareBreakpointOnThread(bp);
    }

    /**
     * @brief Sets a hardware breakpoint process-wide (current/future threads as applicable).
     * @param address Watched address.
     * @param reg DR slot (DR0–DR3).
     * @param type Access type.
     * @param len Operand length.
     * @return true on success; false otherwise.
     */
    inline bool setHardwareBreakpoint(LPVOID address, DRReg reg, AccessType type, BreakpointLength len) {
        hwBp_t bp = { (HANDLE)nullptr, address, reg, type, len };
        return setHardwareBreakpoint(bp);
    }

    /**
     * @brief Sets a hardware breakpoint using a 32-bit int address (for convenience).
     * @param address Watched address (32-bit).
     * @param reg DR slot (DR0–DR3).
     * @param type Access type.
     * @param len Operand length.
     * @return true on success; false otherwise.
     */
    inline bool setHardwareBreakpoint(int address, DRReg reg, AccessType type, BreakpointLength len) {
        hwBp_t bp = { (HANDLE)nullptr, reinterpret_cast<LPVOID>(static_cast<intptr_t>(address)), reg, type, len };
        return setHardwareBreakpoint(bp);
    }

    /**
     * @brief Changes memory protection on a region (uintptr_t overload).
     * @param baseAddress Region base.
     * @param regionSize Length in bytes.
     * @param newProtect PAGE_* flags.
     * @return true on success; false otherwise.
     */
    inline bool changeMemoryProtection(uintptr_t baseAddress, SIZE_T regionSize, DWORD newProtect) {
        return changeMemoryProtection(reinterpret_cast<LPVOID>(baseAddress), regionSize, newProtect);
    }

    /**
     * @brief Writes raw bytes to target memory (uintptr_t overload).
     * @param address Destination runtime address.
     * @param buffer Source buffer.
     * @param size Number of bytes to write.
     * @return true on success; false otherwise.
     */
    inline bool writeMemory(uintptr_t address, const void* buffer, SIZE_T size) {
        return writeMemory(reinterpret_cast<LPVOID>(address), buffer, size);
    }

    /**
     * @brief Reads raw bytes from target memory (uintptr_t overload).
     * @param address Source runtime address.
     * @param buffer Destination buffer.
     * @param size Number of bytes to read.
     * @return true on success; false otherwise.
     */
    inline bool readMemory(uintptr_t address, void* buffer, SIZE_T size) {
        return readMemory(reinterpret_cast<LPVOID>(address), buffer, size);
    }

    /**
     * @brief Writes a POD value to target memory (typed helper).
     * @tparam T Trivially copyable type.
     * @param address Destination runtime address.
     * @param value Value to write.
     * @return true on success; false otherwise.
     */
    template<typename T>
    bool writeMemory(uintptr_t address, const T& value) {
        SIZE_T bytesWritten = 0;
        if (!WriteProcessMemory(hProcessGlobal, reinterpret_cast<LPVOID>(address), &value, sizeof(T), &bytesWritten) || bytesWritten != sizeof(T)) {
            return false;
        }
        FlushInstructionCache(hProcessGlobal, reinterpret_cast<LPVOID>(address), sizeof(T));
        return true;
    }

    /**
     * @brief Reads a POD value from target memory (typed helper).
     * @tparam T Trivially copyable type.
     * @param address Source runtime address.
     * @return Read value; default-initialized T on failure.
     */
    template<typename T>
    T readMemory(uintptr_t address) {
        T value{};
        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(hProcessGlobal, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), &bytesRead) || bytesRead != sizeof(T)) {
            return T{};
        }
        return value;
    }

    /**
     * @brief Compares an LPVOID to a uintptr_t for equality.
     * @param a Left operand (LPVOID).
     * @param b Right operand (uintptr_t).
     * @return true if equal; false otherwise.
     */
    inline bool isEqual(LPVOID a, uintptr_t b) {
        return a == reinterpret_cast<LPCVOID>(b);
    }

    /**
     * @brief Returns HANDLE of debugged process
     * @return HANDLE
     */
    inline HANDLE getProcessHandle( )
    {
        return hProcessGlobal;
    }
public:
    /**
     * @brief Constructs a Debugger with default settings.
     */
    Debugger();

    /**
     * @brief Constructs a Debugger with verbosity control.
     * @param verbose Enable verbose logging if true.
     */
    Debugger(bool verbose);

    /**
     * @brief Starts a process under debugging.
     * @param exeName Path to the executable.
     * @return 0 on success; non-zero on failure.
     */
    int startProcess(std::string exeName);

    /**
     * @brief Attaches to a running process by name.
     * @param exeName Process image name (e.g., "notepad.exe").
     * @return 0 on success; non-zero on failure.
     */
    int attachToProcess(std::string exeName);

    /**
     * @brief Detaches from the current debuggee.
     * @return 0 on success; non-zero on failure.
     */
    int detachFromProcess();

    /**
     * @brief Main debugger message loop.
     * @return 0 on normal exit; non-zero on error.
     */
    int loop();
};

} // namespace RoboDBG

#endif
