# RoboDBG 🤖 Engine

**RoboDBG** is a work-in-progress library for building **Windows user-mode debuggers** in both **C++** and **Python**.
This library is **very experimental** - Expect **bugs** and **missing features**.

## ✨ Features

- 🖥 **Architecture Support:** Works with x32 and x64 applications with and without ASLR
- 🎯 **Breakpoints:** Implemented Breakpoints and Hardware Breakpoints
- 📦 **Memory Control:** Read, write, and modify memory protection flags.
- 🐍 **Python Integration:** Full Python API bindings support

## 📦 Installation

### C++  
Download the latest `.lib` and `.h` files from the [Releases](https://github.com/m2w4/robodbg/releases) page.

### Python  
Install directly from GitHub using `pip`:
```bash
pip install https://github.com/m2w4/robodbg/releases/download/v0.0.1/py_robodbg-0.0.1.tar.gz
```

## 🧑‍💻 Getting Started

To get you started with a tutorial, go to docs/tutorial.md.
The doxygen documentation is available at <a href="https://www.robodbg.com">robodbg.com</a>.

### Usage

```py
from robodbg.x86 import Debugger, BreakpointAction, Register32, DRReg, AccessType, BreakpointLength

class MyDebugger(Debugger):
    def onStart(self, imageBase, entryPoint):
        self.setBreakpoint(0x00401000)
        
    def onBreakpoint(self, address, hThread):
        print(f"[onBreakpoint] Breakpoint hit at {hex(address)}")
        return BreakpointAction.BREAK
```

```cpp
#include <stdio.h>
#include <iostream>
#include <robodbg/debugger.h>

using namespace RoboDBG;
class MyDebugger : public Debugger {
public:
    virtual void onStart( uintptr_t imageBase, uintptr_t entryPoint ) {
        setBreakpoint(0x00401000);
    }

    virtual BreakpointAction onBreakpoint(uintptr_t address, HANDLE hThread) {
        std::cout << "[*] Breakpoint hit at: 0x" << std::hex << address << std::endl;
    }
};
```

### Examples (Python)

[Debugger Template](examples/template.py)

[UPX CrackMe Solver (~75 Lines)](examples/crackMe.py)

[Warden Sniffer for WotLK 3.3.5a (~50 Lines)](examples/wardenScanWotlk.py)

### Examples (C++)

[Debugger Template](examples/template.cpp)

[Warden Sniffer for WotLK 3.3.5a (~80 Lines)](examples/wardenScanWotlk.cpp)

[Warden Dumper  for MoP   5.4.8 (~100 Lines)](examples/wardenScanWotlk.cpp)

### Compile

See [BUILDING.md](BUILDING.md) file for how to compile and install roboDBG.

### License

This project is released under the MIT license. If you redistribute the binary
or source code of RoboDBG, please attach file LICENSE.TXT with your products.
