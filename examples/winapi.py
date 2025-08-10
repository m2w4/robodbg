#Example usage of robodbg in addition to ctypes to use win32 APIs

from robodbg.x64 import Debugger, BreakpointAction

import ctypes
from ctypes import wintypes

# WinAPI constants
TH32CS_SNAPTHREAD = 0x00000004

kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)

class THREADENTRY32(ctypes.Structure):
    _fields_ = [
        ("dwSize", wintypes.DWORD),
        ("cntUsage", wintypes.DWORD),
        ("th32ThreadID", wintypes.DWORD),
        ("th32OwnerProcessID", wintypes.DWORD),
        ("tpBasePri", wintypes.LONG),
        ("tpDeltaPri", wintypes.LONG),
        ("dwFlags", wintypes.DWORD),
    ]

def list_threads_of_process(hProcess):
    """List thread IDs belonging to a process HANDLE."""
    # Get PID from handle
    pid = kernel32.GetProcessId(hProcess)
    if not pid:
        raise ctypes.WinError(ctypes.get_last_error())

    # Take snapshot of all threads
    snapshot = kernel32.CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)
    if snapshot == wintypes.HANDLE(-1).value:
        raise ctypes.WinError(ctypes.get_last_error())

    try:
        te32 = THREADENTRY32()
        te32.dwSize = ctypes.sizeof(THREADENTRY32)

        threads = []
        if kernel32.Thread32First(snapshot, ctypes.byref(te32)):
            while True:
                if te32.th32OwnerProcessID == pid:
                    threads.append(te32.th32ThreadID)
                if not kernel32.Thread32Next(snapshot, ctypes.byref(te32)):
                    break
        return threads
    finally:
        kernel32.CloseHandle(snapshot)

class PyDebugger(Debugger):
    def __init__(self):
        super().__init__()
        print("[PyDebugger] Initialized")

    def onStart(self, imageBase, entryPoint):
        print(f"[onStart] Image Base: {hex(imageBase)}, Entry Point: {hex(entryPoint)}")
        capsule = wintypes.HANDLE(self.getProcessHandle( ))
        print(list_threads_of_process(capsule))

    def onEnd(self, exitCode, pid):
        print(f"[onEnd] Exit Code: {exitCode}, PID: {pid}")

    def onAttach(self):
        print("[onAttach] Attached to process")

if __name__ == "__main__":
    p = PyDebugger( )
    p.startProcess("notepad.exe")
    p.loop( )
