# Example usage of robodbg of unpacking a UPX-packed CrackMe
# Get the binary here: https://crackmes.one/crackme/6861509baadb6eeafb398de7
from robodbg import Debugger, BreakpointAction, Flags64, Register64


class CrackMeSolver(Debugger):
    def on_start(self, imageBase, entryPoint):
        self.username = None
        self.password = None

        # # Break shortly before VirtualAlloc
        # At this stage we can read the unpacked code and get it's base
        # by reading the RSI register
        self.virtualProtect = self.aslr(0x17511)

        # # Bypass isDebuggerPresent by chaning the ZF Flag to True
        # call isDebuggerPresent
        # test eax, eax
        # >JE noDebuggerPresent
        # Hint: You can also use self.hideDebugger( ) to bypass PEB-based Debugger detection
        self.is_debugger_present_check = self.aslr(0x181B)

        # # strcmp instruction to check the username against a valid one
        self.usernameCheck = self.aslr(0x1B12)

        # # strcmp instruction to check the password against a valid one
        self.passwordCheck = self.aslr(0x1AD9)

        # # JE instruction which checks the login credentials
        # >JE checkLogin
        self.loginCheck = self.aslr(0x19F6)

        print("Setting breakpoint after the unpacking is complete")
        self.set_breakpoint(self.virtualProtect)

    def on_breakpoint(self, address, hThread):
        print(f"[on_breakpoint] Breakpoint hit at {hex(address)}")
        if address == self.virtualProtect:
            self.set_breakpoint(self.is_debugger_present_check)
            self.set_breakpoint(self.usernameCheck)
            self.set_breakpoint(self.passwordCheck)
            self.set_breakpoint(self.loginCheck)
        elif address == self.is_debugger_present_check:
            self.set_flag(hThread, Flags64.ZF, True)

        elif address == self.usernameCheck:
            usernameAddr = self.get_register(hThread, Register64.RDX)  # Read the address of the username from RDX
            self.username = self.read_cstring(usernameAddr)

        elif address == self.passwordCheck:
            passwordAddr = self.get_register( hThread, Register64.RAX)  # Read the address of the username from RAX
            self.password = self.read_cstring(passwordAddr)

        elif address == self.loginCheck:
            self.set_flag(hThread, Flags64.ZF, True)
        return BreakpointAction.BREAK

    def read_cstring(self, addr):
        result_bytes = []
        offset = 0
        while True:
            b = self.read_memory(addr + offset, 1)[0]  # read 1 byte
            if b == 0:
                break
            result_bytes.append(b)
            offset += 1
        return "".join(chr(i) for i in result_bytes)


if __name__ == "__main__":
    dbg = CrackMeSolver()
    dbg.start(
        "CrackMe_By_InDuLgEo_V1.exe"
    )  # Get it from here: https://crackmes.one/crackme/6861509baadb6eeafb398de7
    dbg.loop()
    print("Application finished")
    print("Username: " + dbg.username)
    print("Password: " + dbg.password)
