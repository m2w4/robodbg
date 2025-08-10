#include "util.h"


namespace RoboDBG::Util {
    uintptr_t injectDLL(HANDLE hProcess, const char* dllPath) {
        // Get handle to target process
        //HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (hProcess == NULL) {
            std::cerr << "OpenProcess failed: " << GetLastError() << std::endl;
            return 0;
        }

        // Calculate required memory size
        size_t pathLen = strlen(dllPath) + 1;

        // Allocate memory in target process for DLL path
        LPVOID pDllPath = VirtualAllocEx(hProcess, NULL, pathLen, MEM_COMMIT, PAGE_READWRITE);
        if (pDllPath == NULL) {
            std::cerr << "VirtualAllocEx failed: " << GetLastError() << std::endl;
            CloseHandle(hProcess);
            return 0;
        }
        std::cerr << "DLL Base: " << std::hex << pDllPath << std::endl;
        // Write DLL path to target process memory
        if (!WriteProcessMemory(hProcess, pDllPath, (LPVOID)dllPath, pathLen, NULL)) {
            std::cerr << "WriteProcessMemory failed: " << GetLastError() << std::endl;
            VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return 0;
        }

        // Get address of LoadLibraryA in kernel32.dll
        LPVOID pLoadLibrary = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
        if (pLoadLibrary == NULL) {
            std::cerr << "GetProcAddress failed: " << GetLastError() << std::endl;
            VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return 0;
        }
        std::cerr << "Loaded!" << std::endl;
        // Create remote thread to load our DLL
        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
                                            (LPTHREAD_START_ROUTINE)pLoadLibrary,
                                            pDllPath, 0, NULL);
        if (hThread == NULL) {
            std::cerr << "CreateRemoteThread failed: " << GetLastError() << std::endl;
            VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return 0;
        }
        std::cerr << "Thread created!" << std::endl;
        return reinterpret_cast<uintptr_t>(pDllPath);
    }

    std::string getDllName(HANDLE hProcess, LPVOID lpImageName, BOOL isUnicode) {
        if (!lpImageName) return "<unknown>";
        LPVOID actualStringPtr = nullptr;
        SIZE_T bytesRead;

        if (!ReadProcessMemory(hProcess, lpImageName, &actualStringPtr, sizeof(LPVOID), &bytesRead)) {
            return "<read error>";
        }

        if (!actualStringPtr) {
            return "<null pointer>";
        }

        std::string name;

        if (isUnicode) {
            wchar_t wbuffer[MAX_PATH] = {};
            if (ReadProcessMemory(hProcess, actualStringPtr, &wbuffer, sizeof(wbuffer), &bytesRead)) {
                char buffer[MAX_PATH];
                WideCharToMultiByte(CP_ACP, 0, wbuffer, -1, buffer, MAX_PATH, nullptr, nullptr);
                name = buffer;
            } else {
                name = "<unicode read error>";
            }
        } else {
            char buffer[MAX_PATH] = {};
            if (ReadProcessMemory(hProcess, actualStringPtr, &buffer, sizeof(buffer), &bytesRead)) {
                name = buffer;
            } else {
                name = "<ansi read error>";
            }
        }

        return name;
    }



    bool executeRemote(HANDLE hProcessGlobal, const std::vector<BYTE>& shellcode) {
        if (shellcode.empty()) return false;

        SIZE_T codeSize = shellcode.size();

        // 1. Allocate memory in remote process (RWX)
        LPVOID remoteMem = VirtualAllocEx(hProcessGlobal, nullptr, codeSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!remoteMem) return false;

        // 2. Write shellcode
        if (!WriteProcessMemory(hProcessGlobal, remoteMem, shellcode.data(), codeSize, nullptr)) {
            VirtualFreeEx(hProcessGlobal, remoteMem, 0, MEM_RELEASE);
            return false;
        }

        // 3. Create remote thread
        HANDLE hThread = CreateRemoteThread(hProcessGlobal, nullptr, 0,
                                            (LPTHREAD_START_ROUTINE)remoteMem,
                                            nullptr, 0, nullptr);
        if (!hThread) {
            VirtualFreeEx(hProcessGlobal, remoteMem, 0, MEM_RELEASE);
            return false;
        }


        return true;
    }

    // Helper function to find process ID by name
    DWORD findProcessId(const std::string& processName) {
        PROCESSENTRY32 processInfo;
        processInfo.dwSize = sizeof(processInfo);
        HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
        if (processesSnapshot == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            std::cerr << "CreateToolhelp32Snapshot failed with error: " << error << std::endl;
            return 0; //Why invalid handle?)
        }

        Process32First(processesSnapshot, &processInfo);
        if (!processName.compare(processInfo.szExeFile)) {
            CloseHandle(processesSnapshot);
            return processInfo.th32ProcessID;
        }

        while (Process32Next(processesSnapshot, &processInfo)) {
            if (!processName.compare(processInfo.szExeFile)) {
                CloseHandle(processesSnapshot);
                return processInfo.th32ProcessID;
            }
        }

        CloseHandle(processesSnapshot);
        return 0;
    }

    bool EnableDebugPrivilege() {
        HANDLE hToken;
        TOKEN_PRIVILEGES tp;
        LUID luid;

        // Step 1: Open the current process token
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
            std::cerr << "[-] OpenProcessToken failed: " << GetLastError() << "\n";
            return false;
        }

        // Step 2: Get the LUID for SeDebugPrivilege
        if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
            std::cerr << "[-] LookupPrivilegeValue failed: " << GetLastError() << "\n";
            CloseHandle(hToken);
            return false;
        }

        // Step 3: Set up the TOKEN_PRIVILEGES structure
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        // Step 4: Adjust token privileges to enable SeDebugPrivilege
        if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr)) {
            std::cerr << "[-] AdjustTokenPrivileges failed: " << GetLastError() << "\n";
            CloseHandle(hToken);
            return false;
        }

        if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
            std::cerr << "[-] The token does not have the specified privilege.\n";
            CloseHandle(hToken);
            return false;
        }

        CloseHandle(hToken);
        return true;
    }

    // Helper function to get entry point from PE headers
    DWORD_PTR getEntryPoint(HANDLE hProcess, LPVOID baseAddress) {
        IMAGE_DOS_HEADER dosHeader;
        IMAGE_NT_HEADERS ntHeaders;

        SIZE_T bytesRead;

        // Read DOS header
        if (!ReadProcessMemory(hProcess, baseAddress, &dosHeader, sizeof(dosHeader), &bytesRead) ||
            bytesRead != sizeof(dosHeader) || dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
            return 0;
        }

        // Read NT headers
        LPVOID ntHeadersAddr = (LPBYTE)baseAddress + dosHeader.e_lfanew;
        if (!ReadProcessMemory(hProcess, ntHeadersAddr, &ntHeaders, sizeof(ntHeaders), &bytesRead) ||
            bytesRead != sizeof(ntHeaders) || ntHeaders.Signature != IMAGE_NT_SIGNATURE) {
            return 0;
        }

            // Calculate entry point address
            DWORD_PTR entryPoint = (DWORD_PTR)baseAddress + ntHeaders.OptionalHeader.AddressOfEntryPoint;
        return entryPoint;
    }
}
