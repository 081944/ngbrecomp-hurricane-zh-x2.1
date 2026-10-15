/*
 * The slice of QEMU's osdep.h that xemu's NV2A code relies on: the standard
 * headers, glib, and the handful of helper macros and byte-order accessors.
 * Everything here behaves as the QEMU original does on a little-endian host.
 */
#ifndef NV2A_SHIM_OSDEP_H
#define NV2A_SHIM_OSDEP_H

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <glib.h>

#include "qemu/compiler.h"
#include "qemu/units.h"

typedef uint64_t hwaddr;
#define HWADDR_PRIx PRIx64
#define HWADDR_PRId PRId64

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define QEMU_ALIGN_DOWN(n, m) ((n) / (m) * (m))
#define QEMU_ALIGN_UP(n, m)   QEMU_ALIGN_DOWN((n) + (m) - 1, (m))
#define ROUND_UP(n, d)        (((n) + (d) - 1) & -(0 ? (n) : (d)))
#define ROUND_DOWN(n, d)      ((n) & -(0 ? (n) : (d)))
#define DIV_ROUND_UP(n, d)    (((n) + (d) - 1) / (d))

#define NANOSECONDS_PER_SECOND 1000000000LL

/* Guest pages are 4 KiB whatever the host uses. */
#define TARGET_PAGE_BITS 12
#define TARGET_PAGE_SIZE (1u << TARGET_PAGE_BITS)
#define TARGET_PAGE_MASK ((hwaddr)~(TARGET_PAGE_SIZE - 1))
#define TARGET_PAGE_ALIGN(addr) ROUND_UP((addr), TARGET_PAGE_SIZE)

/* ---- Bit helpers ---- */
static inline int ctz32(uint32_t v) { return v ? __builtin_ctz(v) : 32; }
static inline int clz32(uint32_t v) { return v ? __builtin_clz(v) : 32; }
static inline int ctz64(uint64_t v) { return v ? __builtin_ctzll(v) : 64; }
static inline int clz64(uint64_t v) { return v ? __builtin_clzll(v) : 64; }
static inline bool is_power_of_2(uint64_t v) { return v && !(v & (v - 1)); }
static inline uint64_t pow2ceil(uint64_t v)
{
    return v <= 1 ? 1 : 1ULL << (64 - clz64(v - 1));
}
static inline uint32_t extract32(uint32_t v, int start, int len)
{
    return (v >> start) & (~0U >> (32 - len));
}
static inline uint64_t extract64(uint64_t v, int start, int len)
{
    return (v >> start) & (~0ULL >> (64 - len));
}
static inline uint32_t deposit32(uint32_t v, int start, int len, uint32_t f)
{
    uint32_t mask = (~0U >> (32 - len)) << start;
    return (v & ~mask) | ((f << start) & mask);
}

/* a * b / c without intermediate overflow. */
static inline uint64_t muldiv64(uint64_t a, uint32_t b, uint32_t c)
{
    return (uint64_t)(((unsigned __int128)a * b) / c);
}
static inline uint64_t muldiv64_round_up(uint64_t a, uint32_t b, uint32_t c)
{
    return (uint64_t)(((unsigned __int128)a * b + c - 1) / c);
}

/* ---- Little-endian accessors (host is little-endian) ---- */
static inline uint32_t ldl_le_p(const void *p) { uint32_t v; memcpy(&v, p, 4); return v; }
static inline uint16_t lduw_le_p(const void *p) { uint16_t v; memcpy(&v, p, 2); return v; }
static inline uint64_t ldq_le_p(const void *p) { uint64_t v; memcpy(&v, p, 8); return v; }
static inline void stl_le_p(void *p, uint32_t v) { memcpy(p, &v, 4); }
static inline void stw_le_p(void *p, uint16_t v) { memcpy(p, &v, 2); }
static inline void stq_le_p(void *p, uint64_t v) { memcpy(p, &v, 8); }

/* ---- Atomics ---- */
#define qatomic_read(ptr)       __atomic_load_n(ptr, __ATOMIC_RELAXED)
#define qatomic_set(ptr, i)     __atomic_store_n(ptr, i, __ATOMIC_RELAXED)
#define qatomic_inc(ptr)        ((void)__atomic_fetch_add(ptr, 1, __ATOMIC_SEQ_CST))
#define qatomic_dec(ptr)        ((void)__atomic_fetch_sub(ptr, 1, __ATOMIC_SEQ_CST))
#define qatomic_xchg(ptr, i)    __atomic_exchange_n(ptr, i, __ATOMIC_SEQ_CST)
#define qatomic_or(ptr, n)      ((void)__atomic_fetch_or(ptr, n, __ATOMIC_SEQ_CST))
#define qatomic_and(ptr, n)     ((void)__atomic_fetch_and(ptr, n, __ATOMIC_SEQ_CST))
#define smp_mb()                __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define smp_rmb()               __atomic_thread_fence(__ATOMIC_ACQUIRE)
#define smp_wmb()               __atomic_thread_fence(__ATOMIC_RELEASE)

/* ---- Errors and reporting ---- */
#define error_report(...)  (fprintf(stderr, "nv2a: " __VA_ARGS__), fputc('\n', stderr))
#define warn_report(...)   (fprintf(stderr, "nv2a warning: " __VA_ARGS__), fputc('\n', stderr))
#define info_report(...)   (fprintf(stderr, "nv2a: " __VA_ARGS__), fputc('\n', stderr))

/* No RCU: the NV2A thread registers with it and never uses it. */
static inline void rcu_register_thread(void) {}
static inline void rcu_unregister_thread(void) {}

/* The code is never built under TCG here: guest code is native, like KVM. */
static inline bool tcg_enabled(void) { return false; }

#include "qemu/bitmap.h"
#include "qemu/thread.h"
#include "qapi/error.h"
#include "ui/xemu-settings.h"

#endif
