/* Every Direct3D entry point of the title (tools/take_d3d.py writes this):
 * the hook first (src/d3d/d3d.c), the recompiled body unless it took the call. */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include "d3d_funcs.h"

int d3d_hook(int fn);
void d3d_after(int fn);

void d3d_orig_003C5260(void);
void sub_003C5260(void)  /* D3DDevice_SetRenderState_EdgeAntiAlias */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_EdgeAntiAlias)) {
        d3d_orig_003C5260();
        d3d_after(D3DF_D3DDevice_SetRenderState_EdgeAntiAlias);
    }
}

void d3d_orig_003C52A0(void);
void sub_003C52A0(void)  /* D3DDevice_SetRenderState_ShadowFunc */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_ShadowFunc)) {
        d3d_orig_003C52A0();
        d3d_after(D3DF_D3DDevice_SetRenderState_ShadowFunc);
    }
}

void d3d_orig_003C52E0(void);
void sub_003C52E0(void)  /* D3DDevice_SetRenderState_FogColor */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_FogColor)) {
        d3d_orig_003C52E0();
        d3d_after(D3DF_D3DDevice_SetRenderState_FogColor);
    }
}

void d3d_orig_003C5340(void);
void sub_003C5340(void)  /* D3DDevice_SetRenderState_CullMode */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_CullMode)) {
        d3d_orig_003C5340();
        d3d_after(D3DF_D3DDevice_SetRenderState_CullMode);
    }
}

void d3d_orig_003C53C0(void);
void sub_003C53C0(void)  /* D3DDevice_SetRenderState_FrontFace */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_FrontFace)) {
        d3d_orig_003C53C0();
        d3d_after(D3DF_D3DDevice_SetRenderState_FrontFace);
    }
}

void d3d_orig_003C5410(void);
void sub_003C5410(void)  /* D3DDevice_SetRenderState_NormalizeNormals */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_NormalizeNormals)) {
        d3d_orig_003C5410();
        d3d_after(D3DF_D3DDevice_SetRenderState_NormalizeNormals);
    }
}

void d3d_orig_003C5460(void);
void sub_003C5460(void)  /* D3DDevice_SetRenderState_ZEnable */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_ZEnable)) {
        d3d_orig_003C5460();
        d3d_after(D3DF_D3DDevice_SetRenderState_ZEnable);
    }
}

void d3d_orig_003C5500(void);
void sub_003C5500(void)  /* D3DDevice_SetRenderState_StencilEnable */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_StencilEnable)) {
        d3d_orig_003C5500();
        d3d_after(D3DF_D3DDevice_SetRenderState_StencilEnable);
    }
}

void d3d_orig_003C55B0(void);
void sub_003C55B0(void)  /* D3DDevice_SetRenderState_StencilFail */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_StencilFail)) {
        d3d_orig_003C55B0();
        d3d_after(D3DF_D3DDevice_SetRenderState_StencilFail);
    }
}

void d3d_orig_003C5630(void);
void sub_003C5630(void)  /* D3DDevice_SetRenderState_TextureFactor */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_TextureFactor)) {
        d3d_orig_003C5630();
        d3d_after(D3DF_D3DDevice_SetRenderState_TextureFactor);
    }
}

void d3d_orig_003C56A0(void);
void sub_003C56A0(void)  /* D3DDevice_SetRenderState_LineWidth */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_LineWidth)) {
        d3d_orig_003C56A0();
        d3d_after(D3DF_D3DDevice_SetRenderState_LineWidth);
    }
}

void d3d_orig_003C5710(void);
void sub_003C5710(void)  /* D3DDevice_SetRenderState_Dxt1NoiseEnable */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_Dxt1NoiseEnable)) {
        d3d_orig_003C5710();
        d3d_after(D3DF_D3DDevice_SetRenderState_Dxt1NoiseEnable);
    }
}

void d3d_orig_003C57A0(void);
void sub_003C57A0(void)  /* D3DDevice_SetRenderState_YuvEnable */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_YuvEnable)) {
        d3d_orig_003C57A0();
        d3d_after(D3DF_D3DDevice_SetRenderState_YuvEnable);
    }
}

void d3d_orig_003C57E0(void);
void sub_003C57E0(void)  /* D3DDevice_SetRenderState_OcclusionCullEnable */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_OcclusionCullEnable)) {
        d3d_orig_003C57E0();
        d3d_after(D3DF_D3DDevice_SetRenderState_OcclusionCullEnable);
    }
}

void d3d_orig_003C5850(void);
void sub_003C5850(void)  /* D3DDevice_SetRenderState_StencilCullEnable */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_StencilCullEnable)) {
        d3d_orig_003C5850();
        d3d_after(D3DF_D3DDevice_SetRenderState_StencilCullEnable);
    }
}

void d3d_orig_003C58C0(void);
void sub_003C58C0(void)  /* D3DDevice_SetRenderState_RopZCmpAlwaysRead */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_RopZCmpAlwaysRead)) {
        d3d_orig_003C58C0();
        d3d_after(D3DF_D3DDevice_SetRenderState_RopZCmpAlwaysRead);
    }
}

void d3d_orig_003C58E0(void);
void sub_003C58E0(void)  /* D3DDevice_SetRenderState_RopZRead */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_RopZRead)) {
        d3d_orig_003C58E0();
        d3d_after(D3DF_D3DDevice_SetRenderState_RopZRead);
    }
}

void d3d_orig_003C5900(void);
void sub_003C5900(void)  /* D3DDevice_SetRenderState_DoNotCullUncompressed */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_DoNotCullUncompressed)) {
        d3d_orig_003C5900();
        d3d_after(D3DF_D3DDevice_SetRenderState_DoNotCullUncompressed);
    }
}

void d3d_orig_003C5920(void);
void sub_003C5920(void)  /* D3DDevice_SetRenderState_ZBias */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_ZBias)) {
        d3d_orig_003C5920();
        d3d_after(D3DF_D3DDevice_SetRenderState_ZBias);
    }
}

void d3d_orig_003C59D0(void);
void sub_003C59D0(void)  /* D3DDevice_SetRenderState_MultiSampleMode */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_MultiSampleMode)) {
        d3d_orig_003C59D0();
        d3d_after(D3DF_D3DDevice_SetRenderState_MultiSampleMode);
    }
}

void d3d_orig_003C5A00(void);
void sub_003C5A00(void)  /* D3DDevice_SetRenderState_MultiSampleRenderTargetMode */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_MultiSampleRenderTargetMode)) {
        d3d_orig_003C5A00();
        d3d_after(D3DF_D3DDevice_SetRenderState_MultiSampleRenderTargetMode);
    }
}

void d3d_orig_003C5A30(void);
void sub_003C5A30(void)  /* D3DDevice_SetRenderState_MultiSampleAntiAlias */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_MultiSampleAntiAlias)) {
        d3d_orig_003C5A30();
        d3d_after(D3DF_D3DDevice_SetRenderState_MultiSampleAntiAlias);
    }
}

void d3d_orig_003C5A70(void);
void sub_003C5A70(void)  /* D3DDevice_SetRenderState_MultiSampleMask */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_MultiSampleMask)) {
        d3d_orig_003C5A70();
        d3d_after(D3DF_D3DDevice_SetRenderState_MultiSampleMask);
    }
}

void d3d_orig_003C5A90(void);
void sub_003C5A90(void)  /* D3DDevice_SetRenderState_SampleAlpha */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_SampleAlpha)) {
        d3d_orig_003C5A90();
        d3d_after(D3DF_D3DDevice_SetRenderState_SampleAlpha);
    }
}

void d3d_orig_003C5AB0(void);
void sub_003C5AB0(void)  /* D3DDevice_SetRenderState_LogicOp */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_LogicOp)) {
        d3d_orig_003C5AB0();
        d3d_after(D3DF_D3DDevice_SetRenderState_LogicOp);
    }
}

void d3d_orig_003C5B10(void);
void sub_003C5B10(void)  /* D3DDevice_SetRenderState_FillMode */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_FillMode)) {
        d3d_orig_003C5B10();
        d3d_after(D3DF_D3DDevice_SetRenderState_FillMode);
    }
}

void d3d_orig_003C5B60(void);
void sub_003C5B60(void)  /* D3DDevice_SetRenderState_BackFillMode */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_BackFillMode)) {
        d3d_orig_003C5B60();
        d3d_after(D3DF_D3DDevice_SetRenderState_BackFillMode);
    }
}

void d3d_orig_003C5B80(void);
void sub_003C5B80(void)  /* D3DDevice_SetRenderState_TwoSidedLighting */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_TwoSidedLighting)) {
        d3d_orig_003C5B80();
        d3d_after(D3DF_D3DDevice_SetRenderState_TwoSidedLighting);
    }
}

void d3d_orig_003C5BE0(void);
void sub_003C5BE0(void)  /* D3DDevice_SetRenderState_VertexBlend */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_VertexBlend)) {
        d3d_orig_003C5BE0();
        d3d_after(D3DF_D3DDevice_SetRenderState_VertexBlend);
    }
}

void d3d_orig_003C5C30(void);
void sub_003C5C30(void)  /* D3DDevice_SetRenderState_PSTextureModes */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_PSTextureModes)) {
        d3d_orig_003C5C30();
        d3d_after(D3DF_D3DDevice_SetRenderState_PSTextureModes);
    }
}

void d3d_orig_003C5C60(void);
void sub_003C5C60(void)  /* D3DDevice_SetRenderState_Simple */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderState_Simple)) {
        d3d_orig_003C5C60();
        d3d_after(D3DF_D3DDevice_SetRenderState_Simple);
    }
}

void d3d_orig_003C5C90(void);
void sub_003C5C90(void)  /* D3DDevice_SetTextureState_TexCoordIndex */
{
    if (!d3d_hook(D3DF_D3DDevice_SetTextureState_TexCoordIndex)) {
        d3d_orig_003C5C90();
        d3d_after(D3DF_D3DDevice_SetTextureState_TexCoordIndex);
    }
}

void d3d_orig_003C5DA0(void);
void sub_003C5DA0(void)  /* D3DDevice_SetTextureState_BumpEnv */
{
    if (!d3d_hook(D3DF_D3DDevice_SetTextureState_BumpEnv)) {
        d3d_orig_003C5DA0();
        d3d_after(D3DF_D3DDevice_SetTextureState_BumpEnv);
    }
}

void d3d_orig_003C5E10(void);
void sub_003C5E10(void)  /* D3DDevice_SetTextureState_BorderColor */
{
    if (!d3d_hook(D3DF_D3DDevice_SetTextureState_BorderColor)) {
        d3d_orig_003C5E10();
        d3d_after(D3DF_D3DDevice_SetTextureState_BorderColor);
    }
}

void d3d_orig_003C5E60(void);
void sub_003C5E60(void)  /* D3DDevice_SetTextureState_ColorKeyColor */
{
    if (!d3d_hook(D3DF_D3DDevice_SetTextureState_ColorKeyColor)) {
        d3d_orig_003C5E60();
        d3d_after(D3DF_D3DDevice_SetTextureState_ColorKeyColor);
    }
}

void d3d_orig_003C5EA0(void);
void sub_003C5EA0(void)  /* D3DDevice_SetTextureStageStateNotInline */
{
    if (!d3d_hook(D3DF_D3DDevice_SetTextureStageStateNotInline)) {
        d3d_orig_003C5EA0();
        d3d_after(D3DF_D3DDevice_SetTextureStageStateNotInline);
    }
}

void d3d_orig_003C5FD0(void);
void sub_003C5FD0(void)  /* D3DDevice_GetViewportOffsetAndScale */
{
    if (!d3d_hook(D3DF_D3DDevice_GetViewportOffsetAndScale)) {
        d3d_orig_003C5FD0();
        d3d_after(D3DF_D3DDevice_GetViewportOffsetAndScale);
    }
}

void d3d_orig_003C63B0(void);
void sub_003C63B0(void)  /* D3D_CommonSetDebugRegisters */
{
    if (!d3d_hook(D3DF_D3D_CommonSetDebugRegisters)) {
        d3d_orig_003C63B0();
        d3d_after(D3DF_D3D_CommonSetDebugRegisters);
    }
}

void d3d_orig_003C6490(void);
void sub_003C6490(void)  /* D3D_CommonSetMultiSampleModeAndScale */
{
    if (!d3d_hook(D3DF_D3D_CommonSetMultiSampleModeAndScale)) {
        d3d_orig_003C6490();
        d3d_after(D3DF_D3D_CommonSetMultiSampleModeAndScale);
    }
}

void d3d_orig_003C67B0(void);
void sub_003C67B0(void)  /* D3D_UpdateProjectionViewportTransform */
{
    if (!d3d_hook(D3DF_D3D_UpdateProjectionViewportTransform)) {
        d3d_orig_003C67B0();
        d3d_after(D3DF_D3D_UpdateProjectionViewportTransform);
    }
}

void d3d_orig_003C69D0(void);
void sub_003C69D0(void)  /* D3DDevice_Reset */
{
    if (!d3d_hook(D3DF_D3DDevice_Reset)) {
        d3d_orig_003C69D0();
        d3d_after(D3DF_D3DDevice_Reset);
    }
}

void d3d_orig_003C6A80(void);
void sub_003C6A80(void)  /* D3DDevice_SetRenderTarget */
{
    if (!d3d_hook(D3DF_D3DDevice_SetRenderTarget)) {
        d3d_orig_003C6A80();
        d3d_after(D3DF_D3DDevice_SetRenderTarget);
    }
}

void d3d_orig_003C6D90(void);
void sub_003C6D90(void)  /* D3DDevice_SetGammaRamp */
{
    if (!d3d_hook(D3DF_D3DDevice_SetGammaRamp)) {
        d3d_orig_003C6D90();
        d3d_after(D3DF_D3DDevice_SetGammaRamp);
    }
}

void d3d_orig_003C6E10(void);
void sub_003C6E10(void)  /* D3DDevice_CopyRects */
{
    if (!d3d_hook(D3DF_D3DDevice_CopyRects)) {
        d3d_orig_003C6E10();
        d3d_after(D3DF_D3DDevice_CopyRects);
    }
}

void d3d_orig_003C7160(void);
void sub_003C7160(void)  /* D3DDevice_SetTransform */
{
    if (!d3d_hook(D3DF_D3DDevice_SetTransform)) {
        d3d_orig_003C7160();
        d3d_after(D3DF_D3DDevice_SetTransform);
    }
}

void d3d_orig_003C7270(void);
void sub_003C7270(void)  /* D3DDevice_SetViewport */
{
    if (!d3d_hook(D3DF_D3DDevice_SetViewport)) {
        d3d_orig_003C7270();
        d3d_after(D3DF_D3DDevice_SetViewport);
    }
}

void d3d_orig_003C7490(void);
void sub_003C7490(void)  /* D3DDevice_SetLight */
{
    if (!d3d_hook(D3DF_D3DDevice_SetLight)) {
        d3d_orig_003C7490();
        d3d_after(D3DF_D3DDevice_SetLight);
    }
}

void d3d_orig_003C7850(void);
void sub_003C7850(void)  /* D3DDevice_LightEnable */
{
    if (!d3d_hook(D3DF_D3DDevice_LightEnable)) {
        d3d_orig_003C7850();
        d3d_after(D3DF_D3DDevice_LightEnable);
    }
}

void d3d_orig_003C79A0(void);
void sub_003C79A0(void)  /* D3DDevice_SetTexture */
{
    if (!d3d_hook(D3DF_D3DDevice_SetTexture)) {
        d3d_orig_003C79A0();
        d3d_after(D3DF_D3DDevice_SetTexture);
    }
}

void d3d_orig_003C7B50(void);
void sub_003C7B50(void)  /* D3DDevice_SetPalette */
{
    if (!d3d_hook(D3DF_D3DDevice_SetPalette)) {
        d3d_orig_003C7B50();
        d3d_after(D3DF_D3DDevice_SetPalette);
    }
}

void d3d_orig_003C7C50(void);
void sub_003C7C50(void)  /* D3DDevice_EndVisibilityTest */
{
    if (!d3d_hook(D3DF_D3DDevice_EndVisibilityTest)) {
        d3d_orig_003C7C50();
        d3d_after(D3DF_D3DDevice_EndVisibilityTest);
    }
}

void d3d_orig_003C7CB0(void);
void sub_003C7CB0(void)  /* D3DDevice_BlockUntilVerticalBlank */
{
    if (!d3d_hook(D3DF_D3DDevice_BlockUntilVerticalBlank)) {
        d3d_orig_003C7CB0();
        d3d_after(D3DF_D3DDevice_BlockUntilVerticalBlank);
    }
}

void d3d_orig_003C7CE0(void);
void sub_003C7CE0(void)  /* D3D_SetTileNoWait */
{
    if (!d3d_hook(D3DF_D3D_SetTileNoWait)) {
        d3d_orig_003C7CE0();
        d3d_after(D3DF_D3D_SetTileNoWait);
    }
}

void d3d_orig_003C7DB0(void);
void sub_003C7DB0(void)  /* D3DDevice_SetScissors */
{
    if (!d3d_hook(D3DF_D3DDevice_SetScissors)) {
        d3d_orig_003C7DB0();
        d3d_after(D3DF_D3DDevice_SetScissors);
    }
}

void d3d_orig_003C8120(void);
void sub_003C8120(void)  /* D3DDevice_PersistDisplay */
{
    if (!d3d_hook(D3DF_D3DDevice_PersistDisplay)) {
        d3d_orig_003C8120();
        d3d_after(D3DF_D3DDevice_PersistDisplay);
    }
}

void d3d_orig_003C82B0(void);
void sub_003C82B0(void)  /* D3DDevice_SetFlickerFilter */
{
    if (!d3d_hook(D3DF_D3DDevice_SetFlickerFilter)) {
        d3d_orig_003C82B0();
        d3d_after(D3DF_D3DDevice_SetFlickerFilter);
    }
}

void d3d_orig_003C82F0(void);
void sub_003C82F0(void)  /* D3DDevice_SetScreenSpaceOffset */
{
    if (!d3d_hook(D3DF_D3DDevice_SetScreenSpaceOffset)) {
        d3d_orig_003C82F0();
        d3d_after(D3DF_D3DDevice_SetScreenSpaceOffset);
    }
}

void d3d_orig_003C8710(void);
void sub_003C8710(void)  /* D3DDevice_SetStreamSource */
{
    if (!d3d_hook(D3DF_D3DDevice_SetStreamSource)) {
        d3d_orig_003C8710();
        d3d_after(D3DF_D3DDevice_SetStreamSource);
    }
}

void d3d_orig_003C88A0(void);
void sub_003C88A0(void)  /* D3DDevice_LoadVertexShader */
{
    if (!d3d_hook(D3DF_D3DDevice_LoadVertexShader)) {
        d3d_orig_003C88A0();
        d3d_after(D3DF_D3DDevice_LoadVertexShader);
    }
}

void d3d_orig_003C8930(void);
void sub_003C8930(void)  /* D3DDevice_SelectVertexShader */
{
    if (!d3d_hook(D3DF_D3DDevice_SelectVertexShader)) {
        d3d_orig_003C8930();
        d3d_after(D3DF_D3DDevice_SelectVertexShader);
    }
}

void d3d_orig_003C89F0(void);
void sub_003C89F0(void)  /* D3DDevice_SetShaderConstantMode */
{
    if (!d3d_hook(D3DF_D3DDevice_SetShaderConstantMode)) {
        d3d_orig_003C89F0();
        d3d_after(D3DF_D3DDevice_SetShaderConstantMode);
    }
}

void d3d_orig_003C8DC0(void);
void sub_003C8DC0(void)  /* D3DDevice_CreateVertexShader */
{
    if (!d3d_hook(D3DF_D3DDevice_CreateVertexShader)) {
        d3d_orig_003C8DC0();
        d3d_after(D3DF_D3DDevice_CreateVertexShader);
    }
}

void d3d_orig_003C9030(void);
void sub_003C9030(void)  /* D3DDevice_SetVertexShader */
{
    if (!d3d_hook(D3DF_D3DDevice_SetVertexShader)) {
        d3d_orig_003C9030();
        d3d_after(D3DF_D3DDevice_SetVertexShader);
    }
}

void d3d_orig_003C91A0(void);
void sub_003C91A0(void)  /* D3DDevice_SetVertexShaderConstant1 */
{
    if (!d3d_hook(D3DF_D3DDevice_SetVertexShaderConstant1)) {
        d3d_orig_003C91A0();
        d3d_after(D3DF_D3DDevice_SetVertexShaderConstant1);
    }
}

void d3d_orig_003C9200(void);
void sub_003C9200(void)  /* D3DDevice_SetVertexShaderConstant4 */
{
    if (!d3d_hook(D3DF_D3DDevice_SetVertexShaderConstant4)) {
        d3d_orig_003C9200();
        d3d_after(D3DF_D3DDevice_SetVertexShaderConstant4);
    }
}

void d3d_orig_003C92B0(void);
void sub_003C92B0(void)  /* D3DDevice_CreatePixelShader */
{
    if (!d3d_hook(D3DF_D3DDevice_CreatePixelShader)) {
        d3d_orig_003C92B0();
        d3d_after(D3DF_D3DDevice_CreatePixelShader);
    }
}

void d3d_orig_003C9300(void);
void sub_003C9300(void)  /* D3DDevice_SetPixelShader */
{
    if (!d3d_hook(D3DF_D3DDevice_SetPixelShader)) {
        d3d_orig_003C9300();
        d3d_after(D3DF_D3DDevice_SetPixelShader);
    }
}

void d3d_orig_003C9510(void);
void sub_003C9510(void)  /* D3DDevice_SetPixelShaderConstant */
{
    if (!d3d_hook(D3DF_D3DDevice_SetPixelShaderConstant)) {
        d3d_orig_003C9510();
        d3d_after(D3DF_D3DDevice_SetPixelShaderConstant);
    }
}

void d3d_orig_003C97E0(void);
void sub_003C97E0(void)  /* D3DDevice_CreateVertexBuffer2 */
{
    if (!d3d_hook(D3DF_D3DDevice_CreateVertexBuffer2)) {
        d3d_orig_003C97E0();
        d3d_after(D3DF_D3DDevice_CreateVertexBuffer2);
    }
}

void d3d_orig_003C98C0(void);
void sub_003C98C0(void)  /* D3DDevice_DrawVerticesUP */
{
    if (!d3d_hook(D3DF_D3DDevice_DrawVerticesUP)) {
        d3d_orig_003C98C0();
        d3d_after(D3DF_D3DDevice_DrawVerticesUP);
    }
}

void d3d_orig_003C9A70(void);
void sub_003C9A70(void)  /* D3DDevice_DrawVertices */
{
    if (!d3d_hook(D3DF_D3DDevice_DrawVertices)) {
        d3d_orig_003C9A70();
        d3d_after(D3DF_D3DDevice_DrawVertices);
    }
}

void d3d_orig_003C9B40(void);
void sub_003C9B40(void)  /* D3DDevice_DrawIndexedVertices */
{
    if (!d3d_hook(D3DF_D3DDevice_DrawIndexedVertices)) {
        d3d_orig_003C9B40();
        d3d_after(D3DF_D3DDevice_DrawIndexedVertices);
    }
}

void d3d_orig_003C9E20(void);
void sub_003C9E20(void)  /* D3DDevice_End */
{
    if (!d3d_hook(D3DF_D3DDevice_End)) {
        d3d_orig_003C9E20();
        d3d_after(D3DF_D3DDevice_End);
    }
}

void d3d_orig_003C9EC0(void);
void sub_003C9EC0(void)  /* D3D_DestroyResource */
{
    if (!d3d_hook(D3DF_D3D_DestroyResource)) {
        d3d_orig_003C9EC0();
        d3d_after(D3DF_D3D_DestroyResource);
    }
}

void d3d_orig_003C9FD0(void);
void sub_003C9FD0(void)  /* D3DResource_AddRef */
{
    if (!d3d_hook(D3DF_D3DResource_AddRef)) {
        d3d_orig_003C9FD0();
        d3d_after(D3DF_D3DResource_AddRef);
    }
}

void d3d_orig_003CA010(void);
void sub_003CA010(void)  /* D3DResource_Release */
{
    if (!d3d_hook(D3DF_D3DResource_Release)) {
        d3d_orig_003CA010();
        d3d_after(D3DF_D3DResource_Release);
    }
}

void d3d_orig_003CA060(void);
void sub_003CA060(void)  /* D3DResource_GetType */
{
    if (!d3d_hook(D3DF_D3DResource_GetType)) {
        d3d_orig_003CA060();
        d3d_after(D3DF_D3DResource_GetType);
    }
}

void d3d_orig_003CA270(void);
void sub_003CA270(void)  /* D3DDevice_Clear */
{
    if (!d3d_hook(D3DF_D3DDevice_Clear)) {
        d3d_orig_003CA270();
        d3d_after(D3DF_D3DDevice_Clear);
    }
}

void d3d_orig_003CA720(void);
void sub_003CA720(void)  /* D3D_SetFence */
{
    if (!d3d_hook(D3DF_D3D_SetFence)) {
        d3d_orig_003CA720();
        d3d_after(D3DF_D3D_SetFence);
    }
}

void d3d_orig_003CAA10(void);
void sub_003CAA10(void)  /* D3D_BlockOnTime */
{
    if (!d3d_hook(D3DF_D3D_BlockOnTime)) {
        d3d_orig_003CAA10();
        d3d_after(D3DF_D3D_BlockOnTime);
    }
}

void d3d_orig_003CAB80(void);
void sub_003CAB80(void)  /* D3D_MakeRequestedSpace */
{
    if (!d3d_hook(D3DF_D3D_MakeRequestedSpace)) {
        d3d_orig_003CAB80();
        d3d_after(D3DF_D3D_MakeRequestedSpace);
    }
}

void d3d_orig_003CACC0(void);
void sub_003CACC0(void)  /* D3DDevice_MakeSpace */
{
    if (!d3d_hook(D3DF_D3DDevice_MakeSpace)) {
        d3d_orig_003CACC0();
        d3d_after(D3DF_D3DDevice_MakeSpace);
    }
}

void d3d_orig_003CACD0(void);
void sub_003CACD0(void)  /* CDevice_KickOff */
{
    if (!d3d_hook(D3DF_CDevice_KickOff)) {
        d3d_orig_003CACD0();
        d3d_after(D3DF_CDevice_KickOff);
    }
}

void d3d_orig_003CADF0(void);
void sub_003CADF0(void)  /* D3D_BlockOnResource */
{
    if (!d3d_hook(D3DF_D3D_BlockOnResource)) {
        d3d_orig_003CADF0();
        d3d_after(D3DF_D3D_BlockOnResource);
    }
}

void d3d_orig_003CAFC0(void);
void sub_003CAFC0(void)  /* D3DDevice_RunPushBuffer */
{
    if (!d3d_hook(D3DF_D3DDevice_RunPushBuffer)) {
        d3d_orig_003CAFC0();
        d3d_after(D3DF_D3DDevice_RunPushBuffer);
    }
}

void d3d_orig_003CC160(void);
void sub_003CC160(void)  /* D3DDevice_Swap */
{
    if (!d3d_hook(D3DF_D3DDevice_Swap)) {
        d3d_orig_003CC160();
        d3d_after(D3DF_D3DDevice_Swap);
    }
}

void d3d_orig_003CC310(void);
void sub_003CC310(void)  /* Direct3D_CreateDevice */
{
    if (!d3d_hook(D3DF_Direct3D_CreateDevice)) {
        d3d_orig_003CC310();
        d3d_after(D3DF_Direct3D_CreateDevice);
    }
}

void d3d_orig_003CDE80(void);
void sub_003CDE80(void)  /* CDevice_FreeFrameBuffers */
{
    if (!d3d_hook(D3DF_CDevice_FreeFrameBuffers)) {
        d3d_orig_003CDE80();
        d3d_after(D3DF_CDevice_FreeFrameBuffers);
    }
}

void d3d_orig_003CE090(void);
void sub_003CE090(void)  /* CDevice_InitializeFrameBuffers */
{
    if (!d3d_hook(D3DF_CDevice_InitializeFrameBuffers)) {
        d3d_orig_003CE090();
        d3d_after(D3DF_CDevice_InitializeFrameBuffers);
    }
}

void d3d_orig_003CEA10(void);
void sub_003CEA10(void)  /* Get2DSurfaceDesc */
{
    if (!d3d_hook(D3DF_Get2DSurfaceDesc)) {
        d3d_orig_003CEA10();
        d3d_after(D3DF_Get2DSurfaceDesc);
    }
}

void d3d_orig_003CEEF5(void);
void sub_003CEEF5(void)  /* CMiniport_InitHardware */
{
    if (!d3d_hook(D3DF_CMiniport_InitHardware)) {
        d3d_orig_003CEEF5();
        d3d_after(D3DF_CMiniport_InitHardware);
    }
}

void d3d_orig_003D04D9(void);
void sub_003D04D9(void)  /* CMiniport_GetDisplayCapabilities */
{
    if (!d3d_hook(D3DF_CMiniport_GetDisplayCapabilities)) {
        d3d_orig_003D04D9();
        d3d_after(D3DF_CMiniport_GetDisplayCapabilities);
    }
}

void d3d_orig_003D1EF0(void);
void sub_003D1EF0(void)  /* CDevice_SetStateVB */
{
    if (!d3d_hook(D3DF_CDevice_SetStateVB)) {
        d3d_orig_003D1EF0();
        d3d_after(D3DF_CDevice_SetStateVB);
    }
}

void d3d_orig_003D20A0(void);
void sub_003D20A0(void)  /* CDevice_SetStateUP */
{
    if (!d3d_hook(D3DF_CDevice_SetStateUP)) {
        d3d_orig_003D20A0();
        d3d_after(D3DF_CDevice_SetStateUP);
    }
}


/* D3DDevice_BeginVisibilityTest, which the title has inlined (src/d3d/d3d_orig.c): the renderer counts the
 * draws that follow, until D3DDevice_EndVisibilityTest, for the report it reads (the sun's lens flare, #35). */
void d3d_visibility_begin(void);   /* src/d3d/hle.cpp */
void d3d_orig_001A0BB0(void);
void sub_001A0BB0(void)
{
    d3d_visibility_begin();
    d3d_orig_001A0BB0();
}
