#ifndef NV2A_SHIM_COMPILER_H
#define NV2A_SHIM_COMPILER_H
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define QEMU_PACKED __attribute__((packed))
#define QEMU_NORETURN __attribute__((noreturn))
#define QEMU_UNUSED __attribute__((unused))
#define G_NORETURN_SHIM __attribute__((noreturn))
#ifndef container_of
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
#endif
#define glue_(a, b) a##b
#define glue(a, b) glue_(a, b)
#define stringify_(s) #s
#define stringify(s) stringify_(s)
#define tostring(s) stringify_(s)
#define QEMU_BUILD_BUG_MSG(x, msg) _Static_assert(!(x), msg)
#define QEMU_BUILD_BUG_ON(x) _Static_assert(!(x), "not expecting: " #x)
#endif
