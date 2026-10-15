#ifndef APU_SDL_OUT_H
#define APU_SDL_OUT_H
#include <stdint.h>
/* SDL2 audio sink for POSIX builds; see apu_sdl_out.c. */
int  apu_sdl_out_init(void);
int  apu_sdl_out_active(void);
void apu_sdl_out_submit(const int16_t *frames, int n);
/* Stop the sound now (paused) or let it start again (not). */
void apu_sdl_out_pause(int paused);
/* Frames queued for the device and not yet played; -1 when not active. */
int  apu_sdl_out_queued_frames(void);
#endif
