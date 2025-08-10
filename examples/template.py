from robodbg.x64 import Debugger, BreakpointAction

class PyDebugger(Debugger):
    def __init__(self):
        super().__init__()
        print("[PyDebugger] Initialized")

    def onStart(self, imageBase, entryPoint):
        print(f"[onStart] Image Base: {hex(imageBase)}, Entry Point: {hex(entryPoint)}")

    def onEnd(self, exitCode, pid):
        print(f"[onEnd] Exit Code: {exitCode}, PID: {pid}")

    def onAttach(self):
        print("[onAttach] Attached to process")

    def onThreadCreate(self, hThread, threadId, threadBase, startAddress):
        print(f"[onThreadCreate] Thread ID: {threadId}, Base: {hex(threadBase)}, Start: {hex(startAddress)}")

    def onThreadExit(self, threadID):
        print(f"[onThreadExit] Thread ID: {threadID}")

    def onDLLLoad(self, address, dllName, entryPoint):
        print(f"[onDLLLoad] {dllName} at {hex(address)} with EP: {hex(entryPoint)}")
        return True

    def onDLLUnload(self, address, dllName):
        print(f"[onDLLUnload] {dllName} from {hex(address)}")

    def onBreakpoint(self, address, hThread):
        print(f"[onBreakpoint] Breakpoint hit at {hex(address)}")
        return BreakpointAction.BREAK

    def onHardwareBreakpoint(self, address, hThread, reg):
        print(f"[onHardwareBreakpoint] HWBP at {hex(address)} in {reg}")
        return BreakpointAction.BREAK

    def onSinglestep(self, address, hThread):
        print(f"[onSinglestep] Address: {hex(address)}")

    def onDebugString(self, dbgString):
        print(f"[onDebugString] {dbgString}")

    def onAccessViolation(self, address, faultingAddress, accessType):
        print(f"[onAccessViolation] At {hex(address)} -> Faulting {hex(faultingAddress)}, Type: {accessType}")

    def onRIPError(self, rip):
        print(f"[onRIPError] {rip}")

    def onUnknownException(self, addr, code):
        print(f"[onUnknownException] At {hex(addr)}, Code: {hex(code)}")

    def onUnknownDebugEvent(self, code):
        print(f"[onUnknownDebugEvent] Code: {hex(code)}")

# === Example Usage ===
if __name__ == "__main__":
    dbg = PyDebugger()
    dbg.attachToProcess("example.exe")
    dbg.loop()
