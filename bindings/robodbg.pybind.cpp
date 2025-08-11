#include <pybind11/pybind11.h>
#include <pybind11/stl.h>    // for std::string, std::vector support

#include "debugger.h"

namespace py = pybind11;


class PyDebugger : public RoboDBG::Debugger {
public:
    using RoboDBG::Debugger::Debugger;

    // === Virtual Callbacks ===
    void onStart(uintptr_t imageBase, uintptr_t entryPoint) override {
        PYBIND11_OVERRIDE(void, RoboDBG::Debugger, onStart, imageBase, entryPoint);
    }

    void onEnd(DWORD exitCode, DWORD pid) override {
        PYBIND11_OVERRIDE(void, RoboDBG::Debugger, onEnd, exitCode, pid);
    }

    void onThreadCreate(HANDLE hThread, DWORD threadId, uintptr_t threadBase, uintptr_t startAddress) override {
        PYBIND11_OVERRIDE(void, RoboDBG::Debugger, onThreadCreate, hThread, threadId, threadBase, startAddress);
    }

    void onThreadExit(DWORD threadID) override {
        PYBIND11_OVERRIDE(void, RoboDBG::Debugger, onThreadExit, threadID);
    }

    bool onDLLLoad(uintptr_t address, std::string dllName, uintptr_t entryPoint) override {
        PYBIND11_OVERRIDE(bool, RoboDBG::Debugger, onDLLLoad, address, dllName, entryPoint);
    }

    void onDLLUnload(uintptr_t address, std::string dllName) override {
        PYBIND11_OVERRIDE(void, RoboDBG::Debugger, onDLLUnload, address, dllName);
    }

    RoboDBG::BreakpointAction onBreakpoint(uintptr_t address, HANDLE hThread) override {
        PYBIND11_OVERRIDE(RoboDBG::BreakpointAction, RoboDBG::Debugger, onBreakpoint, address, hThread);
    }

    RoboDBG::BreakpointAction onHardwareBreakpoint(uintptr_t address, HANDLE hThread, RoboDBG::DRReg reg) override {
        PYBIND11_OVERRIDE(RoboDBG::BreakpointAction, RoboDBG::Debugger, onHardwareBreakpoint, address, hThread, reg);
    }

    void onSinglestep(uintptr_t address, HANDLE hThread) override {
        PYBIND11_OVERRIDE(void, RoboDBG::Debugger, onSinglestep, address, hThread);
    }

    void onDebugString(std::string dbgString) override {
        PYBIND11_OVERRIDE(void, RoboDBG::Debugger, onDebugString, dbgString);
    }

    void onAccessViolation(uintptr_t address, uintptr_t faultingAddress, long accessType) override {
        PYBIND11_OVERRIDE(void, RoboDBG::Debugger, onAccessViolation, address, faultingAddress, accessType);
    }

    void onRIPError(const RIP_INFO& rip) override {
        PYBIND11_OVERRIDE(void, RoboDBG::Debugger, onRIPError, rip);
    }

    void onUnknownException(uintptr_t addr, DWORD code) override {
        PYBIND11_OVERRIDE(void, RoboDBG::Debugger, onUnknownException, addr, code);
    }

    void onUnknownDebugEvent(DWORD code) override {
        PYBIND11_OVERRIDE(void, RoboDBG::Debugger, onUnknownDebugEvent, code);
    }

    // === Protected -> Public Python Wrappers ===
    bool pySetHardwareBreakpointOnThread(RoboDBG::hwBp_t bp) {
        return this->setHardwareBreakpointOnThread(bp);
    }

    bool pySetHardwareBreakpoint(RoboDBG::hwBp_t bp) {
        return this->setHardwareBreakpoint(bp);
    }

    bool pySetHardwareBreakpoint(int address, RoboDBG::DRReg reg, RoboDBG::AccessType type, RoboDBG::BreakpointLength len) {
        return this->setHardwareBreakpoint(address, reg, type, len);
    }

    std::vector<RoboDBG::hwBp_t> pyGetHardwareBreakpoints() {
        return this->getHardwareBreakpoints();
    }

    void pyEnableSingleStep(HANDLE hThread) {
        this->enableSingleStep(hThread);
    }

    void pyDecrementIP(HANDLE hThread) {
        this->decrementIP(hThread);
    }

    bool pyClearHardwareBreakpoint(RoboDBG::DRReg reg) {
        return this->clearHardwareBreakpoint(reg);
    }

    bool pyClearHardwareBreakpointOnThread(HANDLE hThread, RoboDBG::DRReg reg) {
        return this->clearHardwareBreakpointOnThread(hThread, reg);
    }

    RoboDBG::hwBp_t pyGetBreakpointByReg(RoboDBG::DRReg reg) {
        return this->getBreakpointByReg(reg);
    }

    void pySetBreakpoint(uintptr_t address) {
        this->setBreakpoint(reinterpret_cast<LPVOID>(address));
    }

    std::vector<RoboDBG::MemoryRegion_t> pyGetMemoryPages() {
        return this->getMemoryPages();
    }

    bool pyChangeMemoryProtection(RoboDBG::MemoryRegion_t page, DWORD newProtect) {
        return this->changeMemoryProtection(page, newProtect);
    }

    std::vector<uintptr_t> pySearchInMemory(const std::vector<BYTE>& pattern) {
        return this->searchInMemory(pattern);
    }

    bool pyChangeMemoryProtectionRaw(uintptr_t baseAddress, size_t size, DWORD newProtect) {
        return this->changeMemoryProtection(baseAddress, size, newProtect);
    }

    bool pyWriteMemory(uintptr_t address, const std::vector<uint8_t>& data) {
        return this->writeMemory(address, data.data(), data.size());
    }

    std::vector<uint8_t> pyReadMemory(uintptr_t address, size_t size) {
        std::vector<uint8_t> buffer(size);
        if (!this->readMemory(address, buffer.data(), size))
            return {};
        return buffer;
    }

    uintptr_t pyASLR(uintptr_t address) {
        return this->ASLR(address);
    }

    bool pyHideDebugger() {
        return this->hideDebugger();
    }

    void pyPrintIP(HANDLE hThread) {
        this->printIP(hThread);
    }

    // --- read-only views for Python ---
    pybind11::dict pyGetBreakpoints() const {
        namespace py = pybind11;
        py::dict d;
        for (const auto& [addr, byte] : breakpoints) {
            d[py::int_(reinterpret_cast<uintptr_t>(addr))] = py::int_(static_cast<unsigned int>(byte));
        }
        return d;
    }

    pybind11::dict pyGetDlls() const {
        namespace py = pybind11;
        py::dict d;
        for (const auto& [addr, byte] : dlls) {
            d[py::int_(reinterpret_cast<uintptr_t>(addr))] = py::int_(static_cast<unsigned int>(byte));
        }
        return d;
    }

    pybind11::list pyGetHwBreakpointsMap() const {
        namespace py = pybind11;
        py::list out;
        for (const auto& [addr, bp] : hwBreakpoints) {
            out.append(py::make_tuple(reinterpret_cast<uintptr_t>(addr), bp));
        }
        return out;
    }

    pybind11::list pyGetThreads() const {
        namespace py = pybind11;
        py::list out;
        for (const auto& t : threads) {
            out.append(t); // requires thread_t to be bound; otherwise build a dict here
        }
        return out;
    }

    void pyActualizeThreadList() {
        this->actualizeThreadList( );
    }

    int pyGetDebuggedPid() const { return debuggedPid; }

    uintptr_t pyGetProcessHandle( ) {
        return  reinterpret_cast<uintptr_t>(this->getProcessHandle( ));
    }

    #ifdef _WIN64
    // 64-bit register wrapper functions
    bool pyGetFlag(HANDLE hThread, RoboDBG::Flags64 flag) {
        return this->getFlag(hThread, flag);
    }

    void pySetFlag(HANDLE hThread, RoboDBG::Flags64 flag, bool enabled) {
        this->setFlag(hThread, flag, enabled);
    }

    int64_t pyGetRegister(HANDLE hThread, RoboDBG::Register64 reg) {
        return this->getRegister(hThread, reg);
    }

    void pySetRegister(HANDLE hThread, RoboDBG::Register64 reg, int64_t value) {
        this->setRegister(hThread, reg, value);
    }
    #else
    // 32-bit register wrapper functions
    bool pyGetFlag(HANDLE hThread, RoboDBG::Flags32 flag) {
        return this->getFlag(hThread, flag);
    }

    void pySetFlag(HANDLE hThread, RoboDBG::Flags32 flag, bool enabled) {
        this->setFlag(hThread, flag, enabled);
    }

    int32_t pyGetRegister(HANDLE hThread, RoboDBG::Register32 reg) {
        return this->getRegister(hThread, reg);
    }

    void pySetRegister(HANDLE hThread, RoboDBG::Register32 reg, int32_t value) {
        this->setRegister(hThread, reg, value);
    }
    #endif
};

PYBIND11_MODULE(dbg, m) {
    py::enum_<RoboDBG::BreakpointAction>(m, "BreakpointAction")
    .value("BREAK", RoboDBG::BreakpointAction::BREAK)
    .value("RESTORE", RoboDBG::BreakpointAction::RESTORE)
    .value("SINGLE_STEP", RoboDBG::BreakpointAction::SINGLE_STEP)
    .export_values();

    py::enum_<RoboDBG::AccessType>(m, "AccessType")
    .value("EXECUTE", RoboDBG::AccessType::EXECUTE)
    .value("WRITE", RoboDBG::AccessType::WRITE)
    .value("READWRITE", RoboDBG::AccessType::READWRITE)
    .export_values();

    py::enum_<RoboDBG::DRReg>(m, "DRReg")
    .value("NOP", RoboDBG::DRReg::NOP)
    .value("DR0", RoboDBG::DRReg::DR0)
    .value("DR1", RoboDBG::DRReg::DR1)
    .value("DR2", RoboDBG::DRReg::DR2)
    .value("DR3", RoboDBG::DRReg::DR3)
    .export_values();

    py::enum_<RoboDBG::BreakpointLength>(m, "BreakpointLength")
    .value("BYTE", RoboDBG::BreakpointLength::BYTE)
    .value("WORD", RoboDBG::BreakpointLength::WORD)
    .value("DWORD", RoboDBG::BreakpointLength::DWORD)
    .value("QWORD", RoboDBG::BreakpointLength::QWORD)
    .export_values();

    py::class_<RoboDBG::thread_t>(m, "ThreadInfo")
    .def_readwrite("hThread", &RoboDBG::thread_t::hThread)
    .def_readwrite("threadId", &RoboDBG::thread_t::threadId)
    .def_readwrite("threadBase", &RoboDBG::thread_t::threadBase)
    .def_readwrite("startAddress", &RoboDBG::thread_t::startAddress);

    py::class_<RoboDBG::hwBp_t>(m, "HardwareBreakpoint")
    .def_readwrite("hThread", &RoboDBG::hwBp_t::hThread)
    .def_readwrite("address", &RoboDBG::hwBp_t::address)
    .def_readwrite("reg", &RoboDBG::hwBp_t::reg)
    .def_readwrite("type", &RoboDBG::hwBp_t::type)
    .def_readwrite("len", &RoboDBG::hwBp_t::len);

    py::class_<RoboDBG::MemoryRegion_t>(m, "MemoryRegion")
    .def_readwrite("BaseAddress", &RoboDBG::MemoryRegion_t::BaseAddress)
    .def_readwrite("RegionSize", &RoboDBG::MemoryRegion_t::RegionSize)
    .def_readwrite("State", &RoboDBG::MemoryRegion_t::State)
    .def_readwrite("Protect", &RoboDBG::MemoryRegion_t::Protect)
    .def_readwrite("Type", &RoboDBG::MemoryRegion_t::Type);

    // Pybind11 bindings for Flags and Register enums
    // Pybind11 bindings for Flags and Register enums


    #ifdef _WIN64
    // 64-bit Flags enum
    py::enum_<RoboDBG::Flags64>(m, "Flags64")
    .value("CF", RoboDBG::Flags64::CF, "Carry Flag")
    .value("PF", RoboDBG::Flags64::PF, "Parity Flag")
    .value("AF", RoboDBG::Flags64::AF, "Auxiliary Carry Flag")
    .value("ZF", RoboDBG::Flags64::ZF, "Zero Flag")
    .value("SF", RoboDBG::Flags64::SF, "Sign Flag")
    .value("TF", RoboDBG::Flags64::TF, "Trap Flag")
    .value("IF", RoboDBG::Flags64::IF, "Interrupt Enable Flag")
    .value("DF", RoboDBG::Flags64::DF, "Direction Flag")
    .value("OF", RoboDBG::Flags64::OF, "Overflow Flag")
    .export_values();

    // 64-bit Register enum
    py::enum_<RoboDBG::Register64>(m, "Register64")
    .value("RAX", RoboDBG::Register64::RAX, "Accumulator Register")
    .value("RBX", RoboDBG::Register64::RBX, "Base Register")
    .value("RCX", RoboDBG::Register64::RCX, "Counter Register")
    .value("RDX", RoboDBG::Register64::RDX, "Data Register")
    .value("RSI", RoboDBG::Register64::RSI, "Source Index Register")
    .value("RDI", RoboDBG::Register64::RDI, "Destination Index Register")
    .value("RBP", RoboDBG::Register64::RBP, "Base Pointer Register")
    .value("RSP", RoboDBG::Register64::RSP, "Stack Pointer Register")
    .value("R8", RoboDBG::Register64::R8, "General Purpose Register R8")
    .value("R9", RoboDBG::Register64::R9, "General Purpose Register R9")
    .value("R10", RoboDBG::Register64::R10, "General Purpose Register R10")
    .value("R11", RoboDBG::Register64::R11, "General Purpose Register R11")
    .value("R12", RoboDBG::Register64::R12, "General Purpose Register R12")
    .value("R13", RoboDBG::Register64::R13, "General Purpose Register R13")
    .value("R14", RoboDBG::Register64::R14, "General Purpose Register R14")
    .value("R15", RoboDBG::Register64::R15, "General Purpose Register R15")
    .value("RIP", RoboDBG::Register64::RIP, "Instruction Pointer Register")
    .export_values();

    #else
    // 32-bit Flags enum
    py::enum_<RoboDBG::Flags32>(m, "Flags32")
    .value("CF", RoboDBG::Flags32::CF, "Carry Flag")
    .value("PF", RoboDBG::Flags32::PF, "Parity Flag")
    .value("AF", RoboDBG::Flags32::AF, "Auxiliary Carry Flag")
    .value("ZF", RoboDBG::Flags32::ZF, "Zero Flag")
    .value("SF", RoboDBG::Flags32::SF, "Sign Flag")
    .value("TF", RoboDBG::Flags32::TF, "Trap Flag")
    .value("IF", RoboDBG::Flags32::IF, "Interrupt Enable Flag")
    .value("DF", RoboDBG::Flags32::DF, "Direction Flag")
    .value("OF", RoboDBG::Flags32::OF, "Overflow Flag")
    .export_values();

    // 32-bit Register enum
    py::enum_<RoboDBG::Register32>(m, "Register32")
    .value("EAX", RoboDBG::Register32::EAX, "Accumulator Register")
    .value("EBX", RoboDBG::Register32::EBX, "Base Register")
    .value("ECX", RoboDBG::Register32::ECX, "Counter Register")
    .value("EDX", RoboDBG::Register32::EDX, "Data Register")
    .value("ESI", RoboDBG::Register32::ESI, "Source Index Register")
    .value("EDI", RoboDBG::Register32::EDI, "Destination Index Register")
    .value("EBP", RoboDBG::Register32::EBP, "Base Pointer Register")
    .value("ESP", RoboDBG::Register32::ESP, "Stack Pointer Register")
    .value("EIP", RoboDBG::Register32::EIP, "Instruction Pointer Register")
    .export_values();
    #endif

    py::class_<RoboDBG::Debugger, PyDebugger>(m, "Debugger")
    .def(py::init<>())
    .def(py::init<bool>())
    .def("loop", &RoboDBG::Debugger::loop)
    .def("startProcess",
         static_cast<int (RoboDBG::Debugger::*)(std::string)>( &RoboDBG::Debugger::startProcess), py::arg("exe_name")
    ).def("startProcess",
         static_cast<int (RoboDBG::Debugger::*)(std::string, const std::vector<std::string>&)>(&RoboDBG::Debugger::startProcess), py::arg("exe_name"), py::arg("args")
    ).def("detachFromProcess", &RoboDBG::Debugger::detachFromProcess)
    .def("attachToProcess", &RoboDBG::Debugger::attachToProcess)
    .def("setBreakpoint", [](RoboDBG::Debugger &self, uintptr_t address) {
        static_cast<PyDebugger &>(self).pySetBreakpoint(address);
    })
    .def("setHardwareBreakpoint", [](RoboDBG::Debugger &self, RoboDBG::hwBp_t bp) {
        return static_cast<PyDebugger &>(self).pySetHardwareBreakpoint(bp);
    })
    .def("setHardwareBreakpointOnThread", [](RoboDBG::Debugger &self, RoboDBG::hwBp_t bp) {
        return static_cast<PyDebugger &>(self).pySetHardwareBreakpointOnThread(bp);
    })
    .def("setHardwareBreakpoint", [](RoboDBG::Debugger &self, int address, RoboDBG::DRReg reg, RoboDBG::AccessType type, RoboDBG::BreakpointLength len) {
        return static_cast<PyDebugger &>(self).pySetHardwareBreakpoint(address, reg, type, len);
    })
    .def("getHardwareBreakpoints", [](RoboDBG::Debugger &self) {
        return static_cast<PyDebugger &>(self).pyGetHardwareBreakpoints();
    })
    .def("enableSingleStep", [](RoboDBG::Debugger &self, HANDLE hThread) {
        return static_cast<PyDebugger &>(self).pyEnableSingleStep(hThread);
    })
    .def("decrementIP", [](RoboDBG::Debugger &self, HANDLE hThread) {
        return static_cast<PyDebugger &>(self).pyDecrementIP(hThread);
    })
    .def("clearHardwareBreakpoint", [](RoboDBG::Debugger &self, RoboDBG::DRReg reg) {
        return static_cast<PyDebugger &>(self).pyClearHardwareBreakpoint(reg);
    })
    .def("clearHardwareBreakpointOnThread", [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::DRReg reg) {
        return static_cast<PyDebugger &>(self).pyClearHardwareBreakpointOnThread(hThread, reg);
    })
    .def("getBreakpointByReg", [](RoboDBG::Debugger &self, RoboDBG::DRReg reg) {
        return static_cast<PyDebugger &>(self).pyGetBreakpointByReg(reg);
    })
    .def("getMemoryPages", [](RoboDBG::Debugger &self) {
        return static_cast<PyDebugger &>(self).pyGetMemoryPages();
    })
    .def("changeMemoryProtection", [](RoboDBG::Debugger &self, RoboDBG::MemoryRegion_t page, DWORD newProtect) {
        return static_cast<PyDebugger &>(self).pyChangeMemoryProtection(page, newProtect);
    })
    .def("searchInMemory", [](RoboDBG::Debugger &self, const std::vector<uint8_t>& pattern) {
        return static_cast<PyDebugger &>(self).pySearchInMemory(pattern);
    })
    .def("changeMemoryProtectionRaw", [](RoboDBG::Debugger &self, uintptr_t address, size_t size, DWORD newProtect) {
        return static_cast<PyDebugger &>(self).pyChangeMemoryProtectionRaw(address, size, newProtect);
    })
    .def("writeMemory", [](RoboDBG::Debugger &self, uintptr_t address, const std::vector<uint8_t>& data) {
        return static_cast<PyDebugger &>(self).pyWriteMemory(address, data);
    })
    .def("readMemory", [](RoboDBG::Debugger &self, uintptr_t address, size_t size) {
        return static_cast<PyDebugger &>(self).pyReadMemory(address, size);
    })
    .def("ASLR", [](RoboDBG::Debugger &self, uintptr_t address) {
        return static_cast<PyDebugger &>(self).pyASLR(address);
    })
    .def("hideDebugger", [](RoboDBG::Debugger &self) {
        return static_cast<PyDebugger &>(self).pyHideDebugger();
    })
    .def_property_readonly("breakpoints", [](RoboDBG::Debugger& self) {
    return static_cast<PyDebugger&>(self).pyGetBreakpoints();
    })
    .def_property_readonly("hwBreakpoints", [](RoboDBG::Debugger& self) {
        return static_cast<PyDebugger&>(self).pyGetHwBreakpointsMap();
    })
    .def_property_readonly("dlls", [](RoboDBG::Debugger& self) {
        return static_cast<PyDebugger&>(self).pyGetDlls();
    })
    .def_property_readonly("threads", [](RoboDBG::Debugger& self) {
        return static_cast<PyDebugger&>(self).pyGetThreads();
    })
    .def_property_readonly("debuggedPid", [](RoboDBG::Debugger& self) {
        return static_cast<PyDebugger&>(self).pyGetDebuggedPid();
    })
    .def_property_readonly("actualizeThreadList", [](RoboDBG::Debugger& self) {
        return static_cast<PyDebugger&>(self).pyActualizeThreadList();
    })
    .def("printIP", [](RoboDBG::Debugger &self, HANDLE hThread) {
        return static_cast<PyDebugger &>(self).pyPrintIP(hThread);
    })
    .def("getProcessHandle", [](RoboDBG::Debugger &self) {
        return static_cast<PyDebugger &>(self).pyGetProcessHandle( );
    })
    // Register function bindings
#ifdef _WIN64
    .def("getFlag", [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::Flags64 flag) {
        return static_cast<PyDebugger &>(self).pyGetFlag(hThread, flag);
    })
    .def("setFlag", [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::Flags64 flag, bool enabled) {
        return static_cast<PyDebugger &>(self).pySetFlag(hThread, flag, enabled);
    })
    .def("getRegister", [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::Register64 reg) {
        return static_cast<PyDebugger &>(self).pyGetRegister(hThread, reg);
    })
    .def("setRegister", [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::Register64 reg, int64_t value) {
        return static_cast<PyDebugger &>(self).pySetRegister(hThread, reg, value);
    })
#else
    .def("getFlag", [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::Flags32 flag) {
        return static_cast<PyDebugger &>(self).pyGetFlag(hThread, flag);
    })
    .def("setFlag", [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::Flags32 flag, bool enabled) {
        return static_cast<PyDebugger &>(self).pySetFlag(hThread, flag, enabled);
    })
    .def("getRegister", [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::Register32 reg) {
        return static_cast<PyDebugger &>(self).pyGetRegister(hThread, reg);
    })
    .def("setRegister", [](RoboDBG::Debugger &self, HANDLE hThread, RoboDBG::Register32 reg, int32_t value) {
        return static_cast<PyDebugger &>(self).pySetRegister(hThread, reg, value);
    })
    #endif
    ;
}
