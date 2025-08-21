from enum import IntFlag
from .dbg import Debugger as BaseDebugger, BreakpointAction

#Memory Contants from https://learn.microsoft.com/en-us/windows/win32/memory/memory-protection-constants
class PageProtection(IntFlag):
    # Basic protection flags
    NOACCESS             = 0x01
    READONLY             = 0x02
    READWRITE            = 0x04
    WRITECOPY            = 0x08
    EXECUTE              = 0x10
    EXECUTE_READ         = 0x20
    EXECUTE_READWRITE    = 0x40
    EXECUTE_WRITECOPY    = 0x80

    # Modifiers
    GUARD                = 0x100
    NOCACHE              = 0x200
    WRITECOMBINE         = 0x400

    # Control Flow Guard (CFG)
    TARGETS_INVALID      = 0x40000000
    TARGETS_NO_UPDATE    = 0x40000000  # Alias, same bitmask

    # Enclave-specific (Intel SGX)
    ENCLAVE_DECOMMIT     = 0x20000000
    ENCLAVE_THREAD_CTRL  = 0x80000000
    ENCLAVE_UNVALIDATED  = 0x10000000


class Debugger(BaseDebugger):
    def __init__(self, _verbose=False):
        super().__init__()
        self.verbose = _verbose
        if self.verbose:
            print("[PyDebugger] Initialized")

    def on_start(self, image_base, entry_point):
        if self.verbose:
            print(
                f"[on_start] Image Base: {hex(image_base)}, Entry Point: {hex(entry_point)}"
            )

    def on_end(self, exit_code, pid):
        if self.verbose:
            print(f"[on_end] Exit Code: {exit_code}, PID: {pid}")

    def on_attach(self):
        if self.verbose:
            print("[on_attach] Attached to process")

    def on_thread_create(self, h_thread, thread_id, thread_base, start_address):
        if self.verbose:
            print(
                f"[on_thread_create] Thread ID: {thread_id}, Base: {hex(thread_base)}, Start: {hex(start_address)}"
            )

    def on_thread_exit(self, thread_id):
        if self.verbose:
            print(f"[on_thread_exit] Thread ID: {thread_id}")

    def on_dll_load(self, address, dll_name, entry_point):
        if self.verbose:
            print(
                f"[on_dll_load] {dll_name} at {hex(address)} with EP: {hex(entry_point)}"
            )
        return True

    def on_dll_unload(self, address, dll_name):
        if self.verbose:
            print(f"[on_dll_unload] {dll_name} from {hex(address)}")

    def on_breakpoint(self, address, h_thread):
        if self.verbose:
            print(f"[on_breakpoint] Breakpoint hit at {hex(address)}")
        return BreakpointAction.BREAK

    def on_hardware_breakpoint(self, address, h_thread, reg):
        if self.verbose:
            print(f"[on_hardware_breakpoint] HWBP at {hex(address)} in {reg}")
        return BreakpointAction.BREAK

    def on_single_step(self, address, h_thread):
        if self.verbose:
            print(f"[on_single_step] Address: {hex(address)}")

    def on_debug_string(self, dbg_string):
        if self.verbose:
            print(f"[on_debug_string] {dbg_string}")

    def on_access_violation(self, address, faulting_address, access_type):
        if self.verbose:
            print(
                f"[on_access_violation] At {hex(address)} -> Faulting {hex(faulting_address)}, Type: {access_type}"
            )

    def on_rip_error(self, rip):
        if self.verbose:
            print(f"[on_rip_error] {rip}")

    def on_unknown_exception(self, addr, code):
        if self.verbose:
            print(f"[on_unknown_exception] At {hex(addr)}, Code: {hex(code)}")

    def on_unknown_debug_event(self, code):
        if self.verbose:
            print(f"[on_unknown_debug_event] Code: {hex(code)}")
