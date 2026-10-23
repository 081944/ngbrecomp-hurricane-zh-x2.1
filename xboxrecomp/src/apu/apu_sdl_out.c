/*
 * Host audio output through SDL2, for POSIX builds and for Windows builds
 * with XBOX_SDL_BACKENDS (otherwise Windows has XAudio2).
 *
 * The APU thread renders 48 kHz stereo 16-bit blocks at the console's pace
 * and queues them here. Kept apart from apu_core.c, whose QEMU shim defines
 * SDL-flavoured stand-ins that would clash with the real header.
 */
#if !defined(_WIN32) || defined(XBOX_SDL_BACKENDS)
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "apu_sdl_out.h"

static SDL_AudioDeviceID s_dev;
static FILE *s_wav;          /* NGB_DSOUND_WAV=<file>: what is queued, 48 kHz stereo (compared with the host mixer's) */
static uint32_t s_wav_frames;
static int s_failed;
static int s_playing;

int apu_sdl_out_init(void)
{
    SDL_AudioSpec want, have;
    const char *e = getenv("NGB_AUDIO");

    if (s_dev)
        return 1;
    /* NGB_AUDIO=0 turns sound off. */
    if (s_failed || (e && *e == '0'))
        return 0;
    if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "[APU] SDL audio unavailable: %s\n", SDL_GetError());
        s_failed = 1;
        return 0;
    }
    SDL_zero(want);
    want.freq = 48000;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 480;     /* 10 ms: the device's own share of the delay */
    s_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!s_dev) {
        fprintf(stderr, "[APU] cannot open an audio device: %s\n", SDL_GetError());
        s_failed = 1;
        return 0;
    }
    if ((e = getenv("NGB_DSOUND_WAV")) && (s_wav = fopen(e, "wb"))) {
        static const unsigned char h[44] = { 'R','I','F','F',36,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,
            1,0,2,0,0x80,0xBB,0,0,0,0xEE,2,0,4,0,16,0,'d','a','t','a',0,0,0,0 };
        fwrite(h, 1, 44, s_wav);
    }
    /* Starts paused; apu_sdl_out_submit unpauses once a little is queued. */
    fprintf(stderr, "[APU] SDL audio: %s, 48 kHz stereo\n",
            SDL_GetCurrentAudioDriver());
    return 1;
}

int apu_sdl_out_active(void)
{
    return s_dev != 0;
}

/* Silence while paused: what is queued is dropped, and the device waits for
 * a little to be queued again before it plays (as at the start). */
void apu_sdl_out_pause(int paused)
{
    if (!s_dev)
        return;
    SDL_PauseAudioDevice(s_dev, 1);
    SDL_ClearQueuedAudio(s_dev);
    s_playing = 0;
    (void)paused;
}

int apu_sdl_out_queued_frames(void)
{
    if (!s_dev || !s_playing)
        return -1;
    return (int)(SDL_GetQueuedAudioSize(s_dev) / 4);
}

/* Queue n stereo frames. The APU paces itself on the queue level (see
 * throttle in apu_core.c), so this only drops a backlog that has grown far
 * past it -- a suspended stream -- rather than play it late. */
void apu_sdl_out_submit(const int16_t *frames, int n)
{
    const Uint32 max_queued = 48000 / 2 * 4;   /* 500 ms of S16 stereo */
    const Uint32 prebuffer = 1536 * 4;          /* ~32 ms */
    Uint32 queued;

    if (!s_dev)
        return;
    if (s_wav) {
        uint32_t data, riff;
        long at;
        fwrite(frames, 4, (size_t)n, s_wav);
        s_wav_frames += (uint32_t)n;
        data = s_wav_frames * 4;
        riff = 36 + data;
        at = ftell(s_wav);
        fseek(s_wav, 4, SEEK_SET);
        fwrite(&riff, 4, 1, s_wav);
        fseek(s_wav, 40, SEEK_SET);
        fwrite(&data, 4, 1, s_wav);
        fseek(s_wav, at, SEEK_SET);
    }
    queued = SDL_GetQueuedAudioSize(s_dev);
    if (queued > max_queued)
        SDL_ClearQueuedAudio(s_dev);
    SDL_QueueAudio(s_dev, frames, (Uint32)n * 4);
    if (!s_playing && SDL_GetQueuedAudioSize(s_dev) >= prebuffer) {
        SDL_PauseAudioDevice(s_dev, 0);
        s_playing = 1;
    }
}
#endif
