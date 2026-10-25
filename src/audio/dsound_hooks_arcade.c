/* Every DirectSound entry point of the arcade's programs (NinjaTrilogy and NinjaTomoe share them) (tools/take_dsound.py writes this):
 * the hook first (src/audio/dsound.c), the recompiled body unless it took the call. */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include "dsound_funcs.h"

const DsFunc ds_funcs[DSF_COUNT] = {
    [DSF_IDirectSound_Release] = { 0x0007FFD3, "IDirectSound_Release", 4 },
    [DSF_IDirectSoundBuffer_Unlock] = { 0x0007FFE9, "IDirectSoundBuffer_Unlock", 20 },
    [DSF_IDirectSoundBuffer_Release] = { 0x0007FFEE, "IDirectSoundBuffer_Release", 4 },
    [DSF_IDirectSoundBuffer_SetVolume] = { 0x000809A6, "IDirectSoundBuffer_SetVolume", 8 },
    [DSF_IDirectSoundBuffer_Play] = { 0x000809C2, "IDirectSoundBuffer_Play", 16 },
    [DSF_IDirectSoundBuffer_Stop] = { 0x000809E6, "IDirectSoundBuffer_Stop", 4 },
    [DSF_IDirectSoundBuffer_SetCurrentPosition] = { 0x00080A1A, "IDirectSoundBuffer_SetCurrentPosition", 8 },
    [DSF_IDirectSoundBuffer_Lock] = { 0x00080A36, "IDirectSoundBuffer_Lock", 32 },
    [DSF_IDirectSoundBuffer_SetNotificationPositions] = { 0x00080A66, "IDirectSoundBuffer_SetNotificationPositions", 12 },
    [DSF_DirectSoundDoWork] = { 0x00080A86, "DirectSoundDoWork", 0 },
    [DSF_IDirectSound_CreateSoundBuffer] = { 0x000816A5, "IDirectSound_CreateSoundBuffer", 16 },
    [DSF_DirectSoundCreate] = { 0x0008186F, "DirectSoundCreate", 12 },
};

int dsound_hook(int fn);

void dsound_orig_0007FFD3(void);
void sub_0007FFD3(void)  /* IDirectSound_Release */
{
    if (!dsound_hook(DSF_IDirectSound_Release))
        dsound_orig_0007FFD3();
}

void dsound_orig_0007FFE9(void);
void sub_0007FFE9(void)  /* IDirectSoundBuffer_Unlock */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_Unlock))
        dsound_orig_0007FFE9();
}

void dsound_orig_0007FFEE(void);
void sub_0007FFEE(void)  /* IDirectSoundBuffer_Release */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_Release))
        dsound_orig_0007FFEE();
}

void dsound_orig_000809A6(void);
void sub_000809A6(void)  /* IDirectSoundBuffer_SetVolume */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetVolume))
        dsound_orig_000809A6();
}

void dsound_orig_000809C2(void);
void sub_000809C2(void)  /* IDirectSoundBuffer_Play */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_Play))
        dsound_orig_000809C2();
}

void dsound_orig_000809E6(void);
void sub_000809E6(void)  /* IDirectSoundBuffer_Stop */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_Stop))
        dsound_orig_000809E6();
}

void dsound_orig_00080A1A(void);
void sub_00080A1A(void)  /* IDirectSoundBuffer_SetCurrentPosition */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetCurrentPosition))
        dsound_orig_00080A1A();
}

void dsound_orig_00080A36(void);
void sub_00080A36(void)  /* IDirectSoundBuffer_Lock */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_Lock))
        dsound_orig_00080A36();
}

void dsound_orig_00080A66(void);
void sub_00080A66(void)  /* IDirectSoundBuffer_SetNotificationPositions */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetNotificationPositions))
        dsound_orig_00080A66();
}

void dsound_orig_00080A86(void);
void sub_00080A86(void)  /* DirectSoundDoWork */
{
    if (!dsound_hook(DSF_DirectSoundDoWork))
        dsound_orig_00080A86();
}

void dsound_orig_000816A5(void);
void sub_000816A5(void)  /* IDirectSound_CreateSoundBuffer */
{
    if (!dsound_hook(DSF_IDirectSound_CreateSoundBuffer))
        dsound_orig_000816A5();
}

void dsound_orig_0008186F(void);
void sub_0008186F(void)  /* DirectSoundCreate */
{
    if (!dsound_hook(DSF_DirectSoundCreate))
        dsound_orig_0008186F();
}

