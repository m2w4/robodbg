from robodbg import Debugger, BreakpointAction

class PyDebugger(Debugger):
    def __init__(self):
        super().__init__()
        print("[PyDebugger] Initialized")

    def on_start(self, image_base, entry_point):
        print(f"[on_start] Image Base: {hex(image_base)}, Entry Point: {hex(entry_point)}")

    def on_end(self, exit_code, pid):
        print(f"[on_end] Exit Code: {exit_code}, PID: {pid}")

    def on_attach(self):
        print("[on_attach] Attached to process")

    def on_thread_create(self, h_thread, thread_id, thread_base, start_address):
        print(f"[on_thread_create] Thread ID: {thread_id}, Base: {hex(thread_base)}, Start: {hex(start_address)}")

    def on_thread_exit(self, thread_id):
        print(f"[on_thread_exit] Thread ID: {thread_id}")

    def on_dll_load(self, address, dll_name, entry_point):
        print(f"[on_dll_load] {dll_name} at {hex(address)} with EP: {hex(entry_point)}")
        return True

    def on_dll_unload(self, address, dll_name):
        print(f"[on_dll_unload] {dll_name} from {hex(address)}")

    def on_breakpoint(self, address, h_thread):
        print(f"[on_breakpoint] Breakpoint hit at {hex(address)}")
        return BreakpointAction.BREAK

    def on_hardware_breakpoint(self, address, h_thread, reg):
        print(f"[on_hardware_breakpoint] HWBP at {hex(address)} in {reg}")
        return BreakpointAction.BREAK

    def on_single_step(self, address, h_thread):
        print(f"[on_single_step] Address: {hex(address)}")

    def on_debug_string(self, dbg_string):
        print(f"[on_debug_string] {dbg_string}")

    def on_access_violation(self, address, faulting_address, access_type):
        print(f"[on_access_violation] At {hex(address)} -> Faulting {hex(faulting_address)}, Type: {access_type}")

    def on_rip_error(self, rip):
        print(f"[on_rip_error] {rip}")

    def on_unknown_exception(self, addr, code):
        print(f"[on_unknown_exception] At {hex(addr)}, Code: {hex(code)}")

    def on_unknown_debug_event(self, code):
        print(f"[on_unknown_debug_event] Code: {hex(code)}")
