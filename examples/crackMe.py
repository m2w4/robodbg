# Example usage of robodbg of unpacking a UPX-packed CrackMe
# Get the binary here: https://crackmes.one/crackme/6861509baadb6eeafb398de7
from robodbg.x64 import *

class CrackMeSolver(Debugger):
    def onStart(self, imageBase, entryPoint):
        self.username = None
        self.password = None

        # # Break shortly before VirtualAlloc
        # At this stage we can read the unpacked code and get it's base
        # by reading the RSI register
        self.virtualProtect = self.ASLR( 0x17511 )

        # # Bypass isDebuggerPresent by chaning the ZF Flag to True
        # call isDebuggerPresent
        # test eax, eax
        #>JE noDebuggerPresent
        #Hint: You can also use self.hideDebugger( ) to bypass PEB-based Debugger detection
        self.isDebuggerPresentCheck = self.ASLR( 0x181B )

        # # strcmp instruction to check the username against a valid one
        self.usernameCheck = self.ASLR( 0x1B12 )

        # # strcmp instruction to check the password against a valid one
        self.passwordCheck = self.ASLR( 0x1AD9 )

        # # JE instruction which checks the login credentials
        #>JE checkLogin
        self.loginCheck    = self.ASLR( 0x19F6 )

        print("Setting breakpoint after the unpacking is complete")
        self.setBreakpoint(self.virtualProtect)


    def onBreakpoint(self, address, hThread):
        print(f"[onBreakpoint] Breakpoint hit at {hex(address)}")
        if address == self.virtualProtect:
            self.setBreakpoint(self.isDebuggerPresentCheck)
            self.setBreakpoint(self.usernameCheck)
            self.setBreakpoint(self.passwordCheck)
            self.setBreakpoint(self.loginCheck)
        elif address == self.isDebuggerPresentCheck:
            self.setFlag(hThread, Flags64.ZF, True)

        elif address == self.usernameCheck:
            usernameAddr = self.getRegister(hThread, Register64.RDX) #Read the address of the username from RDX
            self.username = self.read_cstring(usernameAddr)

        elif address == self.passwordCheck:
            passwordAddr = self.getRegister(hThread, Register64.RAX) #Read the address of the username from RAX
            self.password = self.read_cstring(passwordAddr)

        elif address == self.loginCheck:
            self.setFlag(hThread, Flags64.ZF, True)
        return BreakpointAction.BREAK

    def read_cstring(self, addr):
        result_bytes = []
        offset = 0
        while True:
            b = self.readMemory(addr + offset, 1)[0]  # read 1 byte
            if b == 0:
                break
            result_bytes.append(b)
            offset += 1
        return ''.join(chr(i) for i in result_bytes)

if __name__ == "__main__":
    dbg = CrackMeSolver()
    dbg.startProcess("CrackMe_By_InDuLgEo_V1.exe") #Get it from here: https://crackmes.one/crackme/6861509baadb6eeafb398de7
    dbg.loop()
    print("Application finished")
    print("Username: " + dbg.username)
    print("Password: " + dbg.password)
