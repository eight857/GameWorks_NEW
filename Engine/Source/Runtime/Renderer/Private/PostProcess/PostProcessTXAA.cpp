// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessTXAA.cpp: Post process MotionBlur implementation.
=============================================================================*/

#if WITH_TXAA

#include "PostProcessTXAA.h"
#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessAmbientOcclusion.h"
#include "PostProcessEyeAdaptation.h"
#include "PostProcessTonemap.h"
#include "PostProcessing.h"
#include "SceneUtils.h"
#include "NoExportTypes.h"
#include "PipelineStateCache.h"

void FRCPassPostProcessTXAA::Process(FRenderingCompositePassContext& Context)
{
	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	if (!InputDesc)
	{
		// input is not hooked up correctly
		return;
	}
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(Context.RHICmdList);

	//auto View = Context.View;
	FSceneViewState* ViewState = Context.ViewState;

	FIntPoint TexSize = InputDesc->Extent;

	// we assume the input and output is full resolution

	FIntPoint SrcSize = InputDesc->Extent;
	FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;

	// e.g. 4 means the input texture is 4x smaller than the buffer size
	uint32 ScaleFactor = SceneContext.GetBufferSizeXY().X / SrcSize.X;

	FIntRect SrcRect = Context.View.ViewRect / ScaleFactor;
	FIntRect DestRect = SrcRect;

	SCOPED_DRAW_EVENTF(Context.RHICmdList, TXAA, TEXT("TXAA %dx%d"), SrcRect.Width(), SrcRect.Height());

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	//Context.SetRenderTarget(RHICmdList, DestRenderTarget.TargetableTexture, FTextureRHIRef());
	SetRenderTarget(Context.RHICmdList, DestRenderTarget.TargetableTexture, SceneContext.GetSceneDepthTexture(), ESimpleRenderTargetMode::EUninitializedColorExistingDepth, FExclusiveDepthStencil::DepthRead_StencilWrite);

	// is optimized away if possible (RT size=view size, )
    DrawClearQuad(Context.RHICmdList, Context.GetFeatureLevel(), true, FLinearColor::Black, false, 0, false, 0, PassOutputs[0].RenderTargetDesc.Extent, SrcRect);

	Context.SetViewportAndCallRHI(SrcRect);

    auto GetInputTexture = [&](int Id)
	{
		FRenderingCompositeOutputRef* OutputRef = Context.Pass->GetInput((EPassInputId)Id);
		FRenderingCompositeOutput* Input = OutputRef->GetOutput();
		TRefCountPtr<IPooledRenderTarget> InputPooledElement;
		InputPooledElement = Input->RequestInput();
		const FTextureRHIRef& SrcTexture = InputPooledElement->GetRenderTargetItem().ShaderResourceTexture;

		return SrcTexture;
	};
    auto SampleX = Context.View.TemporalJitterPixelsX;
    auto SampleY = Context.View.TemporalJitterPixelsY;
    static const auto TXAADbgJitter = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.TXAADbgJitter"));
    Context.RHICmdList.ResolveTXAA(DestRenderTarget.TargetableTexture, GetInputTexture(0), GetInputTexture(1), GetInputTexture(2), GetInputTexture(3),
        TXAADbgJitter->GetValueOnRenderThread() ? 
        FVector2D(SampleX * 2.0f / Context.View.ViewRect.Width(), SampleY * -2.0f / Context.View.ViewRect.Height()) :
        FVector2D(SampleX , SampleY));

	Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());

	ViewState->TemporalAAHistoryRT = PassOutputs[0].PooledRenderTarget;
}

FPooledRenderTargetDesc FRCPassPostProcessTXAA::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;

	Ret.Reset();
//     Ret.Format = PF_FloatRGBA;
	Ret.DebugName = TEXT("TXAA");
	Ret.AutoWritable = false;

	return Ret;
}

class FPostProcessComputeMotionVectorPS : public FGlobalShader
{
    DECLARE_SHADER_TYPE(FPostProcessComputeMotionVectorPS, Global);

    static bool ShouldCache(EShaderPlatform Platform)
    {
        return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
    }

    static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
    {
        FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
    }

    /** Default constructor. */
    FPostProcessComputeMotionVectorPS() {}

public:
    FPostProcessPassParameters PostprocessParameter;
    FDeferredPixelShaderParameters DeferredParameters;
    FShaderParameter VelocityScaling;

    /** Initialization constructor. */
    FPostProcessComputeMotionVectorPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FGlobalShader(Initializer)
    {
        PostprocessParameter.Bind(Initializer.ParameterMap);
        DeferredParameters.Bind(Initializer.ParameterMap);
        VelocityScaling.Bind(Initializer.ParameterMap, TEXT("VelocityScaling"));
    }

    // FShader interface.
    virtual bool Serialize(FArchive& Ar) override
    {
        bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
        Ar << PostprocessParameter << DeferredParameters << VelocityScaling;
        return bShaderHasOutdatedParameters;
    }

    void SetParameters(const FRenderingCompositePassContext& Context)
    {
        const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

        FGlobalShader::SetParameters<FViewUniformShaderParameters>(Context.RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);

        FSamplerStateRHIParamRef FilterTable[1];
        FilterTable[0] = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

        PostprocessParameter.SetPS(ShaderRHI, Context, 0, eFC_0000, FilterTable);

        DeferredParameters.Set(Context.RHICmdList, ShaderRHI, Context.View, MD_PostProcess);

        FSceneViewState* ViewState = (FSceneViewState*)Context.View.State;
        const bool bIgnoreVelocity = (ViewState && ViewState->bSequencerIsPaused);
        SetShaderValue(Context.RHICmdList, ShaderRHI, VelocityScaling, bIgnoreVelocity ? 0.0f : 1.0f);

        SetUniformBufferParameter(Context.RHICmdList, ShaderRHI, GetUniformBufferParameter<FCameraMotionParameters>(), CreateCameraMotionParametersUniformBuffer(Context.View));
    }
};

IMPLEMENT_SHADER_TYPE(, FPostProcessComputeMotionVectorPS, TEXT("PostProcessComputeMotionVector"), TEXT("ComputeMotionVectorPS"), SF_Pixel);


void FRCPassPostProcessComputeMotionVector::Process(FRenderingCompositePassContext& Context)
{
    const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

    if (!InputDesc)
    {
        // input is not hooked up correctly
        return;
    }
    FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(Context.RHICmdList);

    //FViewInfo& View = Context.View;
    FSceneViewState* ViewState = Context.ViewState;

    FIntPoint TexSize = InputDesc->Extent;

    // we assume the input and output is full resolution

    FIntPoint SrcSize = InputDesc->Extent;
    FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;

    // e.g. 4 means the input texture is 4x smaller than the buffer size
    uint32 ScaleFactor = SceneContext.GetBufferSizeXY().X / SrcSize.X;

    FIntRect SrcRect = Context.View.ViewRect / ScaleFactor;
    FIntRect DestRect = SrcRect;

    SCOPED_DRAW_EVENTF(Context.RHICmdList, ComputeMotionVector, TEXT("ComputeMotionVector %dx%d"), SrcRect.Width(), SrcRect.Height());

    const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

    //Context.SetRenderTarget(RHICmdList, DestRenderTarget.TargetableTexture, FTextureRHIRef());
    SetRenderTarget(Context.RHICmdList, DestRenderTarget.TargetableTexture, SceneContext.GetSceneDepthTexture(), ESimpleRenderTargetMode::EUninitializedColorExistingDepth, FExclusiveDepthStencil::DepthRead_StencilWrite);

    // is optimized away if possible (RT size=view size, )
    //Context.RHICmdList.Clear(true, FLinearColor::Black, false, (float)ERHIZBuffer::FarPlane, false, 0, SrcRect);
    DrawClearQuad(Context.RHICmdList, Context.GetFeatureLevel(), true, FLinearColor::Black, false, 0, false, 0, PassOutputs[0].RenderTargetDesc.Extent, SrcRect);

    Context.SetViewportAndCallRHI(SrcRect);

    // set the state
    FGraphicsPipelineStateInitializer GraphicsPSOInit;
    Context.RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
    GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
    GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
    GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

    TShaderMapRef< FPostProcessTonemapVS >				VertexShader(Context.GetShaderMap());
    TShaderMapRef< FPostProcessComputeMotionVectorPS >	PixelShader(Context.GetShaderMap());

    GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
    GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
    GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
    GraphicsPSOInit.PrimitiveType = PT_TriangleList;
    SetGraphicsPipelineState(Context.RHICmdList, GraphicsPSOInit);

    VertexShader->SetVS(Context);
    PixelShader->SetParameters(Context);

    DrawPostProcessPass(
        Context.RHICmdList,
        0, 0,
        SrcRect.Width(), SrcRect.Height(),
        SrcRect.Min.X, SrcRect.Min.Y,
        SrcRect.Width(), SrcRect.Height(),
        SrcRect.Size(),
        SrcSize,
        *VertexShader,
        Context.View.StereoPass,
        Context.HasHmdMesh(),
        EDRF_UseTriangleOptimization);

    Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessComputeMotionVector::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
    FPooledRenderTargetDesc Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;

    Ret.Reset();
    Ret.Format = PF_G16R16F;
    Ret.DebugName = TEXT("ComputeMotionVector");
    Ret.AutoWritable = false;

    return Ret;
}

#endif