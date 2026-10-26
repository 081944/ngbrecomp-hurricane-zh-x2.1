/* recomp_threadload.h: each thread's CPU time, read on the thread itself, and the process's. */
#include "threadload.h"

#include <stdint.h>
#include <stdio.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <time.h>
#include <unistd.h>
#endif

static volatile uint64_t s_cpu_us[RECOMP_LOAD_SLOTS];   /* each slot's thread, as of its last sample */
static uint64_t s_last_us[RECOMP_LOAD_SLOTS], s_last_all_us, s_last_wall_us;
static int s_cpus;

#if defined(_WIN32)
static uint64_t filetimes_us(const FILETIME *k, const FILETIME *u)
{
    ULARGE_INTEGER a, b;
    a.LowPart = k->dwLowDateTime; a.HighPart = k->dwHighDateTime;
    b.LowPart = u->dwLowDateTime; b.HighPart = u->dwHighDateTime;
    return (a.QuadPart + b.QuadPart) / 10;
}
static uint64_t thread_us(void)
{
    FILETIME c, e, k, u;
    return GetThreadTimes(GetCurrentThread(), &c, &e, &k, &u) ? filetimes_us(&k, &u) : 0;
}
static uint64_t process_us(void)
{
    FILETIME c, e, k, u;
    return GetProcessTimes(GetCurrentProcess(), &c, &e, &k, &u) ? filetimes_us(&k, &u) : 0;
}
static uint64_t wall_us(void)
{
    LARGE_INTEGER f, n;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&n);
    return (uint64_t)(n.QuadPart / (f.QuadPart / 1000000));
}
static int cpu_count(void)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (int)si.dwNumberOfProcessors;
}
#else
static uint64_t clock_us(clockid_t id)
{
    struct timespec ts;
    if (clock_gettime(id, &ts) != 0)
        return 0;
    return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}
static uint64_t thread_us(void) { return clock_us(CLOCK_THREAD_CPUTIME_ID); }
static uint64_t process_us(void) { return clock_us(CLOCK_PROCESS_CPUTIME_ID); }
static uint64_t wall_us(void) { return clock_us(CLOCK_MONOTONIC); }
static int cpu_count(void) { long n = sysconf(_SC_NPROCESSORS_ONLN); return n > 0 ? (int)n : 1; }
#endif

void recomp_load_sample(int slot)
{
    if (slot >= 0 && slot < RECOMP_LOAD_SLOTS)
        s_cpu_us[slot] = thread_us();
}

void recomp_load_report(void)
{
    static const char *const names[RECOMP_LOAD_SLOTS] = { "game", "GPU model", "audio" };
    uint64_t now = wall_us();
    if (!s_last_wall_us) {
        s_last_wall_us = now;
        return;
    }
    uint64_t dt = now - s_last_wall_us;
    if (dt < 5000000)
        return;
    if (!s_cpus)
        s_cpus = cpu_count();
    uint64_t all = process_us();
    char line[256];
    int n = snprintf(line, sizeof line, "[LOAD]");
    for (int i = 0; i < RECOMP_LOAD_SLOTS && n < (int)sizeof line; i++) {
        uint64_t cpu = s_cpu_us[i];
        /* a thread that never sampled, or not since the last report, shows nothing rather than a stale 0 */
        if (cpu && cpu >= s_last_us[i]) {
            n += snprintf(line + n, sizeof line - (size_t)n, "%s %s %llu ms/s", i ? "," : "", names[i],
                          (unsigned long long)((cpu - s_last_us[i]) * 1000 / dt));
        }
        s_last_us[i] = cpu;
    }
    if (all >= s_last_all_us && n < (int)sizeof line)
        snprintf(line + n, sizeof line - (size_t)n, ", all threads %llu, of %d on %d CPUs",
                 (unsigned long long)((all - s_last_all_us) * 1000 / dt), s_cpus * 1000, s_cpus);
    fprintf(stderr, "%s\n", line);
    s_last_all_us = all;
    s_last_wall_us = now;
}
