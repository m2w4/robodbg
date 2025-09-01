/**
 * @file freezer.h
 * @brief Plugin to freeze and restore an application by suspending and un-suspending all threads
 * @author Milkshake
 */
// Build example (with a separate main): cl /std:c++20 /EHsc main.cpp imports.cpp /link Psapi.lib
#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <optional>



// RAII handle wrapper
struct Handle {
    HANDLE h{ nullptr };
    Handle() = default;
    explicit Handle(HANDLE x) : h(x) {}
    ~Handle() { if (h) ::CloseHandle(h); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    Handle(Handle&& o) noexcept : h(o.h) { o.h = nullptr; }
    Handle& operator=(Handle&& o) noexcept {
        if (this != &o) {
            if (h) ::CloseHandle(h);
            h = o.h; o.h = nullptr;
        }
        return *this;
    }
    operator bool() const { return h != nullptr; }
};

// State per thread
struct ThreadState {
    DWORD tid{};
    int   dynPriority{};
    BOOL  boostDisabled{};
    GROUP_AFFINITY groupAffinity{};
    DWORD prevSuspendCount{};
    bool  weSuspended{};
};

// Main class
class Freezer {
public:
    explicit Freezer(HANDLE process);

    /**
     * @brief suspends every thread in a process
     * @return returns the initial states before suspending
     */
    std::optional<std::vector<ThreadState>> suspend() const;

    /**
     * @brief suspends every thread in a process
     * @return returns the initial states before suspending
     */
    bool restore(const std::vector<ThreadState>& states) const;

    /**
     * @brief suspends every thread in a process
     * @return returns the initial states before suspending
     */
    bool isValid() const { return (bool)process_; }

private:
    std::vector<DWORD> enumerateThreadIds() const;

    Handle process_{};
    DWORD pid_{ 0 };
};
