/* backtrace() and backtrace_symbols_fd() for Android before 13 (API 33), whose C library has no execinfo.h:
 * the frames from the unwinder, each named through dladdr as glibc's does. For the crash and sample reports
 * in ngb-recomp's main.c and nv2a_host.c. */
#ifndef NGB_ANDROID_BACKTRACE_H
#define NGB_ANDROID_BACKTRACE_H

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <unwind.h>

struct ngb_bt_state { void **frames; int n, max; };

static _Unwind_Reason_Code ngb_bt_step(struct _Unwind_Context *ctx, void *arg)
{
    struct ngb_bt_state *s = arg;
    uintptr_t pc = _Unwind_GetIP(ctx);
    if (pc) {
        if (s->n == s->max)
            return _URC_END_OF_STACK;
        s->frames[s->n++] = (void *)pc;
    }
    return _URC_NO_REASON;
}

static inline int backtrace(void **frames, int max)
{
    struct ngb_bt_state s = { frames, 0, max };
    _Unwind_Backtrace(ngb_bt_step, &s);
    return s.n;
}

static inline void backtrace_symbols_fd(void *const *frames, int n, int fd)
{
    for (int i = 0; i < n; i++) {
        Dl_info info;
        if (dladdr(frames[i], &info) && info.dli_fname)
            dprintf(fd, "%s(%s+0x%lx) [%p]\n", info.dli_fname, info.dli_sname ? info.dli_sname : "",
                    (unsigned long)((char *)frames[i] - (char *)(info.dli_saddr ? info.dli_saddr : info.dli_fbase)),
                    frames[i]);
        else
            dprintf(fd, "[%p]\n", frames[i]);
    }
}

#endif
