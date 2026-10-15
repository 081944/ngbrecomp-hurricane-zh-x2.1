/*
 * win32_vcpu.h -- on Windows with the virtual CPU (XBOX_WIN_VCPU), the Win32
 * calls that can block a guest thread give the CPU up meanwhile, as the POSIX
 * compat layer's do (win32_compat.c): waits, sleeps, contended critical
 * sections and SRW locks, condition variables. Thread suspension keeps the
 * CPU off a suspended thread, and a thread that ends leaves it.
 *
 * Included ahead of every file of the runtime and the title (CMake: the
 * platform target's -include), after <windows.h> so its declarations keep
 * their names. win32_host.c and vcpu.c, which call the real functions, are
 * built with XBOX_VCPU_WRAP_IMPL.
 */
#ifndef XBOX_WIN32_VCPU_H
#define XBOX_WIN32_VCPU_H

#if defined(_WIN32) && defined(XBOX_WIN_VCPU) && !defined(XBOX_VCPU_WRAP_IMPL)
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

VOID   vw_EnterCriticalSection(LPCRITICAL_SECTION cs);
VOID   vw_AcquireSRWLockExclusive(PSRWLOCK l);
VOID   vw_AcquireSRWLockShared(PSRWLOCK l);
BOOL   vw_SleepConditionVariableCS(PCONDITION_VARIABLE cv, PCRITICAL_SECTION cs, DWORD ms);
BOOL   vw_SleepConditionVariableSRW(PCONDITION_VARIABLE cv, PSRWLOCK l, DWORD ms, ULONG flags);
DWORD  vw_WaitForSingleObject(HANDLE h, DWORD ms);
DWORD  vw_WaitForSingleObjectEx(HANDLE h, DWORD ms, BOOL alertable);
DWORD  vw_WaitForMultipleObjects(DWORD n, const HANDLE *h, BOOL all, DWORD ms);
DWORD  vw_WaitForMultipleObjectsEx(DWORD n, const HANDLE *h, BOOL all, DWORD ms, BOOL alertable);
DWORD  vw_SignalObjectAndWait(HANDLE sig, HANDLE wait, DWORD ms, BOOL alertable);
VOID   vw_Sleep(DWORD ms);
DWORD  vw_SleepEx(DWORD ms, BOOL alertable);
BOOL   vw_SwitchToThread(void);
DWORD  vw_SuspendThread(HANDLE h);
DWORD  vw_ResumeThread(HANDLE h);
DECLSPEC_NORETURN VOID vw_ExitThread(DWORD code);
HANDLE vw_CreateThread(LPSECURITY_ATTRIBUTES sa, SIZE_T stack, LPTHREAD_START_ROUTINE fn,
                       LPVOID arg, DWORD flags, LPDWORD tid);

/* The host side of the virtual CPU, as win32_compat.h declares it on POSIX
 * (win32_host.c). */
void  compat_host_enter(void);
void  compat_host_leave(void);
int   compat_guest_enter(void);
void  compat_guest_leave(int saved_depth);
int   compat_host_depth(void);
void  compat_thread_set_user(void *user);
void *compat_thread_user(HANDLE thread);
int   compat_is_thread(HANDLE h);
void  compat_park_if_pending(void);

#ifdef __cplusplus
}
#endif

#define EnterCriticalSection(cs)                 vw_EnterCriticalSection(cs)
#define AcquireSRWLockExclusive(l)               vw_AcquireSRWLockExclusive(l)
#define AcquireSRWLockShared(l)                  vw_AcquireSRWLockShared(l)
#define SleepConditionVariableCS(cv, cs, ms)     vw_SleepConditionVariableCS(cv, cs, ms)
#define SleepConditionVariableSRW(cv, l, ms, f)  vw_SleepConditionVariableSRW(cv, l, ms, f)
#define WaitForSingleObject(h, ms)               vw_WaitForSingleObject(h, ms)
#define WaitForSingleObjectEx(h, ms, a)          vw_WaitForSingleObjectEx(h, ms, a)
#define WaitForMultipleObjects(n, h, all, ms)    vw_WaitForMultipleObjects(n, h, all, ms)
#define WaitForMultipleObjectsEx(n, h, all, ms, a) vw_WaitForMultipleObjectsEx(n, h, all, ms, a)
#define SignalObjectAndWait(s, w, ms, a)         vw_SignalObjectAndWait(s, w, ms, a)
#define Sleep(ms)                                vw_Sleep(ms)
#define SleepEx(ms, a)                           vw_SleepEx(ms, a)
#define SwitchToThread()                         vw_SwitchToThread()
#define SuspendThread(h)                         vw_SuspendThread(h)
#define ResumeThread(h)                          vw_ResumeThread(h)
#define ExitThread(c)                            vw_ExitThread(c)
#define CreateThread(sa, st, fn, arg, fl, tid)   vw_CreateThread(sa, st, fn, arg, fl, tid)
#endif

#endif
