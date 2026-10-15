#pragma once
#include "qemu/osdep.h"
#define xemu_queue_notification(msg)  fprintf(stderr, "nv2a: %s\n", (msg))
#define xemu_queue_error_message(msg) fprintf(stderr, "nv2a error: %s\n", (msg))
