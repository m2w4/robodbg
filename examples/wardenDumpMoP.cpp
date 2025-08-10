// RoboDBG example for dumping the Warden module from MoP 5.4.8

#include <stdio.h>
#include <iostream>
#include <autodbg/debugger.h>
#include <autodbg/util.h>

using namespace RoboDBG;

bool saveModule(const std::vector<BYTE>& buffer, const std::string& filename) {
    if (buffer.empty()) {
        std::cerr << "saveModule: buffer is empty, nothing to write.\n";
        return false;
    }

    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        std::cerr << "saveModule: failed to open " << filename << " for writing.\n";
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    if (!outFile) {
        std::cerr << "saveModule: error occurred while writing to file.\n";
        return false;
    }

    std::cout << "saveModule: wrote " << buffer.size() << " bytes to " << filename << "\n";
    return true;
}

class WardenDumper : public Debugger {
private:
    uintptr_t   wardenAlloc  = 0xa711f3; //Break at instruction after VirtualAlloc to get the address of Warden
    uintptr_t   wardenRead   = 0xa71534; //Break at the end of the Warden Initialize function and the module is written to memory
    std::size_t wardenSize   = 0x7000;
    uintptr_t wardenBaseAddress;
public:

    //Gets called when the debugger is attached
    virtual void onStart( uintptr_t imageBase, uintptr_t entryPoint ) {
        hideDebugger( );
        std::cout << "[*] Setting breakpoints for dumping Warden" << std::endl;
        setBreakpoint( ASLR(wardenAlloc) ); //ASLR adds the imageBase to the address.
        setBreakpoint( ASLR(wardenRead ) );
    }

    virtual BreakpointAction onBreakpoint(uintptr_t address, HANDLE hThread) {
        std::cout << "[*] Breakpoint hit at: 0x" << std::hex << address << std::endl;
        if( address == ASLR(wardenAlloc) ) {
            wardenBaseAddress = getRegister(hThread,  Register64::RAX);
            return RESTORE;
        } else if ( address == ASLR(wardenRead)) {
            std::vector<uintptr_t> results = searchInMemory({ 0x42, 0x4C, 0x4C, 0x32}); //Search for Warden Module header "BLL2"
            std::cout << "[*] Pattern found at addresses (" << results.size( ) << "):" << std::endl;
            for (uintptr_t addr : results) {
                if (addr >= wardenBaseAddress &&
                    addr <= wardenBaseAddress + wardenSize)
                {
                    std::cout << "[*] Warden Module Address:  " << std::hex << addr << std::endl;
                    std::vector<BYTE> module(wardenSize);
                    if( readTargetMemory(addr, module.data( ), module.size( )) )
                    {
                        saveModule(module, "warden.bin");
                        return BREAK;
                    }
                    break;
                }
            }
            return RESTORE;
        } else {
            std::err << "[*] Unhandled breakpoint at " << std::hex << address << std::endl;
        }
        return BREAK;
    }
};

int main( ) {
    EnableDebugPrivilege( );
    WardenDumper *ws = new WardenDumper( );
    if(ws->attachToProcess("WoW.exe") == 0)
    {
        std::cout << "Attached to application!" <<std::endl;
    }
    ws->loop( );
    return 0;
}
