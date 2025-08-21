// dbg.nb.cpp
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/trampoline.h>          // <-- needed for NB_TRAMPOLINE/NB_OVERRIDE*

#include "debugger.h"

namespace nb = nanobind;
using namespace nb::literals;

class PyDebugger : public RoboDBG::Debugger {
public:
    //using RoboDBG::Debugger::Debugger;

    // Expose protected base methods as public so binding lambdas can call them.
    using RoboDBG::Debugger::setBreakpoint;
    using RoboDBG::Debugger::setHardwareBreakpoint;
    using RoboDBG::Debugger::setHardwareBreakpointOnThread;
    using RoboDBG::Debugger::getHardwareBreakpoints;
    using RoboDBG::Debugger::enableSingleStep;
    using RoboDBG::Debugger::decrementIP;
    using RoboDBG::Debugger::clearHardwareBreakpoint;
    using RoboDBG::Debugger::clearHardwareBreakpointOnThread;
    using RoboDBG::Debugger::getBreakpointByReg;
    using RoboDBG::Debugger::getMemoryPages;
    using RoboDBG::Debugger::changeMemoryProtection;
    using RoboDBG::Debugger::searchInMemory;
    using RoboDBG::Debugger::writeMemory;
    using RoboDBG::Debugger::readMemory;
    using RoboDBG::Debugger::ASLR;
    using RoboDBG::Debugger::hideDebugger;
    using RoboDBG::Debugger::printIP;
    using RoboDBG::Debugger::actualizeThreadList;

    // Mark this as a trampoline for 14 Python-overridable virtuals
    NB_TRAMPOLINE(RoboDBG::Debugger, 14);

    // === Virtual Callbacks (C++ -> Python) ===
    void onStart(uintptr_t imageBase, uintptr_t entryPoint) override {
        NB_OVERRIDE_NAME("on_start", onStart, imageBase, entryPoint);
    }

    void onEnd(DWORD exitCode, DWORD pid) override {
        NB_OVERRIDE_NAME("on_end", onEnd, exitCode, pid);
    }

    void onThreadCreate(HANDLE hThread, DWORD threadId, uintptr_t threadBase, uintptr_t startAddress) override {
        NB_OVERRIDE_NAME("on_thread_create", onThreadCreate, hThread, threadId, threadBase, startAddress);
    }

    void onThreadExit(DWORD threadID) override {
        NB_OVERRIDE_NAME("on_thread_exit", onThreadExit, threadID);
    }

    bool onDLLLoad(uintptr_t address, std::string dllName, uintptr_t entryPoint) override {
        NB_OVERRIDE_NAME("on_dll_load", onDLLLoad, address, dllName, entryPoint);
        return true;
    }

    void onDLLUnload(uintptr_t address, std::string dllName) override {
        NB_OVERRIDE_NAME("on_dll_unload", onDLLUnload, address, dllName);
    }

    RoboDBG::BreakpointAction onBreakpoint(uintptr_t address, HANDLE hThread) override {
        NB_OVERRIDE_NAME("on_breakpoint", onBreakpoint, address, hThread);
    }

    RoboDBG::BreakpointAction onHardwareBreakpoint(uintptr_t address, HANDLE hThread, RoboDBG::DRReg reg) override {
        NB_OVERRIDE_NAME("on_hardware_breakpoint", onHardwareBreakpoint, address, hThread, reg);
    }

    void onSinglestep(uintptr_t address, HANDLE hThread) override {
        NB_OVERRIDE_NAME("on_single_step", onSinglestep, address, hThread);
    }

    void onDebugString(std::string dbgString) override {
        NB_OVERRIDE_NAME("on_debug_string", onDebugString, dbgString);
    }

    void onAccessViolation(uintptr_t address, uintptr_t faultingAddress, long accessType) override {
        NB_OVERRIDE_NAME("on_access_violation", onAccessViolation, address, faultingAddress, accessType);
    }

    void onRIPError(const RIP_INFO& rip) override {
        NB_OVERRIDE_NAME("on_rip_error", onRIPError, rip);
    }

    void onUnknownException(uintptr_t addr, DWORD code) override {
        NB_OVERRIDE_NAME("on_unknown_exception", onUnknownException, addr, code);
    }

    void onUnknownDebugEvent(DWORD code) override {
        NB_OVERRIDE_NAME("on_unknown_debug_event", onUnknownDebugEvent, code);
    }

    // --- read-only views for Python (need access to protected members) ---
    nb::dict py_get_breakpoints() const {
        nb::dict d;
        for (const auto& [addr, byte] : breakpoints) {
            d[nb::int_(reinterpret_cast<uintptr_t>(addr))] = nb::int_(static_cast<unsigned int>(byte));
        }
        return d;
    }

    nb::dict py_get_ddls() const { // kept name to match your property below
        nb::dict d;
        for (const auto& [addr, byte] : dlls) {
            d[nb::int_(reinterpret_cast<uintptr_t>(addr))] = nb::int_(static_cast<unsigned int>(byte));
        }
        return d;
    }

    nb::list py_get_hw_breakpoints_map() const {
        nb::list out;
        for (const auto& [addr, bp] : hwBreakpoints) {
            out.append(nb::make_tuple(reinterpret_cast<uintptr_t>(addr), bp));
        }
        return out;
    }

    nb::list py_get_threads() const {
        nb::list out;
        for (const auto& t : threads) {
            out.append(t);
        }
        return out;
    }

    int py_get_debugged_pid() const { return debuggedPid; }

    uintptr_t py_get_process_handle() {
        return reinterpret_cast<uintptr_t>(this->getProcessHandle());
    }

    RoboDBG::MemoryRegion_t py_get_page_by_address(uintptr_t address) {
        return this->getPageByAddress(reinterpret_cast<LPVOID>(address));
    }

    #ifdef _WIN64
    bool py_get_flag(HANDLE hThread, RoboDBG::Flags64 flag) { return this->getFlag(hThread, flag); }
    void py_set_flag(HANDLE hThread, RoboDBG::Flags64 flag, bool enabled) { this->setFlag(hThread, flag, enabled); }
    int64_t py_get_register(HANDLE hThread, RoboDBG::Register64 reg) { return this->getRegister(hThread, reg); }
    void   py_set_register(HANDLE hThread, RoboDBG::Register64 reg, int64_t value) { this->setRegister(hThread, reg, value); }
    #else
    bool py_get_flag(HANDLE hThread, RoboDBG::Flags32 flag) { return this->getFlag(hThread, flag); }
    void py_set_flag(HANDLE hThread, RoboDBG::Flags32 flag, bool enabled) { this->setFlag(hThread, flag, enabled); }
    int32_t py_get_register(HANDLE hThread, RoboDBG::Register32 reg) { return this->getRegister(hThread, reg); }
    void    py_set_register(HANDLE hThread, RoboDBG::Register32 reg, int32_t value) { this->setRegister(hThread, reg, value); }
    #endif
};

NB_MODULE(dbg, m) {
    // === Enums ===
    nb::enum_<RoboDBG::BreakpointAction>(m, "BreakpointAction")
    .value("BREAK", RoboDBG::BreakpointAction::BREAK)
    .value("RESTORE", RoboDBG::BreakpointAction::RESTORE)
    .value("SINGLE_STEP", RoboDBG::BreakpointAction::SINGLE_STEP);

    nb::enum_<RoboDBG::AccessType>(m, "AccessType")
    .value("EXECUTE", RoboDBG::AccessType::EXECUTE)
    .value("WRITE", RoboDBG::AccessType::WRITE)
    .value("READWRITE", RoboDBG::AccessType::READWRITE);

    nb::enum_<RoboDBG::DRReg>(m, "DRReg")
    .value("NOP", RoboDBG::DRReg::NOP)
    .value("DR0", RoboDBG::DRReg::DR0)
    .value("DR1", RoboDBG::DRReg::DR1)
    .value("DR2", RoboDBG::DRReg::DR2)
    .value("DR3", RoboDBG::DRReg::DR3);

    nb::enum_<RoboDBG::BreakpointLength>(m, "BreakpointLength")
    .value("BYTE", RoboDBG::BreakpointLength::BYTE)
    .value("WORD", RoboDBG::BreakpointLength::WORD)
    .value("DWORD", RoboDBG::BreakpointLength::DWORD)
    .value("QWORD", RoboDBG::BreakpointLength::QWORD);

    #ifdef _WIN64
    nb::enum_<RoboDBG::Flags64>(m, "Flags64")
    .value("CF", RoboDBG::Flags64::CF).value("PF", RoboDBG::Flags64::PF)
    .value("AF", RoboDBG::Flags64::AF).value("ZF", RoboDBG::Flags64::ZF)
    .value("SF", RoboDBG::Flags64::SF).value("TF", RoboDBG::Flags64::TF)
    .value("IF", RoboDBG::Flags64::IF).value("DF", RoboDBG::Flags64::DF)
    .value("OF", RoboDBG::Flags64::OF);

    nb::enum_<RoboDBG::Register64>(m, "Register64")
    .value("RAX", RoboDBG::Register64::RAX).value("RBX", RoboDBG::Register64::RBX)
    .value("RCX", RoboDBG::Register64::RCX).value("RDX", RoboDBG::Register64::RDX)
    .value("RSI", RoboDBG::Register64::RSI).value("RDI", RoboDBG::Register64::RDI)
    .value("RBP", RoboDBG::Register64::RBP).value("RSP", RoboDBG::Register64::RSP)
    .value("R8", RoboDBG::Register64::R8).value("R9", RoboDBG::Register64::R9)
    .value("R10", RoboDBG::Register64::R10).value("R11", RoboDBG::Register64::R11)
    .value("R12", RoboDBG::Register64::R12).value("R13", RoboDBG::Register64::R13)
    .value("R14", RoboDBG::Register64::R14).value("R15", RoboDBG::Register64::R15)
    .value("RIP", RoboDBG::Register64::RIP);
    #else
    nb::enum_<RoboDBG::Flags32>(m, "Flags32")
    .value("CF", RoboDBG::Flags32::CF).value("PF", RoboDBG::Flags32::PF)
    .value("AF", RoboDBG::Flags32::AF).value("ZF", RoboDBG::Flags32::ZF)
    .value("SF", RoboDBG::Flags32::SF).value("TF", RoboDBG::Flags32::TF)
    .value("IF", RoboDBG::Flags32::IF).value("DF", RoboDBG::Flags32::DF)
    .value("OF", RoboDBG::Flags32::OF);

    nb::enum_<RoboDBG::Register32>(m, "Register32")
    .value("EAX", RoboDBG::Register32::EAX).value("EBX", RoboDBG::Register32::EBX)
    .value("ECX", RoboDBG::Register32::ECX).value("EDX", RoboDBG::Register32::EDX)
    .value("ESI", RoboDBG::Register32::ESI).value("EDI", RoboDBG::Register32::EDI)
    .value("EBP", RoboDBG::Register32::EBP).value("ESP", RoboDBG::Register32::ESP)
    .value("EIP", RoboDBG::Register32::EIP);
    #endif

    // === PODs / structs ===
    nb::class_<RoboDBG::thread_t>(m, "ThreadInfo")
    .def_rw("h_thread", &RoboDBG::thread_t::hThread)
    .def_rw("thread_id", &RoboDBG::thread_t::threadId)
    .def_rw("thread_base", &RoboDBG::thread_t::threadBase)
    .def_rw("start_address", &RoboDBG::thread_t::startAddress);

    nb::class_<RoboDBG::hwBp_t>(m, "HardwareBreakpoint")
    .def_rw("h_thread", &RoboDBG::hwBp_t::hThread)
    .def_rw("address", &RoboDBG::hwBp_t::address)
    .def_rw("reg", &RoboDBG::hwBp_t::reg)
    .def_rw("type", &RoboDBG::hwBp_t::type)
    .def_rw("len", &RoboDBG::hwBp_t::len);

    nb::class_<RoboDBG::MemoryRegion_t>(m, "MemoryRegion")
    .def_rw("base_address", &RoboDBG::MemoryRegion_t::BaseAddress)
    .def_rw("region_size", &RoboDBG::MemoryRegion_t::RegionSize)
    .def_rw("state", &RoboDBG::MemoryRegion_t::State)
    .def_rw("protect", &RoboDBG::MemoryRegion_t::Protect)
    .def_rw("type", &RoboDBG::MemoryRegion_t::Type);

    // === Main class (note trampoline as 2nd template arg) ===
    nb::class_<RoboDBG::Debugger, PyDebugger>(m, "Debugger")
    .def(nb::init<>())
    .def(nb::init<bool>(), "verbose"_a=false)

    .def("loop", &RoboDBG::Debugger::loop)
    .def("start",
         nb::overload_cast<std::string>(&RoboDBG::Debugger::start),
         "exe_name"_a)

    .def("start",
         nb::overload_cast<std::string, const std::vector<std::string>&>(&RoboDBG::Debugger::start),
         "exe_name"_a, "args"_a)

    .def("detach", &RoboDBG::Debugger::detach)
    .def("attach", &RoboDBG::Debugger::attach)

    .def("set_breakpoint",
         [](RoboDBG::Debugger &self, uintptr_t address) {
             static_cast<PyDebugger&>(self).setBreakpoint(reinterpret_cast<LPVOID>(address));
         }, "address"_a)

    .def("set_hardware_breakpoint",
         [](RoboDBG::Debugger &self, const RoboDBG::hwBp_t& bp) {
             return static_cast<PyDebugger&>(self).setHardwareBreakpoint(bp);
         }, "bp"_a)

    .def("set_hardware_breakpoint_on_thread",
         [](RoboDBG::Debugger &self, const RoboDBG::hwBp_t& bp) {
             return static_cast<PyDebugger&>(self).setHardwareBreakpointOnThread(bp);
         }, "bp"_a)

    .def("set_hardware_breakpoint",
         [](RoboDBG::Debugger &self, int address, RoboDBG::DRReg reg, RoboDBG::AccessType type, RoboDBG::BreakpointLength len) {
             return static_cast<PyDebugger&>(self).setHardwareBreakpoint(address, reg, type, len);
         }, "address"_a, "reg"_a, "type"_a, "len"_a)

    .def("get_hardware_breakpoints",
         [](RoboDBG::Debugger &self) {
             return static_cast<PyDebugger&>(self).getHardwareBreakpoints();
         })

    .def("enable_single_step",
         [](RoboDBG::Debugger &self, HANDLE hThread) {
             static_cast<PyDebugger&>(self).enableSingleStep(hThread);
         }, "h_thread"_a)

    .def("decrement_ip",
         [](RoboDBG::Debugger &self, HANDLE hThread) {
             static_cast<PyDebugger&>(self).decrementIP(hThread);
         }, "h_thread"_a)

    .def("clear_hardware_breakpoint",
         [](RoboDBG::Debugger &self, RoboDBG::DRReg reg) {
             return static_cast<PyDebugger&>(self).clearHardwareBreakpoint(reg);
         }, "reg"_a)

    .def("clear_hardware_breakpoint_on_thread",
         [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::DRReg reg) {
             return static_cast<PyDebugger&>(self).clearHardwareBreakpointOnThread(hThread, reg);
         }, "h_thread"_a, "reg"_a)

    .def("get_breakpoint_by_reg",
         [](RoboDBG::Debugger &self, RoboDBG::DRReg reg) {
             return static_cast<PyDebugger&>(self).getBreakpointByReg(reg);
         }, "reg"_a)

    .def("get_memory_pages",
         [](RoboDBG::Debugger &self) {
             return static_cast<PyDebugger&>(self).getMemoryPages();
         })

    .def("get_page_by_address",
         [](RoboDBG::Debugger &self, uintptr_t address) {
             return static_cast<PyDebugger&>(self).py_get_page_by_address(address);
         },
         "address"_a)

    .def("change_memory_protection",
         [](RoboDBG::Debugger &self, RoboDBG::MemoryRegion_t page, DWORD newProtect) {
             return static_cast<PyDebugger&>(self).changeMemoryProtection(page, newProtect);
         }, "page"_a, "new_protect"_a)

    .def("search_in_memory",
         [](RoboDBG::Debugger &self, const std::vector<uint8_t>& pattern) {
             return static_cast<PyDebugger&>(self).searchInMemory(pattern);
         }, "pattern"_a)

    .def("change_memory_protection_raw",
         [](RoboDBG::Debugger &self, uintptr_t address, size_t size, DWORD newProtect) {
             return static_cast<PyDebugger&>(self).changeMemoryProtection(address, size, newProtect);
         }, "address"_a, "size"_a, "new_protect"_a)

    .def("write_memory",
         [](RoboDBG::Debugger &self, uintptr_t address, const std::vector<uint8_t>& data) {
             return static_cast<PyDebugger&>(self).writeMemory(address, data.data(), data.size());
         }, "address"_a, "data"_a)

    .def("read_memory",
         [](RoboDBG::Debugger &self, uintptr_t address, size_t size) {
             std::vector<uint8_t> buffer(size);
             if (!static_cast<PyDebugger&>(self).readMemory(address, buffer.data(), size))
                 buffer.clear();
             return buffer;
         }, "address"_a, "size"_a)

    .def("aslr",
         [](RoboDBG::Debugger &self, uintptr_t address) {
             return static_cast<PyDebugger&>(self).ASLR(address);
         }, "address"_a)

    .def("hide_debugger",
         [](RoboDBG::Debugger &self) {
             return static_cast<PyDebugger&>(self).hideDebugger();
         })

    .def("print_ip",
         [](RoboDBG::Debugger &self, HANDLE hThread) {
             static_cast<PyDebugger&>(self).printIP(hThread);
         }, "h_thread"_a)

    .def("get_process_handle",
         [](RoboDBG::Debugger &self) {
             return static_cast<PyDebugger&>(self).py_get_process_handle();
         })

    // read-only “properties”
    .def_prop_ro("breakpoints", [](RoboDBG::Debugger& self) {
        return static_cast<PyDebugger&>(self).py_get_breakpoints();
    })
    .def_prop_ro("hw_breakpoints", [](RoboDBG::Debugger& self) {
        return static_cast<PyDebugger&>(self).py_get_hw_breakpoints_map();
    })
    .def_prop_ro("dlls", [](RoboDBG::Debugger& self) {
        return static_cast<PyDebugger&>(self).py_get_ddls();
    })
    .def_prop_ro("threads", [](RoboDBG::Debugger& self) {
        return static_cast<PyDebugger&>(self).py_get_threads();
    })
    .def_prop_ro("debugged_pid", [](RoboDBG::Debugger& self) {
        return static_cast<PyDebugger&>(self).py_get_debugged_pid();
    })

    .def("actualize_thread_list",
         [](RoboDBG::Debugger& self) {
             static_cast<PyDebugger&>(self).actualizeThreadList();
         })

    #ifdef _WIN64
    .def("get_flag",
         [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::Flags64 flag) {
             return static_cast<PyDebugger&>(self).py_get_flag(hThread, flag);
         }, "h_thread"_a, "flag"_a)
    .def("set_flag",
         [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::Flags64 flag, bool enabled) {
             static_cast<PyDebugger&>(self).py_set_flag(hThread, flag, enabled);
         }, "h_thread"_a, "flag"_a, "enabled"_a)
    .def("get_register",
         [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::Register64 reg) {
             return static_cast<PyDebugger&>(self).py_get_register(hThread, reg);
         }, "h_thread"_a, "reg"_a)
    .def("set_register",
         [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::Register64 reg, int64_t value) {
             static_cast<PyDebugger&>(self).py_set_register(hThread, reg, value);
         }, "h_thread"_a, "reg"_a, "value"_a)
    #else
    .def("get_flag",
         [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::Flags32 flag) {
             return static_cast<PyDebugger&>(self).py_get_flag(hThread, flag);
         }, "h_thread"_a, "flag"_a)
    .def("set_flag",
         [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::Flags32 flag, bool enabled) {
             static_cast<PyDebugger&>(self).py_set_flag(hThread, flag, enabled);
         }, "h_thread"_a, "flag"_a, "enabled"_a)
    .def("get_register",
         [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::Register32 reg) {
             return static_cast<PyDebugger&>(self).py_get_register(hThread, reg);
         }, "h_thread"_a, "reg"_a)
    .def("set_register",
         [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::Register32 reg, int32_t value) {
             static_cast<PyDebugger&>(self).py_set_register(hThread, reg, value);
         }, "h_thread"_a, "reg"_a, "value"_a)
    #endif
    ;
}
