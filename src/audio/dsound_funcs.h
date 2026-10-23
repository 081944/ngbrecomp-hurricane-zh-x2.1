/* Every DirectSound function a program hooks (tools/take_dsound.py, from tools/dsound_symbols*.txt);
 * each program's hooks file holds its table (ds_funcs), the functions it does not have zero. */
#pragma once
#include <stdint.h>

typedef struct { uint32_t va; const char *name; int stack; } DsFunc;

enum {
    DSF_DirectSoundCreate = 0,
    DSF_DirectSoundDoWork = 1,
    DSF_DirectSoundUseFullHRTF = 2,
    DSF_IDirectSoundBuffer_GetCurrentPosition = 3,
    DSF_IDirectSoundBuffer_GetStatus = 4,
    DSF_IDirectSoundBuffer_Lock = 5,
    DSF_IDirectSoundBuffer_Play = 6,
    DSF_IDirectSoundBuffer_Release = 7,
    DSF_IDirectSoundBuffer_SetBufferData = 8,
    DSF_IDirectSoundBuffer_SetCurrentPosition = 9,
    DSF_IDirectSoundBuffer_SetDistanceFactor = 10,
    DSF_IDirectSoundBuffer_SetFilter = 11,
    DSF_IDirectSoundBuffer_SetFrequency = 12,
    DSF_IDirectSoundBuffer_SetHeadroom = 13,
    DSF_IDirectSoundBuffer_SetLoopRegion = 14,
    DSF_IDirectSoundBuffer_SetMaxDistance = 15,
    DSF_IDirectSoundBuffer_SetMinDistance = 16,
    DSF_IDirectSoundBuffer_SetMixBinVolumes = 17,
    DSF_IDirectSoundBuffer_SetMixBins = 18,
    DSF_IDirectSoundBuffer_SetNotificationPositions = 19,
    DSF_IDirectSoundBuffer_SetPosition = 20,
    DSF_IDirectSoundBuffer_SetRolloffFactor = 21,
    DSF_IDirectSoundBuffer_SetVelocity = 22,
    DSF_IDirectSoundBuffer_SetVolume = 23,
    DSF_IDirectSoundBuffer_Stop = 24,
    DSF_IDirectSoundBuffer_StopEx = 25,
    DSF_IDirectSoundBuffer_Unlock = 26,
    DSF_IDirectSound_CommitDeferredSettings = 27,
    DSF_IDirectSound_CreateSoundBuffer = 28,
    DSF_IDirectSound_DownloadEffectsImage = 29,
    DSF_IDirectSound_GetSpeakerConfig = 30,
    DSF_IDirectSound_Release = 31,
    DSF_IDirectSound_SetI3DL2Listener = 32,
    DSF_IDirectSound_SetMixBinHeadroom = 33,
    DSF_IDirectSound_SetPosition = 34,
    DSF_IDirectSound_SetVelocity = 35,
    DSF_COUNT = 36
};

extern const DsFunc ds_funcs[DSF_COUNT];
