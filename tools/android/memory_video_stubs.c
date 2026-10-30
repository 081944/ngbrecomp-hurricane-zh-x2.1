/* The memory-layout test reads an XBE title but must never create a window.
 * Fail if the test accidentally enters any rendering path. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "video/video_player.h"

static void unexpected_video(void)
{
    fprintf(stderr, "Memory probe unexpectedly entered the video backend\n");
    abort();
}

void xbox_FramebufferWindowSetTitle(const uint16_t *name, int max_chars)
{
    (void)name; (void)max_chars;
}
void xbox_FramebufferWindowStart(void) { unexpected_video(); }
void xbox_FramebufferWindowSet(uint32_t va, uint32_t pitch)
{
    (void)va; (void)pitch; unexpected_video();
}
void xbox_FramebufferWindowFrameStats(uint32_t draws)
{
    (void)draws; unexpected_video();
}
void xbox_FramebufferWindowPresent(uint32_t va, uint32_t pitch)
{
    (void)va; (void)pitch; unexpected_video();
}
