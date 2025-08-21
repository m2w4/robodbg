from robodbg import Debugger, BreakpointAction, Register32, DRReg, AccessType, BreakpointLength

class WardenSniffer(Debugger):
    def __init__(self):
        super().__init__()
        self.warden_virtual_alloc = 0x0087243E  #Address where Warden module is allocated
        self.warden_init_end      = 0x008725DA  #Address where Warden module is written
        self.warden_module_size   = 0x10000     #Warden Module size

    def on_start(self, imageBase, entryPoint):
        self.hide_debugger( )
        print("Setting breakpoints to find Warden Address")
        self.set_breakpoint(self.warden_virtual_alloc)
        self.set_breakpoint(self.warden_init_end)

    def on_breakpoint(self, address, hThread):
        print(f"[onBreakpoint] Breakpoint hit at {hex(address)}")
        if address == self.warden_virtual_alloc:
            self.warden_base_addr = self.get_register(hThread,  Register32.EAX)
            print("[*] Warden Module Address: %x\n" % self.warden_base_addr)
            return BreakpointAction.RESTORE
        elif address == self.warden_init_end:
            results = self.search_in_memory( [0x56, 0x57, 0xFC, 0x8B, 0x54] )
            print("[*] Found (%d) Memory Scan Function: %08x" % (len(results), self.warden_base_addr))
            for addr in results:
                if addr >= self.warden_base_addr and addr <= self.warden_base_addr + self.warden_module_size:
                    self.warden_memscan_Addr = addr + 0xf #Offset for the location to get the memory scan instruction
                    self.set_hardware_breakpoint(self.warden_memscan_Addr, DRReg.DR1, AccessType.EXECUTE, BreakpointLength.BYTE)
                    print("[+] Warden MemScan function:  %08x" % self.warden_memscan_Addr)
            return BreakpointAction.RESTORE
        return BreakpointAction.BREAK

    def on_hardware_breakpoint(self, address, hThread, reg):
        #print(f"[onHardwareBreakpoint] HWBP at {hex(address)} in {reg}")
        length = self.get_register(hThread,  Register32.EDX)
        addr   = self.get_register(hThread,  Register32.ESI)
        memory = self.read_memory(addr, length )
        if len(memory) > 0:
            print(f"[+] Scan saved!")
            printableMem =  bytes(memory).hex()
            open("warden.csv","a").write(f"{hex(length)};{hex(addr)};{printableMem}\n")
        return BreakpointAction.RESTORE

if __name__ == "__main__":
    dbg = WardenSniffer()
    dbg.attach("Wow.exe")
    dbg.loop()

