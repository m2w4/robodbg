#Quickstart

## Installation

### C++  
Download the latest `.lib` and `.h` files from the [Releases](https://github.com/m2w4/robodbg/releases) page.

### Python  
Install directly from GitHub using `pip`:
```bash
pip install robodbg
```

A doxygen documentation with all functions is available under <a href="https://www.robodbg.com">robodbg.com</a>.

## Usage

### Setting breakpoints

```py
from robodbg import Debugger

class MyDebugger(Debugger):
    def on_start(self, image_base, entry_point):
        self.set_breakpoint(0x00401000)

    def on_breakpoint(self, address, hThread):
        print(f"[on_breakpoint] Breakpoint hit at {hex(address)}")

if __name__ == "__main__":
    dbg = MyDebugger( )
    dbg.attach("application.exe")
    dbg.loop()
```

### Setting Hardware Breakpoints

```py
from robodbg import Debugger, BreakpointAction, Register32, DRReg, AccessType, BreakpointLength

class MyDebugger(Debugger):
    def onStart(self, image_base, entry_point):
        self.hide_debugger( )
        print("[OnStart] Setting hardware breakpoint")
        self.set_hardware_breakpoint(0x00401000, DRReg.DR1, AccessType.EXECUTE, BreakpointLength.BYTE)

    def on_hardware_breakpoint(self, address, hThread, reg):
        print(f"[onHardwareBreakpoint] HWBP at {hex(address)} in {reg}")
        
# Setting Hardware Breakpoints
        
if __name__ == "__main__":
    dbg = MyDebugger()
    dbg.attach("application.exe")
    dbg.loop()
```

### Registers & Flags

```py
eax = self.get_register(hThread,  Register32.EAX)
eax += 1
self.set_register(hThread, Register32.EAX)
```

```py
self.set_flag(hThread, Flags64.ZF, True)
```

### Memory

#### Search in memory
```py
hits = self.search_in_memory([0x56, 0x57, 0xFC, 0x8B, 0x54])
print(f"Found {len(hits)} hits")
for a in hits[:10]:
    print(f"  0x{a:08X}")
```

#### Read Memory
```py
hits = self.read_memory(0x00401000, 4) #read 4 Byte
```


#### Write Memory
```py
hits = self.write_memory(0x00401000, "\x90\x90\x90\xCC", 4) #write 4 Byte
```
