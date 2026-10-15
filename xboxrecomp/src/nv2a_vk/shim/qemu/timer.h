/* QEMU timers, run on one host thread with the BQL held, as in QEMU. Only the
 * virtual and realtime clocks exist; both are the host monotonic clock. */
#ifndef NV2A_SHIM_TIMER_H
#define NV2A_SHIM_TIMER_H
typedef enum { QEMU_CLOCK_REALTIME, QEMU_CLOCK_VIRTUAL, QEMU_CLOCK_HOST,
               QEMU_CLOCK_VIRTUAL_RT } QEMUClockType;
typedef void QEMUTimerCB(void *opaque);
typedef struct QEMUTimer {
    QEMUTimerCB *cb;
    void *opaque;
    int64_t expire_ns;      /* -1 when not armed */
    struct QEMUTimer *next;
    bool linked;
} QEMUTimer;
int64_t qemu_clock_get_ns(QEMUClockType type);
static inline int64_t qemu_clock_get_us(QEMUClockType type)
{
    return qemu_clock_get_ns(type) / 1000;
}
static inline int64_t qemu_clock_get_ms(QEMUClockType type)
{
    return qemu_clock_get_ns(type) / 1000000;
}
void timer_init_ns(QEMUTimer *t, QEMUClockType type, QEMUTimerCB *cb, void *opaque);
void timer_mod(QEMUTimer *t, int64_t expire_ns);
void timer_del(QEMUTimer *t);
bool timer_pending(QEMUTimer *t);
#endif
