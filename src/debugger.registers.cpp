
#include "debugger.h"

namespace RoboDBG {
    // ===========================
    // GET REGISTER
    // ===========================

    #ifdef _WIN64
    int64_t Debugger::getRegister(HANDLE hThread, Register64 reg) {
        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_ALL;

        if (!GetThreadContext(hThread, &ctx)) {
            std::cerr << "GetThreadContext failed: " << GetLastError() << "\n";
            return -1;
        }

        switch (reg) {
            case Register64::RAX: return ctx.Rax;
            case Register64::RBX: return ctx.Rbx;
            case Register64::RCX: return ctx.Rcx;
            case Register64::RDX: return ctx.Rdx;
            case Register64::RSI: return ctx.Rsi;
            case Register64::RDI: return ctx.Rdi;
            case Register64::RBP: return ctx.Rbp;
            case Register64::RSP: return ctx.Rsp;
            case Register64::R8:  return ctx.R8;
            case Register64::R9:  return ctx.R9;
            case Register64::R10: return ctx.R10;
            case Register64::R11: return ctx.R11;
            case Register64::R12: return ctx.R12;
            case Register64::R13: return ctx.R13;
            case Register64::R14: return ctx.R14;
            case Register64::R15: return ctx.R15;
            case Register64::RIP: return ctx.Rip;
        }

        return -1;
    }

    void Debugger::setRegister(HANDLE hThread, Register64 reg, int64_t value) {
        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_ALL;

        if (!GetThreadContext(hThread, &ctx)) {
            std::cerr << "GetThreadContext failed: " << GetLastError() << "\n";
            return;
        }

        switch (reg) {
            case Register64::RAX: ctx.Rax = value; break;
            case Register64::RBX: ctx.Rbx = value; break;
            case Register64::RCX: ctx.Rcx = value; break;
            case Register64::RDX: ctx.Rdx = value; break;
            case Register64::RSI: ctx.Rsi = value; break;
            case Register64::RDI: ctx.Rdi = value; break;
            case Register64::RBP: ctx.Rbp = value; break;
            case Register64::RSP: ctx.Rsp = value; break;
            case Register64::R8:  ctx.R8  = value; break;
            case Register64::R9:  ctx.R9  = value; break;
            case Register64::R10: ctx.R10 = value; break;
            case Register64::R11: ctx.R11 = value; break;
            case Register64::R12: ctx.R12 = value; break;
            case Register64::R13: ctx.R13 = value; break;
            case Register64::R14: ctx.R14 = value; break;
            case Register64::R15: ctx.R15 = value; break;
            case Register64::RIP: ctx.Rip = value; break;
        }

        SetThreadContext(hThread, &ctx);
    }
    #else
    int32_t Debugger::getRegister(HANDLE hThread, Register32 reg) {
        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_ALL;

        if (!GetThreadContext(hThread, &ctx)) {
            std::cerr << "GetThreadContext failed: " << GetLastError() << "\n";
            return -1;
        }

        switch (reg) {
            case Register32::EAX: return ctx.Eax;
            case Register32::EBX: return ctx.Ebx;
            case Register32::ECX: return ctx.Ecx;
            case Register32::EDX: return ctx.Edx;
            case Register32::ESI: return ctx.Esi;
            case Register32::EDI: return ctx.Edi;
            case Register32::EBP: return ctx.Ebp;
            case Register32::ESP: return ctx.Esp;
            case Register32::EIP: return ctx.Eip;
        }

        return -1; // unreachable if all cases covered
    }

    void Debugger::setRegister(HANDLE hThread, Register32 reg, int value) {
        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_ALL;

        if (!GetThreadContext(hThread, &ctx)) {
            std::cerr << "GetThreadContext failed: " << GetLastError() << "\n";
            return;
        }

        switch (reg) {
            case Register32::EAX: ctx.Eax = value; break;
            case Register32::EBX: ctx.Ebx = value; break;
            case Register32::ECX: ctx.Ecx = value; break;
            case Register32::EDX: ctx.Edx = value; break;
            case Register32::ESI: ctx.Esi = value; break;
            case Register32::EDI: ctx.Edi = value; break;
            case Register32::EBP: ctx.Ebp = value; break;
            case Register32::ESP: ctx.Esp = value; break;
            case Register32::EIP: ctx.Eip = value; break;
        }

        SetThreadContext(hThread, &ctx);
    }

    #endif


    #ifdef _WIN64
    void Debugger::setFlag(HANDLE hThread, Flags64 flag, bool enabled) {
        SuspendThread(hThread);

        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_FULL;

        if (!GetThreadContext(hThread, &ctx)) {
            std::cerr << "GetThreadContext failed\n";
            ResumeThread(hThread);
            return;
        }

        if (enabled)
            ctx.EFlags |= static_cast<DWORD64>(flag);
        else
            ctx.EFlags &= ~static_cast<DWORD64>(flag);

        if (!SetThreadContext(hThread, &ctx)) {
            std::cerr << "SetThreadContext failed\n";
        }

        ResumeThread(hThread);
    }

    bool Debugger::getFlag(HANDLE hThread, Flags64 flag) {
        SuspendThread(hThread);

        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_FULL;

        if (!GetThreadContext(hThread, &ctx)) {
            std::cerr << "GetThreadContext failed\n";
            ResumeThread(hThread);
            return false;
        }

        bool result = (ctx.EFlags & static_cast<DWORD64>(flag)) != 0;
        ResumeThread(hThread);
        return result;
    }

    #else

    void Debugger::setFlag(HANDLE hThread, Flags32 flag, bool enabled) {
        SuspendThread(hThread);

        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_FULL;

        if (!GetThreadContext(hThread, &ctx)) {
            std::cerr << "GetThreadContext failed\n";
            ResumeThread(hThread);
            return;
        }

        if (enabled)
            ctx.EFlags |= static_cast<DWORD>(flag);
        else
            ctx.EFlags &= ~static_cast<DWORD>(flag);

        if (!SetThreadContext(hThread, &ctx)) {
            std::cerr << "SetThreadContext failed\n";
        }

        ResumeThread(hThread);
    }

    bool Debugger::getFlag(HANDLE hThread, Flags32 flag) {
        SuspendThread(hThread);

        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_FULL;

        if (!GetThreadContext(hThread, &ctx)) {
            std::cerr << "GetThreadContext failed\n";
            ResumeThread(hThread);
            return false;
        }

        bool result = (ctx.EFlags & static_cast<DWORD>(flag)) != 0;
        ResumeThread(hThread);
        return result;
    }

    #endif
}
