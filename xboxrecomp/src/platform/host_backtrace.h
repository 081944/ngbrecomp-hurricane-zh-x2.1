#ifndef RECOMP_HOST_BACKTRACE_H
#define RECOMP_HOST_BACKTRACE_H

#ifndef __ANDROID__
#include <execinfo.h>
#else
/* Bionic has no execinfo.h. Use the NDK unwinder for diagnostic stacks. As
 * with the desktop symbolizer, this is best-effort crash/sampling output. */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <unwind.h>

typedef struct {
    void **frames;
    int count, capacity;
} recomp_unwind_state;

static _Unwind_Reason_Code recomp_unwind_frame(struct _Unwind_Context *context, void *opaque)
{
    recomp_unwind_state *s = opaque;
    uintptr_t pc = _Unwind_GetIP(context);
    if (!pc || s->count == s->capacity) return _URC_END_OF_STACK;
    s->frames[s->count++] = (void *)pc;
    return _URC_NO_REASON;
}

static inline int recomp_host_backtrace(void **frames, int capacity)
{
    recomp_unwind_state s = {frames, 0, capacity};
    if (capacity > 0) _Unwind_Backtrace(recomp_unwind_frame, &s);
    return s.count;
}

static inline void recomp_host_backtrace_symbols_fd(void *const *frames, int count, int fd)
{
    for (int i = 0; i < count; i++) {
        Dl_info info;
        if (dladdr(frames[i], &info) && info.dli_sname)
            dprintf(fd, "%p %s+0x%lx\n", frames[i], info.dli_sname,
                    (unsigned long)((uintptr_t)frames[i] - (uintptr_t)info.dli_saddr));
        else
            dprintf(fd, "%p\n", frames[i]);
    }
}
#define backtrace recomp_host_backtrace
#define backtrace_symbols_fd recomp_host_backtrace_symbols_fd
#endif
#endif
