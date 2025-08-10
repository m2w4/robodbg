/**
 * @file util.h
 * @brief Various high-level Utility functions.
 **/

#ifndef HELPER_H
#define HELPER_H

#include <windows.h>
#include <string>
#include <vector>
#include <tlhelp32.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <psapi.h>
#include <map>
#include <fstream>
#include <iomanip>

#include "debugger.h"

/**
 * @namespace RoboDBG::Util
 * @brief Utility helper functions for more high-level stuff.
 */
namespace RoboDBG::Util {

    /**
     * @brief Injects a DLL into the specified process.
     *
     * Allocates memory in the remote process, writes the DLL path, and creates a remote thread
     * to load the DLL.
     *
     * @param hProcess Handle to the target process with PROCESS_ALL_ACCESS rights.
     * @param dllPath Full path to the DLL to inject.
     * @return The remote address where the DLL was loaded, or 0 on failure.
     */
    uintptr_t injectDLL(HANDLE hProcess, const char* dllPath);

    /**
     * @brief Retrieves the name of a DLL from a remote process.
     *
     * Reads the memory at `lpImageName` in the remote process and returns the name as a string.
     *
     * @param hProcess Handle to the target process.
     * @param lpImageName Address of the string in the remote process.
     * @param isUnicode TRUE if the string is in Unicode format, FALSE for ANSI.
     * @return DLL name as a std::string.
     */
    std::string getDllName(HANDLE hProcess, LPVOID lpImageName, BOOL isUnicode);

    /**
     * @brief Finds the process ID of a process by name.
     *
     * Iterates over all running processes and matches against `processName`.
     *
     * @param processName Name of the process executable (e.g., "notepad.exe").
     * @return Process ID on success, or 0 if not found.
     */
    DWORD findProcessId(const std::string& processName);

    /**
     * @brief Executes shellcode in a remote process.
     *
     * Allocates memory in the target process, writes the provided shellcode, and creates
     * a remote thread to execute it.
     *
     * @param hProcessGlobal Handle to the target process with execution rights.
     * @param shellcode Vector containing the shellcode bytes to execute.
     * @return true if the shellcode executed successfully, false otherwise.
     */
    bool executeRemote(HANDLE hProcessGlobal, const std::vector<BYTE>& shellcode);

    /**
     * @brief Gets the entry point address of a loaded module in a remote process.
     *
     * Parses the PE header of the loaded image in the target process to determine
     * the entry point address.
     *
     * @param hProcess Handle to the target process.
     * @param baseAddress Base address of the module in the target process.
     * @return Address of the entry point, or 0 on failure.
     */
    DWORD_PTR getEntryPoint(HANDLE hProcess, LPVOID baseAddress);

    /**
     * @brief Enables the SeDebugPrivilege privilege for the current process.
     *
     * This is required to debug or manipulate processes owned by other users or the system.
     *
     * @return true if the privilege was successfully enabled, false otherwise.
     */
    bool EnableDebugPrivilege();
}

#endif
