// Freezer.cpp
#include "Freezer.h"
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>

static Handle openThreadHandle(DWORD tid, DWORD access) {
    return Handle(OpenThread(access, FALSE, tid));
}

Freezer::Freezer(HANDLE process) {
    HANDLE dup{};
    if (!DuplicateHandle(GetCurrentProcess(), process, GetCurrentProcess(),
        &dup, PROCESS_QUERY_LIMITED_INFORMATION, FALSE, 0)) {
        dup = nullptr;
        }
        process_.h = dup;
    if (process_) {
        pid_ = GetProcessId(process_.h);
    }
}

std::vector<DWORD> Freezer::enumerateThreadIds(DWORD pid) {
    std::vector<DWORD> tids;
    Handle snap(CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0));
    if (!snap) return tids;
    THREADENTRY32 te{ sizeof(te) };
    for (BOOL ok = Thread32First(snap.h, &te); ok; ok = Thread32Next(snap.h, &te)) {
        if (te.th32OwnerProcessID == pid) tids.push_back(te.th32ThreadID);
    }
    return tids;
}

// suspend -> optional states
std::optional<std::vector<ThreadState>> Freezer::suspendByPID(DWORD pid) {
    auto tids = enumerateThreadIds(pid);
    if (tids.empty()) return std::nullopt;

    std::vector<ThreadState> states;
    states.reserve(tids.size());
    bool okAll = true;

    for (auto tid : tids) {
        //OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION | THREAD_SET_INFORMATION, FALSE, tid);
        Handle th(OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION | THREAD_SET_INFORMATION, FALSE, tid));
        if (!th) { okAll = false; continue; }

        DWORD prev = SuspendThread(th.h);
        if (prev == (DWORD)-1) { okAll = false; continue; }

        ThreadState st{};
        st.tid = tid;
        st.prevSuspendCount = prev;

        int pr = GetThreadPriority(th.h);
        st.dynPriority = (pr == THREAD_PRIORITY_ERROR_RETURN) ? THREAD_PRIORITY_NORMAL : pr;

        BOOL boost{};
        GetThreadPriorityBoost(th.h, &boost);
        st.boostDisabled = boost;

        GROUP_AFFINITY ga{};
        if (GetThreadGroupAffinity(th.h, &ga)) st.groupAffinity = ga;

        st.weSuspended = true;
        states.push_back(st);
    }

    if (!okAll || states.empty()) return std::nullopt;
    return states;
}

// CSV I/O
bool Freezer::exportAsCSV(const std::vector<ThreadState>& v, const std::string path) {
    std::ofstream f(path, std::ios::trunc);
    if (!f) return false;
    f << "tid,prevSuspendCount,priority,boostDisabled,group,mask,weSuspended\n";
    for (const auto& st : v) {
        f << st.tid << ","
        << st.prevSuspendCount << ","
        << st.dynPriority << ","
        << (st.boostDisabled ? 1 : 0) << ","
        << st.groupAffinity.Group << ","
        << (unsigned long long)st.groupAffinity.Mask << ","
        << (st.weSuspended ? 1 : 0) << "\n";
    }
    return true;
}

std::optional<std::vector<ThreadState>> Freezer::importFromCSV(const std::string path) {
    std::ifstream f(path);
    if (!f) return std::nullopt;
    std::vector<ThreadState> v;
    std::string line;
    std::getline(f, line); // header
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        ThreadState st{};
        unsigned long long mask=0; int boost=0, suspended=0;
        char comma;
        ss >> st.tid >> comma
        >> st.prevSuspendCount >> comma
        >> st.dynPriority >> comma
        >> boost >> comma
        >> st.groupAffinity.Group >> comma
        >> mask >> comma
        >> suspended;
        if (!ss.fail()) {
            st.groupAffinity.Mask = (KAFFINITY)mask;
            st.boostDisabled = boost ? TRUE : FALSE;
            st.weSuspended = suspended!=0;
            v.push_back(st);
        }
    }
    if (v.empty()) return std::nullopt;
    return v;
}


std::vector<DWORD> Freezer::enumerateThreadIds() const {
    std::vector<DWORD> tids;
    if (!process_) return tids;

    Handle snap(CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0));
    if (!snap) return tids;

    THREADENTRY32 te{ sizeof(te) };
    for (BOOL ok = Thread32First(snap.h, &te); ok; ok = Thread32Next(snap.h, &te)) {
        if (te.th32OwnerProcessID == pid_) tids.push_back(te.th32ThreadID);
    }
    return tids;
}

std::optional<std::vector<ThreadState>> Freezer::suspend() const {
    auto tids = enumerateThreadIds();
    if (tids.empty()) return std::nullopt;

    std::vector<ThreadState> states;
    states.reserve(tids.size());
    bool okAll = true;

    for (auto tid : tids) {
        Handle th(openThreadHandle(tid,
                                   THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION | THREAD_SET_INFORMATION));
        if (!th) { okAll = false; continue; }

        DWORD prev = SuspendThread(th.h);
        if (prev == (DWORD)-1) { okAll = false; continue; }

        ThreadState st{};
        st.tid = tid;
        st.prevSuspendCount = prev;

        int pr = GetThreadPriority(th.h);
        st.dynPriority = (pr == THREAD_PRIORITY_ERROR_RETURN) ? THREAD_PRIORITY_NORMAL : pr;

        BOOL boost{};
        GetThreadPriorityBoost(th.h, &boost);
        st.boostDisabled = boost;

        GROUP_AFFINITY ga{};
        if (GetThreadGroupAffinity(th.h, &ga)) st.groupAffinity = ga;

        st.weSuspended = true;
        states.push_back(st);
    }

    if (!okAll || states.empty()) return std::nullopt;
    return states;
}

bool Freezer::restore(const std::vector<ThreadState>& states) const {
    if (!process_) return false;
    bool ok = true;

    for (const auto& st : states) {
        Handle th(openThreadHandle(
            st.tid, THREAD_SUSPEND_RESUME | THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION));
        if (!th) { ok = false; continue; }

        // restore properties first
        if (st.groupAffinity.Mask) SetThreadGroupAffinity(th.h, &st.groupAffinity, nullptr);
        SetThreadPriority(th.h, st.dynPriority);
        SetThreadPriorityBoost(th.h, st.boostDisabled);

        if (!st.weSuspended) continue;

        for (;;) {
            DWORD prev = ResumeThread(th.h);
            if (prev == DWORD(-1)) { ok = false; break; }
            if (prev == 1) break;
        }
    }
    return ok;
}
