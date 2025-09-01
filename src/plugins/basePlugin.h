#ifndef BASEPLUGIN_H
#define BASEPLUGIN_H

#include <windows.h>
#include <iostream>

class BasePlugin {
protected:
    HANDLE hProcess_ = nullptr;

    /**
     * @brief Writes raw bytes to target memory.
     * @param address Destination address in target.
     * @param buffer Source buffer.
     * @param size Number of bytes to write.
     * @return true on success; false otherwise.
     */
    inline bool writeMemory(LPVOID address, const void* buffer, SIZE_T size)
    {
        SIZE_T bytesWritten = 0;
        if (!WriteProcessMemory(hProcess_, address, buffer, size, &bytesWritten) || bytesWritten != size) {
            std::cerr << "WriteProcessMemory failed: " << GetLastError() << std::endl;
            return false;
        }
        FlushInstructionCache(hProcess_, address, size);
        return true;
    }

    /**
     * @brief Reads raw bytes from target memory.
     * @param address Source address in target.
     * @param buffer Destination buffer.
     * @param size Number of bytes to read.
     * @return true on success; false otherwise.
     */
    inline bool readMemory(LPVOID address, void* buffer, SIZE_T size)
    {
        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(hProcess_, address, buffer, size, &bytesRead) || bytesRead != size) {
            std::cerr << "ReadProcessMemory failed: " << GetLastError() << std::endl;
            return false;
        }
        return true;
    }

    /**
     * @brief Writes raw bytes to target memory (uintptr_t overload).
     * @param address Destination runtime address.
     * @param buffer Source buffer.
     * @param size Number of bytes to write.
     * @return true on success; false otherwise.
     */
    inline bool writeMemory(uintptr_t address, const void* buffer, SIZE_T size) {
        return writeMemory(reinterpret_cast<LPVOID>(address), buffer, size);
    }

    /**
     * @brief Reads raw bytes from target memory (uintptr_t overload).
     * @param address Source runtime address.
     * @param buffer Destination buffer.
     * @param size Number of bytes to read.
     * @return true on success; false otherwise.
     */
    inline bool readMemory(uintptr_t address, void* buffer, SIZE_T size) {
        return readMemory(reinterpret_cast<LPVOID>(address), buffer, size);
    }

    /**
     * @brief Writes a POD value to target memory (typed helper).
     * @tparam T Trivially copyable type.
     * @param address Destination runtime address.
     * @param value Value to write.
     * @return true on success; false otherwise.
     */
    template<typename T>
    bool writeMemory(uintptr_t address, const T& value) {
        SIZE_T bytesWritten = 0;
        if (!WriteProcessMemory(hProcess_, reinterpret_cast<LPVOID>(address), &value, sizeof(T), &bytesWritten) || bytesWritten != sizeof(T)) {
            return false;
        }
        FlushInstructionCache(hProcess_, reinterpret_cast<LPVOID>(address), sizeof(T));
        return true;
    }

    /**
     * @brief Reads a POD value from target memory (typed helper).
     * @tparam T Trivially copyable type.
     * @param address Source runtime address.
     * @return Read value; default-initialized T on failure.
     */
    template<typename T>
    T readMemory(uintptr_t address) {
        T value{};
        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(hProcess_, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), &bytesRead) || bytesRead != sizeof(T)) {
            return T{};
        }
        return value;
    }

};

#endif
