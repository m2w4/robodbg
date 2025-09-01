// Freezer.cpp
#include "Freezer.h"
#include <cstdio>

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

    // Apply properties first
    for (const auto& st : states) {
        Handle th(openThreadHandle(st.tid, THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION));
        if (!th) { ok = false; continue; }
        if (st.groupAffinity.Mask) SetThreadGroupAffinity(th.h, &st.groupAffinity, nullptr);
        SetThreadPriority(th.h, st.dynPriority);
        SetThreadPriorityBoost(th.h, st.boostDisabled);
    }

    // Resume
    for (const auto& st : states) {
        if (!st.weSuspended) continue;
        Handle th(openThreadHandle(st.tid, THREAD_SUSPEND_RESUME));
        if (!th) { ok = false; continue; }
        if (ResumeThread(th.h) == (DWORD)-1) ok = false;
    }
    return ok;
}
