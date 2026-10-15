/*
 * win32_host.c -- Windows with the virtual CPU (XBOX_WIN_VCPU): what the
 * POSIX compat layer (win32_compat.c) adds around the host's primitives for
 * vcpu.c, over the real Win32 API.
 *
 *   - host depth: compat_host_enter/leave bracket runtime code (a kernel
 *     call, the D3D layer), where a thread is never preempted or parked;
 *   - the thread records: a guest thread's vcpu record by its host thread;
 *   - the wrappers win32_vcpu.h routes the runtime's calls to: anything that
 *     can block gives the CPU up while it does (vcpu_block_begin/end), as
 *     the POSIX waits do, and a thread that ends leaves the CPU.
 *
 * Built with XBOX_VCPU_WRAP_IMPL, so the calls here are the real ones.
 */
#if defined(_WIN32) && defined(XBOX_WIN_VCPU)
#include <windows.h>
#include <stdlib.h>

#include "vcpu.h"

static __thread volatile int t_host_depth;

void compat_host_enter(void)
{
    t_host_depth++;
}

void compat_host_leave(void)
{
    if (--t_host_depth == 0)
        vcpu_check_preempt();
}

int compat_host_depth(void)
{
    return t_host_depth;
}

volatile int *compat_host_depth_ptr(void)
{
    return &t_host_depth;
}

/* Guest code called from inside runtime code (the boot thread's main
 * routine, run from a kernel call): preemptible again until it returns. */
int compat_guest_enter(void)
{
    int depth = t_host_depth;
    t_host_depth = 0;
    vcpu_check_preempt();
    return depth;
}

void compat_guest_leave(int saved_depth)
{
    t_host_depth = saved_depth;
}

/* Suspension acts at once here (vcpu_win_suspend_thread): nothing deferred. */
void compat_park_if_pending(void)
{
}

/* The kernel's scheduler record of a thread is its vcpu record. */
void compat_thread_set_user(void *user)
{
    (void)user;
}

void *compat_thread_user(HANDLE h)
{
    DWORD tid = GetThreadId(h);
    return tid ? (void *)vcpu_win_find(tid) : NULL;
}

int compat_is_thread(HANDLE h)
{
    return GetThreadId(h) != 0;
}

/* ---- the wrappers (win32_vcpu.h) ---- */

DWORD vw_WaitForSingleObjectEx(HANDLE h, DWORD ms, BOOL alertable);
DWORD vw_WaitForMultipleObjectsEx(DWORD n, const HANDLE *h, BOOL all, DWORD ms, BOOL alertable);

VOID vw_EnterCriticalSection(LPCRITICAL_SECTION cs)
{
    if (TryEnterCriticalSection(cs))
        return;
    vcpu_block_begin();
    EnterCriticalSection(cs);
    vcpu_block_end();
}

VOID vw_AcquireSRWLockExclusive(PSRWLOCK l)
{
    if (TryAcquireSRWLockExclusive(l))
        return;
    vcpu_block_begin();
    AcquireSRWLockExclusive(l);
    vcpu_block_end();
}

VOID vw_AcquireSRWLockShared(PSRWLOCK l)
{
    if (TryAcquireSRWLockShared(l))
        return;
    vcpu_block_begin();
    AcquireSRWLockShared(l);
    vcpu_block_end();
}

BOOL vw_SleepConditionVariableCS(PCONDITION_VARIABLE cv, PCRITICAL_SECTION cs, DWORD ms)
{
    BOOL r;
    vcpu_block_begin();
    r = SleepConditionVariableCS(cv, cs, ms);
    vcpu_block_end();
    return r;
}

BOOL vw_SleepConditionVariableSRW(PCONDITION_VARIABLE cv, PSRWLOCK l, DWORD ms, ULONG flags)
{
    BOOL r;
    vcpu_block_begin();
    r = SleepConditionVariableSRW(cv, l, ms, flags);
    vcpu_block_end();
    return r;
}

DWORD vw_WaitForSingleObject(HANDLE h, DWORD ms)
{
    return vw_WaitForSingleObjectEx(h, ms, FALSE);
}

DWORD vw_WaitForSingleObjectEx(HANDLE h, DWORD ms, BOOL alertable)
{
    /* Already satisfied: no reason to give the CPU up. */
    DWORD r = WaitForSingleObjectEx(h, 0, alertable);
    if (r != WAIT_TIMEOUT || ms == 0)
        return r;
    vcpu_block_begin();
    r = WaitForSingleObjectEx(h, ms, alertable);
    vcpu_block_end();
    return r;
}

DWORD vw_WaitForMultipleObjects(DWORD n, const HANDLE *h, BOOL all, DWORD ms)
{
    return vw_WaitForMultipleObjectsEx(n, h, all, ms, FALSE);
}

DWORD vw_WaitForMultipleObjectsEx(DWORD n, const HANDLE *h, BOOL all, DWORD ms, BOOL alertable)
{
    DWORD r = WaitForMultipleObjectsEx(n, h, all, 0, alertable);
    if (r != WAIT_TIMEOUT || ms == 0)
        return r;
    vcpu_block_begin();
    r = WaitForMultipleObjectsEx(n, h, all, ms, alertable);
    vcpu_block_end();
    return r;
}

DWORD vw_SignalObjectAndWait(HANDLE sig, HANDLE wait, DWORD ms, BOOL alertable)
{
    DWORD r;
    vcpu_block_begin();
    r = SignalObjectAndWait(sig, wait, ms, alertable);
    vcpu_block_end();
    return r;
}

VOID vw_Sleep(DWORD ms)
{
    if (ms == 0) {
        if (vcpu_self())
            vcpu_yield();
        else
            Sleep(0);
        return;
    }
    vcpu_block_begin();
    Sleep(ms);
    vcpu_block_end();
}

DWORD vw_SleepEx(DWORD ms, BOOL alertable)
{
    DWORD r;
    if (ms == 0 && !alertable) {
        vw_Sleep(0);
        return 0;
    }
    vcpu_block_begin();
    r = SleepEx(ms, alertable);
    vcpu_block_end();
    return r;
}

BOOL vw_SwitchToThread(void)
{
    if (vcpu_self()) {
        vcpu_yield_any();
        return TRUE;
    }
    return SwitchToThread();
}

DWORD vw_SuspendThread(HANDLE h)
{
    return (DWORD)vcpu_win_suspend_thread(h);
}

DWORD vw_ResumeThread(HANDLE h)
{
    return (DWORD)vcpu_win_resume_thread(h);
}

VOID vw_ExitThread(DWORD code)
{
    vcpu_detach();
    ExitThread(code);
}

/* A thread's routine returned: it leaves the CPU, as one that calls
 * ExitThread does. */
struct vw_start { LPTHREAD_START_ROUTINE fn; LPVOID arg; };

static DWORD WINAPI vw_thread_main(LPVOID p)
{
    struct vw_start s = *(struct vw_start *)p;
    DWORD rc;
    free(p);
    rc = s.fn(s.arg);
    vcpu_detach();
    return rc;
}

HANDLE vw_CreateThread(LPSECURITY_ATTRIBUTES sa, SIZE_T stack, LPTHREAD_START_ROUTINE fn,
                       LPVOID arg, DWORD flags, LPDWORD tid)
{
    struct vw_start *s = malloc(sizeof *s);
    HANDLE h;
    if (!s)
        return NULL;
    s->fn = fn;
    s->arg = arg;
    h = CreateThread(sa, stack, vw_thread_main, s, flags, tid);
    if (!h)
        free(s);
    return h;
}
#endif
