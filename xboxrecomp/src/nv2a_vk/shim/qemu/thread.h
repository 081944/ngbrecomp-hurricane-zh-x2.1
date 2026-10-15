/* QEMU's thread primitives over pthreads. QemuEvent is a manual-reset event,
 * as in QEMU: set wakes every waiter and stays set until reset. */
#ifndef NV2A_SHIM_THREAD_H
#define NV2A_SHIM_THREAD_H
#include <pthread.h>

typedef struct QemuMutex { pthread_mutex_t m; } QemuMutex;
typedef struct QemuCond  { pthread_cond_t c; } QemuCond;
typedef struct QemuThread { pthread_t t; } QemuThread;
typedef struct QemuEvent {
    pthread_mutex_t m;
    pthread_cond_t c;
    bool set;
} QemuEvent;

#define QEMU_THREAD_JOINABLE 0
#define QEMU_THREAD_DETACHED 1

static inline void qemu_mutex_init(QemuMutex *m) { pthread_mutex_init(&m->m, NULL); }
static inline void qemu_mutex_destroy(QemuMutex *m) { pthread_mutex_destroy(&m->m); }
static inline void qemu_mutex_lock(QemuMutex *m) { pthread_mutex_lock(&m->m); }
static inline void qemu_mutex_unlock(QemuMutex *m) { pthread_mutex_unlock(&m->m); }
static inline int qemu_mutex_trylock(QemuMutex *m) { return pthread_mutex_trylock(&m->m); }

static inline void qemu_cond_init(QemuCond *c) { pthread_cond_init(&c->c, NULL); }
static inline void qemu_cond_destroy(QemuCond *c) { pthread_cond_destroy(&c->c); }
static inline void qemu_cond_signal(QemuCond *c) { pthread_cond_signal(&c->c); }
static inline void qemu_cond_broadcast(QemuCond *c) { pthread_cond_broadcast(&c->c); }
static inline void qemu_cond_wait(QemuCond *c, QemuMutex *m) { pthread_cond_wait(&c->c, &m->m); }

static inline void qemu_event_init(QemuEvent *e, bool init)
{
    pthread_mutex_init(&e->m, NULL);
    pthread_cond_init(&e->c, NULL);
    e->set = init;
}
static inline void qemu_event_destroy(QemuEvent *e)
{
    pthread_cond_destroy(&e->c);
    pthread_mutex_destroy(&e->m);
}
static inline void qemu_event_set(QemuEvent *e)
{
    pthread_mutex_lock(&e->m);
    e->set = true;
    pthread_cond_broadcast(&e->c);
    pthread_mutex_unlock(&e->m);
}
static inline void qemu_event_reset(QemuEvent *e)
{
    pthread_mutex_lock(&e->m);
    e->set = false;
    pthread_mutex_unlock(&e->m);
}
static inline void qemu_event_wait(QemuEvent *e)
{
    pthread_mutex_lock(&e->m);
    while (!e->set)
        pthread_cond_wait(&e->c, &e->m);
    pthread_mutex_unlock(&e->m);
}

void qemu_thread_create(QemuThread *thread, const char *name,
                        void *(*start_routine)(void *), void *arg, int mode);
static inline void *qemu_thread_join(QemuThread *thread)
{
    void *ret = NULL;
    pthread_join(thread->t, &ret);
    return ret;
}
#endif
