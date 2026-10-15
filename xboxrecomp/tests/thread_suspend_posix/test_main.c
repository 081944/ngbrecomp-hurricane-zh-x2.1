#include "platform/win32_compat.h"
#include <stdio.h>
#include <unistd.h>

static volatile unsigned long s_count;
static volatile int s_in_host, s_host_hold, s_stop;

static DWORD WINAPI spinner(LPVOID p)
{
    (void)p;
    while (!s_stop) {
        s_count++;
        if (s_in_host) {
            compat_host_enter();
            while (s_host_hold)
                s_count++;          /* "host work" continues while suspended */
            compat_host_leave();    /* ...and parks here */
            s_in_host = 0;
        }
    }
    return 0;
}

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d: %s\n", \
    __FILE__, __LINE__, #c); return 1; } } while (0)

int main(void)
{
    HANDLE t = CreateThread(NULL, 0, spinner, NULL, 0, NULL);
    unsigned long a, b;

    usleep(20000);
    CHECK(SuspendThread(t) == 0);
    a = s_count; usleep(50000); b = s_count;
    CHECK(a == b);                       /* guest code stopped */
    CHECK(ResumeThread(t) == 1);
    usleep(20000);
    CHECK(s_count != b);                 /* and runs again */

    s_host_hold = 1; s_in_host = 1;
    usleep(20000);
    CHECK(SuspendThread(t) == 0);
    a = s_count; usleep(50000); b = s_count;
    CHECK(a != b);                       /* host code is not interrupted */
    s_host_hold = 0;
    usleep(20000);
    a = s_count; usleep(50000); b = s_count;
    CHECK(a == b);                       /* parked on leaving it */
    CHECK(ResumeThread(t) == 1);
    usleep(20000);
    CHECK(s_count != b);

    s_stop = 1;
    WaitForSingleObject(t, 1000);
    printf("thread_suspend_posix_test: OK\n");
    return 0;
}
