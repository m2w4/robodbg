from robodbg.x86 import Debugger, BreakpointAction, Register32, DRReg, AccessType, BreakpointLength

class WardenSniffer(Debugger):
    def __init__(self):
        super().__init__()
        self.wardenVirtualAlloc = 0x0087243E  #Address where Warden module is allocated
        self.wardenInitEnd      = 0x008725DA  #Address where Warden module is written
        self.wardenModuleSize   = 0x10000     #Warden Module size

    def onStart(self, imageBase, entryPoint):
        self.hideDebugger( )
        print("Setting breakpoints to find Warden Address")
        self.setBreakpoint(self.wardenVirtualAlloc)
        self.setBreakpoint(self.wardenInitEnd)

    def onBreakpoint(self, address, hThread):
        print(f"[onBreakpoint] Breakpoint hit at {hex(address)}")
        if address == self.wardenVirtualAlloc:
            self.wardenBaseAddress = self.getRegister(hThread,  Register32.EAX)
            print("[*] Warden Module Address: %x\n" % self.wardenBaseAddress)
            return BreakpointAction.RESTORE
        elif address == self.wardenInitEnd:
            results = self.searchInMemory( [0x56, 0x57, 0xFC, 0x8B, 0x54] )
            print("[*] Found (%d) Memory Scan Function: %08x" % (len(results), self.wardenBaseAddress))
            for addr in results:
                if addr >= self.wardenBaseAddress and addr <= self.wardenBaseAddress + self.wardenModuleSize:
                    self.wardenMemScanAddr = addr + 0xf #Offset for the location to get the memory scan instruction
                    self.setHardwareBreakpoint(self.wardenMemScanAddr, DRReg.DR1, AccessType.EXECUTE, BreakpointLength.BYTE)
                    print("[+] Warden MemScan function:  %08x" % self.wardenMemScanAddr)
            return BreakpointAction.RESTORE
        return BreakpointAction.BREAK

    def onHardwareBreakpoint(self, address, hThread, reg):
        #print(f"[onHardwareBreakpoint] HWBP at {hex(address)} in {reg}")
        length = self.getRegister(hThread,  Register32.EDX)
        addr   = self.getRegister(hThread,  Register32.ESI)
        memory = self.readMemory(addr, length )
        if len(memory) > 0:
            print(f"[+] Scan saved!")
            printableMem =  bytes(memory).hex()
            open("warden.csv","a").write(f"{hex(length)};{hex(addr)};{printableMem}\n")
        return BreakpointAction.RESTORE

if __name__ == "__main__":
    dbg = WardenSniffer()
    dbg.attachToProcess("Wow.exe")
    dbg.loop()

