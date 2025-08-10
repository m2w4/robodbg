#Quickstart

## Installation

### C++  
Download the latest `.lib` and `.h` files from the [Releases](https://github.com/m2w4/robodbg/releases) page.

### Python  
Install directly from GitHub using `pip`:
```bash
pip install https://github.com/m2w4/robodbg/releases/download/v0.0.1/robodbg_0.0.1.tar.gz
```

A doxygen documentation with all protected and private functions is available under <a href="https://www.robodbg.com">robodbg.com</a>.

## Usage

### Setting breakpoints

```py
from robodbg.x64 import Debugger, BreakpointAction, Register32, DRReg, AccessType, BreakpointLength

class MyDebugger(Debugger):
    def onStart(self, imageBase, entryPoint):
        self.setBreakpoint(0x00401000)

    def onBreakpoint(self, address, hThread):
        print(f"[onBreakpoint] Breakpoint hit at {hex(address)}")

if __name__ == "__main__":
    dbg = MyDebugger( )
    dbg.attachToProcess("application.exe")
    dbg.loop()
```

### Setting Hardware Breakpoints

```py
from robodbg.x64 import Debugger, BreakpointAction, Register32, DRReg, AccessType, BreakpointLength

class MyDebugger(Debugger):
    def onStart(self, imageBase, entryPoint):
        self.hideDebugger( )
        print("[OnStart] Setting hardware breakpoint")
        self.setHardwareBreakpoint(0x00401000, DRReg.DR1, AccessType.EXECUTE, BreakpointLength.BYTE)

    def onHardwareBreakpoint(self, address, hThread, reg):
        print(f"[onHardwareBreakpoint] HWBP at {hex(address)} in {reg}")
        
# Setting Hardware Breakpoints
        
if __name__ == "__main__":
    dbg = MyDebugger()
    dbg.attachToProcess("application.exe")
    dbg.loop()
```

### Registers & Flags

```py
eax = self.getRegister(hThread,  Register32.EAX)
eax += 1
self.setRegister(hThread, Register32.EAX)
```

```py
self.setFlag(hThread, Flags64.ZF, True)
```
