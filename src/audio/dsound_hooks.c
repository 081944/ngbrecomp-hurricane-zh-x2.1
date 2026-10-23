/* Every DirectSound entry point of the game (tools/take_dsound.py writes this):
 * the hook first (src/audio/dsound.c), the recompiled body unless it took the call. */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include "dsound_funcs.h"

const DsFunc ds_funcs[DSF_COUNT] = {
    [DSF_IDirectSound_Release] = { 0x0037A947, "IDirectSound_Release", 4 },
    [DSF_IDirectSoundBuffer_Unlock] = { 0x0037A95D, "IDirectSoundBuffer_Unlock", 20 },
    [DSF_IDirectSoundBuffer_Release] = { 0x0037A962, "IDirectSoundBuffer_Release", 4 },
    [DSF_DirectSoundUseFullHRTF] = { 0x0037A978, "DirectSoundUseFullHRTF", 0 },
    [DSF_IDirectSound_GetSpeakerConfig] = { 0x0037B685, "IDirectSound_GetSpeakerConfig", 8 },
    [DSF_IDirectSound_DownloadEffectsImage] = { 0x0037B6A1, "IDirectSound_DownloadEffectsImage", 20 },
    [DSF_IDirectSound_SetMixBinHeadroom] = { 0x0037B6C8, "IDirectSound_SetMixBinHeadroom", 12 },
    [DSF_IDirectSoundBuffer_SetVolume] = { 0x0037B6E8, "IDirectSoundBuffer_SetVolume", 8 },
    [DSF_IDirectSoundBuffer_SetFilter] = { 0x0037B704, "IDirectSoundBuffer_SetFilter", 8 },
    [DSF_IDirectSoundBuffer_SetHeadroom] = { 0x0037B720, "IDirectSoundBuffer_SetHeadroom", 8 },
    [DSF_IDirectSoundBuffer_SetMixBinVolumes] = { 0x0037B73C, "IDirectSoundBuffer_SetMixBinVolumes", 8 },
    [DSF_IDirectSoundBuffer_Play] = { 0x0037B758, "IDirectSoundBuffer_Play", 16 },
    [DSF_IDirectSoundBuffer_Stop] = { 0x0037B77C, "IDirectSoundBuffer_Stop", 4 },
    [DSF_IDirectSoundBuffer_StopEx] = { 0x0037B794, "IDirectSoundBuffer_StopEx", 16 },
    [DSF_IDirectSoundBuffer_SetLoopRegion] = { 0x0037B7B8, "IDirectSoundBuffer_SetLoopRegion", 12 },
    [DSF_IDirectSoundBuffer_GetStatus] = { 0x0037B7D8, "IDirectSoundBuffer_GetStatus", 8 },
    [DSF_IDirectSoundBuffer_GetCurrentPosition] = { 0x0037B7F4, "IDirectSoundBuffer_GetCurrentPosition", 12 },
    [DSF_IDirectSoundBuffer_SetCurrentPosition] = { 0x0037B814, "IDirectSoundBuffer_SetCurrentPosition", 8 },
    [DSF_IDirectSoundBuffer_Lock] = { 0x0037B830, "IDirectSoundBuffer_Lock", 32 },
    [DSF_IDirectSoundBuffer_SetFrequency] = { 0x0037BFBF, "IDirectSoundBuffer_SetFrequency", 8 },
    [DSF_IDirectSoundBuffer_SetMixBins] = { 0x0037BFDB, "IDirectSoundBuffer_SetMixBins", 8 },
    [DSF_IDirectSoundBuffer_SetMaxDistance] = { 0x0037BFF7, "IDirectSoundBuffer_SetMaxDistance", 12 },
    [DSF_IDirectSoundBuffer_SetMinDistance] = { 0x0037C01B, "IDirectSoundBuffer_SetMinDistance", 12 },
    [DSF_IDirectSoundBuffer_SetPosition] = { 0x0037C03F, "IDirectSoundBuffer_SetPosition", 20 },
    [DSF_IDirectSoundBuffer_SetVelocity] = { 0x0037C074, "IDirectSoundBuffer_SetVelocity", 20 },
    [DSF_IDirectSoundBuffer_SetDistanceFactor] = { 0x0037C0A9, "IDirectSoundBuffer_SetDistanceFactor", 12 },
    [DSF_IDirectSoundBuffer_SetRolloffFactor] = { 0x0037C0CD, "IDirectSoundBuffer_SetRolloffFactor", 12 },
    [DSF_IDirectSoundBuffer_SetBufferData] = { 0x0037C607, "IDirectSoundBuffer_SetBufferData", 12 },
    [DSF_IDirectSound_CommitDeferredSettings] = { 0x0037C9DE, "IDirectSound_CommitDeferredSettings", 4 },
    [DSF_IDirectSound_CreateSoundBuffer] = { 0x0037CC9B, "IDirectSound_CreateSoundBuffer", 16 },
    [DSF_IDirectSound_SetPosition] = { 0x0037CCBF, "IDirectSound_SetPosition", 20 },
    [DSF_IDirectSound_SetVelocity] = { 0x0037CCF4, "IDirectSound_SetVelocity", 20 },
    [DSF_IDirectSound_SetI3DL2Listener] = { 0x0037CD29, "IDirectSound_SetI3DL2Listener", 12 },
    [DSF_DirectSoundCreate] = { 0x0037CEEF, "DirectSoundCreate", 12 },
};

int dsound_hook(int fn);

void dsound_orig_0037A947(void);
void sub_0037A947(void)  /* IDirectSound_Release */
{
    if (!dsound_hook(DSF_IDirectSound_Release))
        dsound_orig_0037A947();
}

void dsound_orig_0037A95D(void);
void sub_0037A95D(void)  /* IDirectSoundBuffer_Unlock */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_Unlock))
        dsound_orig_0037A95D();
}

void dsound_orig_0037A962(void);
void sub_0037A962(void)  /* IDirectSoundBuffer_Release */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_Release))
        dsound_orig_0037A962();
}

void dsound_orig_0037A978(void);
void sub_0037A978(void)  /* DirectSoundUseFullHRTF */
{
    if (!dsound_hook(DSF_DirectSoundUseFullHRTF))
        dsound_orig_0037A978();
}

void dsound_orig_0037B685(void);
void sub_0037B685(void)  /* IDirectSound_GetSpeakerConfig */
{
    if (!dsound_hook(DSF_IDirectSound_GetSpeakerConfig))
        dsound_orig_0037B685();
}

void dsound_orig_0037B6A1(void);
void sub_0037B6A1(void)  /* IDirectSound_DownloadEffectsImage */
{
    if (!dsound_hook(DSF_IDirectSound_DownloadEffectsImage))
        dsound_orig_0037B6A1();
}

void dsound_orig_0037B6C8(void);
void sub_0037B6C8(void)  /* IDirectSound_SetMixBinHeadroom */
{
    if (!dsound_hook(DSF_IDirectSound_SetMixBinHeadroom))
        dsound_orig_0037B6C8();
}

void dsound_orig_0037B6E8(void);
void sub_0037B6E8(void)  /* IDirectSoundBuffer_SetVolume */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetVolume))
        dsound_orig_0037B6E8();
}

void dsound_orig_0037B704(void);
void sub_0037B704(void)  /* IDirectSoundBuffer_SetFilter */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetFilter))
        dsound_orig_0037B704();
}

void dsound_orig_0037B720(void);
void sub_0037B720(void)  /* IDirectSoundBuffer_SetHeadroom */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetHeadroom))
        dsound_orig_0037B720();
}

void dsound_orig_0037B73C(void);
void sub_0037B73C(void)  /* IDirectSoundBuffer_SetMixBinVolumes */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetMixBinVolumes))
        dsound_orig_0037B73C();
}

void dsound_orig_0037B758(void);
void sub_0037B758(void)  /* IDirectSoundBuffer_Play */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_Play))
        dsound_orig_0037B758();
}

void dsound_orig_0037B77C(void);
void sub_0037B77C(void)  /* IDirectSoundBuffer_Stop */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_Stop))
        dsound_orig_0037B77C();
}

void dsound_orig_0037B794(void);
void sub_0037B794(void)  /* IDirectSoundBuffer_StopEx */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_StopEx))
        dsound_orig_0037B794();
}

void dsound_orig_0037B7B8(void);
void sub_0037B7B8(void)  /* IDirectSoundBuffer_SetLoopRegion */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetLoopRegion))
        dsound_orig_0037B7B8();
}

void dsound_orig_0037B7D8(void);
void sub_0037B7D8(void)  /* IDirectSoundBuffer_GetStatus */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_GetStatus))
        dsound_orig_0037B7D8();
}

void dsound_orig_0037B7F4(void);
void sub_0037B7F4(void)  /* IDirectSoundBuffer_GetCurrentPosition */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_GetCurrentPosition))
        dsound_orig_0037B7F4();
}

void dsound_orig_0037B814(void);
void sub_0037B814(void)  /* IDirectSoundBuffer_SetCurrentPosition */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetCurrentPosition))
        dsound_orig_0037B814();
}

void dsound_orig_0037B830(void);
void sub_0037B830(void)  /* IDirectSoundBuffer_Lock */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_Lock))
        dsound_orig_0037B830();
}

void dsound_orig_0037BFBF(void);
void sub_0037BFBF(void)  /* IDirectSoundBuffer_SetFrequency */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetFrequency))
        dsound_orig_0037BFBF();
}

void dsound_orig_0037BFDB(void);
void sub_0037BFDB(void)  /* IDirectSoundBuffer_SetMixBins */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetMixBins))
        dsound_orig_0037BFDB();
}

void dsound_orig_0037BFF7(void);
void sub_0037BFF7(void)  /* IDirectSoundBuffer_SetMaxDistance */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetMaxDistance))
        dsound_orig_0037BFF7();
}

void dsound_orig_0037C01B(void);
void sub_0037C01B(void)  /* IDirectSoundBuffer_SetMinDistance */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetMinDistance))
        dsound_orig_0037C01B();
}

void dsound_orig_0037C03F(void);
void sub_0037C03F(void)  /* IDirectSoundBuffer_SetPosition */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetPosition))
        dsound_orig_0037C03F();
}

void dsound_orig_0037C074(void);
void sub_0037C074(void)  /* IDirectSoundBuffer_SetVelocity */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetVelocity))
        dsound_orig_0037C074();
}

void dsound_orig_0037C0A9(void);
void sub_0037C0A9(void)  /* IDirectSoundBuffer_SetDistanceFactor */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetDistanceFactor))
        dsound_orig_0037C0A9();
}

void dsound_orig_0037C0CD(void);
void sub_0037C0CD(void)  /* IDirectSoundBuffer_SetRolloffFactor */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetRolloffFactor))
        dsound_orig_0037C0CD();
}

void dsound_orig_0037C607(void);
void sub_0037C607(void)  /* IDirectSoundBuffer_SetBufferData */
{
    if (!dsound_hook(DSF_IDirectSoundBuffer_SetBufferData))
        dsound_orig_0037C607();
}

void dsound_orig_0037C9DE(void);
void sub_0037C9DE(void)  /* IDirectSound_CommitDeferredSettings */
{
    if (!dsound_hook(DSF_IDirectSound_CommitDeferredSettings))
        dsound_orig_0037C9DE();
}

void dsound_orig_0037CC9B(void);
void sub_0037CC9B(void)  /* IDirectSound_CreateSoundBuffer */
{
    if (!dsound_hook(DSF_IDirectSound_CreateSoundBuffer))
        dsound_orig_0037CC9B();
}

void dsound_orig_0037CCBF(void);
void sub_0037CCBF(void)  /* IDirectSound_SetPosition */
{
    if (!dsound_hook(DSF_IDirectSound_SetPosition))
        dsound_orig_0037CCBF();
}

void dsound_orig_0037CCF4(void);
void sub_0037CCF4(void)  /* IDirectSound_SetVelocity */
{
    if (!dsound_hook(DSF_IDirectSound_SetVelocity))
        dsound_orig_0037CCF4();
}

void dsound_orig_0037CD29(void);
void sub_0037CD29(void)  /* IDirectSound_SetI3DL2Listener */
{
    if (!dsound_hook(DSF_IDirectSound_SetI3DL2Listener))
        dsound_orig_0037CD29();
}

void dsound_orig_0037CEEF(void);
void sub_0037CEEF(void)  /* DirectSoundCreate */
{
    if (!dsound_hook(DSF_DirectSoundCreate))
        dsound_orig_0037CEEF();
}

