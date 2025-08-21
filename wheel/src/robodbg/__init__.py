import platform
from .debugger import Debugger, PageProtection

from .dbg import (
    BreakpointAction,
    AccessType,
    BreakpointLength,
    ThreadInfo,
    HardwareBreakpoint,
    MemoryRegion,
    DRReg
)

imports = [
    "BreakpointAction",
    "BreakpointLength",
    "ThreadInfo",
    "MemoryRegion",
    "Debugger",
    "DRReg",
    "PageProtection"
]


if platform.architecture()[0] == "64bit":
    from .dbg import Register64, Flags64

    imports.append("Flags64")
    imports.append("Register64")

elif platform.architecture()[0] == "32bit":
    from .dbg import Register32, Flags32

    imports.append("Flags32")
    imports.append("Register32")

__all__ = imports

