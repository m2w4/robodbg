#include <stdio.h>
#include <iostream>
#include "src/debugger.h"
#include "src/util.h"

using namespace RoboDBG;
class WardenScanner : public Debugger {
private:
    uintptr_t wardenMemScanAddr;
    uintptr_t wardenBaseAddress;

    uintptr_t wardenVirtualAlloc = 0x0087243E; //Address where Warden module is allocated
    uintptr_t wardenInitEnd      = 0x008725DA; //Address where Warden module is written
    uintptr_t wardenModuleSize   = 0x10000;
    uintptr_t rdata              = 0x00DD1100;
public:
    virtual void onStart( uintptr_t imageBase, uintptr_t entryPoint ) {
        hideDebugger( );
        std::cout << "Setting breakpoints for Warden" << std::endl;
        setBreakpoint(wardenVirtualAlloc);
        setBreakpoint(wardenInitEnd);
        //PrintMemoryPages();
        MemoryRegion_t mr = getPageByAddress( rdata );
        changeMemoryProtection(mr, PAGE_READWRITE); //https://github.com/stoneharry/RCEPatcher
    }

    virtual BreakpointAction onBreakpoint(uintptr_t address, HANDLE hThread) {
        std::cout << "[*] Breakpoint hit at: 0x" << std::hex << address << std::endl;
        if ( address == wardenVirtualAlloc ) {
            wardenBaseAddress = static_cast<uintptr_t>(getRegister(hThread,  Register32::EAX));
            printf("[*] Warden Module Address: %x\n",wardenBaseAddress);
            return RESTORE;
        } else if ( address == wardenInitEnd ) {
            std::vector<uintptr_t> results = searchInMemory({ 0x56, 0x57, 0xFC, 0x8B, 0x54 }); //search for memscan function
            std::cout << "[*] Found (" << results.size( ) << ") Memory Scan function: " << std::hex << wardenBaseAddress << std::endl;
            for (uintptr_t addr : results) {
                if  (addr >= wardenBaseAddress &&
                    addr <= wardenBaseAddress + wardenModuleSize)
                {
                    wardenMemScanAddr = addr + 0xf; //Offset for the location to get the
                    setHardwareBreakpoint(wardenMemScanAddr, DRReg::DR1, AccessType::EXECUTE, BreakpointLength::BYTE);
                    std::cout << "[+] Warden MemScan function:  " << std::hex << addr << std::endl;
                    break;
                }
            }
            return RESTORE;
        } else {
            std::cerr << "[-] Unhandled breakpoint at " << std::hex << address << " " << std::hex << wardenMemScanAddr << std::endl;
        }
        return BREAK;
    }

    virtual BreakpointAction onHardwareBreakpoint( uintptr_t address, HANDLE hThread, DRReg reg ) {
        if (address == wardenMemScanAddr) {
            std::cout << "[HW] Warden Package found!" << std::endl;
            int edx =  getRegister(hThread, Register32::EDX);
            int esi =  getRegister(hThread, Register32::ESI);
            int eax =  getRegister(hThread, Register32::EAX);
            std::cout << "EDX: " << std::hex << edx << ", ESI: " << esi << ", EAX: " << eax << std::endl;

            std::ofstream csv("warden.csv", std::ios::app); // append mode
            if (!csv.is_open()) {
                std::cerr << "[-] Failed to open warden.csv for writing.\n";
                return RESTORE;
            }

            csv << std::hex << std::setfill('0');
            csv << std::setw(8) << edx << ";"
            << std::setw(8) << esi << ";"
            << std::setw(8) << eax << ";";

            if (edx > 0) {
                char* buf = new char[edx];
                if (readTargetMemory( static_cast<uintptr_t>(esi), buf, edx)) {
                    for (int i = 0; i < edx; ++i) {
                        csv << std::setw(2) << static_cast<unsigned>(static_cast<unsigned char>(buf[i]));
                    }
                }
                delete[] buf;
            }

            csv << "\n";
            csv.close();
            return RESTORE;
        } else {
            std::cout << "[-] Failed to find hw bp.\n";
        }
        return RESTORE;
    }
};

int main( ) {
    Util::EnableDebugPrivilege( );
    WardenScanner *ws = new WardenScanner( );
    if(ws->attachToProcess("WoW.exe") == 0)
    {
        std::cout << "attached!" <<std::endl;
    }
    ws->loop( );
    return 0;
}
