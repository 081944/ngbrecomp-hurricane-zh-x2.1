/* The big QEMU lock. MMIO handlers run with it held; the NV2A thread takes it
 * around interrupt updates. A plain mutex: no path takes it recursively. */
#ifndef NV2A_SHIM_MAIN_LOOP_H
#define NV2A_SHIM_MAIN_LOOP_H
void bql_lock(void);
void bql_unlock(void);
bool bql_locked(void);
#include "qemu/timer.h"
#endif
