// @third party code - BEGIN HairWorks
#include "RendererPrivate.h"

#include "HairWorksSDK.h"

#ifndef _CPP
#define _CPP
#endif
#include <Nv/HairWorks/Shader/NvHwShaderCommonTypes.h>

#include "AllowWindowsPlatformTypes.h"
#include <Nv/Platform/Dx11/Foundation/NvDx11Handle.h>
#include "HideWindowsPlatformTypes.h"

#include "SceneUtils.h"
#include "ScreenRendering.h"
#include "SceneFilterRendering.h"
#include "AmbientCubemapParameters.h"
#include "HairWorksSceneProxy.h"
#include "HairWorksRenderer.h"

namespace HairWorksRenderer
{
	// Configuration console variables.
	TAutoConsoleVariable<float> CVarHairShadowTexelsScale(TEXT("r.HairWorks.Shadow.TexelsScale"), 5, TEXT(""), ECVF_RenderThreadSafe);
	TAutoConsoleVariable<float> CVarHairShadowBiasScale(TEXT("r.HairWorks.Shadow.BiasScale"), 0, TEXT(""), ECVF_RenderThreadSafe);
	TAutoConsoleVariable<int> CVarHairMsaaLevel(TEXT("r.HairWorks.MsaaLevel"), 4, TEXT(""), ECVF_RenderThreadSafe);
	TAutoConsoleVariable<float> CVarHairOutputVelocity(TEXT("r.HairWorks.OutputVelocity"), 1, TEXT(""), ECVF_RenderThreadSafe);
	TAutoConsoleVariable<int> CVarHairAlwaysCreateRenderTargets(TEXT("r.HairWorks.AlwaysCreateRenderTargets"), 0, TEXT(""), ECVF_RenderThreadSafe);

	// Buffers
	TSharedRef<FRenderTargets> HairRenderTargets(new FRenderTargets);

	// To release buffers.
	class FHairGlobalResource: public FRenderResource
	{
	public:
		void ReleaseDynamicRHI()override
		{
			HairRenderTargets = MakeShareable(new FRenderTargets);
		}
	};

	static TGlobalResource<FHairGlobalResource> HairGlobalResource;

	// Pixel shaders
	class FHairWorksBaseShader
	{
	public:
		static bool ShouldCache(EShaderPlatform Platform)
		{
			return Platform == EShaderPlatform::SP_PCD3D_SM5;
		}
	};

	class FHairWorksBasePs : public FGlobalShader
	{
	protected:
		FHairWorksBasePs()
		{}

		FHairWorksBasePs(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
			: FGlobalShader(Initializer)
		{
			HairConstantBuffer.Bind(Initializer.ParameterMap, TEXT("HairConstantBuffer"));

			TextureSampler.Bind(Initializer.ParameterMap, TEXT("TextureSampler"));

			RootColorTexture.Bind(Initializer.ParameterMap, TEXT("RootColorTexture"));
			TipColorTexture.Bind(Initializer.ParameterMap, TEXT("TipColorTexture"));
			SpecularColorTexture.Bind(Initializer.ParameterMap, TEXT("SpecularColorTexture"));
			StrandTexture.Bind(Initializer.ParameterMap, TEXT("StrandTexture"));

			NvHw_resourceFaceHairIndices.Bind(Initializer.ParameterMap, TEXT("NvHw_resourceFaceHairIndices"));
			NvHw_resourceTangents.Bind(Initializer.ParameterMap, TEXT("NvHw_resourceTangents"));
			NvHw_resourceNormals.Bind(Initializer.ParameterMap, TEXT("NvHw_resourceNormals"));
			NvHw_resourceMasterPositions.Bind(Initializer.ParameterMap, TEXT("NvHw_resourceMasterPositions"));
			NvHw_resourceMasterPrevPositions.Bind(Initializer.ParameterMap, TEXT("NvHw_resourceMasterPrevPositions"));
		}

		void SetParameters(FRHICommandListImmediate& RHICmdList, const FSceneView& View, const NvHw_ConstantBuffer& HairConstBuffer, const TArray<FTexture2DRHIRef>& HairTextures, ID3D11ShaderResourceView* HairSrvs[NvHw::ShaderResourceType::COUNT_OF])
		{
			FGlobalShader::SetParameters(RHICmdList, GetPixelShader(), View);

			SetShaderValue(RHICmdList, GetPixelShader(), this->HairConstantBuffer, HairConstBuffer);

			SetSamplerParameter(RHICmdList, GetPixelShader(), TextureSampler, TStaticSamplerState<>::GetRHI());

			SetTextureParameter(RHICmdList, GetPixelShader(), RootColorTexture, HairTextures[NvHw::HairTextureType::ROOT_COLOR]);
			SetTextureParameter(RHICmdList, GetPixelShader(), TipColorTexture, HairTextures[NvHw::HairTextureType::TIP_COLOR]);
			SetTextureParameter(RHICmdList, GetPixelShader(), SpecularColorTexture, HairTextures[NvHw::HairTextureType::SPECULAR]);
			SetTextureParameter(RHICmdList, GetPixelShader(), StrandTexture, HairTextures[NvHw::HairTextureType::STRAND]);

			auto BindSrv = [&](FShaderResourceParameter& Parameter, NvHw::ShaderResourceType::Enum HairSrvType)
			{
				if (!Parameter.IsBound())
					return;

				extern ENGINE_API ID3D11DeviceContext* GHairWorksDeviceContext;;
				GHairWorksDeviceContext->PSSetShaderResources(Parameter.GetBaseIndex(), 1, &HairSrvs[HairSrvType]);
			};

			BindSrv(NvHw_resourceFaceHairIndices, NvHw::ShaderResourceType::HAIR_INDICES);
			BindSrv(NvHw_resourceTangents, NvHw::ShaderResourceType::TANGENTS);
			BindSrv(NvHw_resourceNormals, NvHw::ShaderResourceType::NORMALS);
			BindSrv(NvHw_resourceMasterPositions, NvHw::ShaderResourceType::MASTER_POSITIONS);
			BindSrv(NvHw_resourceMasterPrevPositions, NvHw::ShaderResourceType::PREV_MASTER_POSITIONS);
		}

		virtual bool Serialize(FArchive& Ar)
		{
			bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);

			Ar << HairConstantBuffer << TextureSampler << RootColorTexture << TipColorTexture << SpecularColorTexture << StrandTexture << NvHw_resourceFaceHairIndices << NvHw_resourceTangents << NvHw_resourceNormals << NvHw_resourceMasterPositions << NvHw_resourceMasterPrevPositions;

			return bShaderHasOutdatedParameters;
		}

		FShaderParameter HairConstantBuffer;

		FShaderResourceParameter TextureSampler;

		FShaderResourceParameter RootColorTexture;
		FShaderResourceParameter TipColorTexture;
		FShaderResourceParameter SpecularColorTexture;
		FShaderResourceParameter StrandTexture;

		FShaderResourceParameter	NvHw_resourceFaceHairIndices;
		FShaderResourceParameter	NvHw_resourceTangents;
		FShaderResourceParameter	NvHw_resourceNormals;
		FShaderResourceParameter	NvHw_resourceMasterPositions;
		FShaderResourceParameter	NvHw_resourceMasterPrevPositions;

	};

	class FHairWorksBasePassPs : public FHairWorksBasePs, public FHairWorksBaseShader
	{
		DECLARE_SHADER_TYPE(FHairWorksBasePassPs, Global);

		FHairWorksBasePassPs()
		{}

		FHairWorksBasePassPs(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
			: FHairWorksBasePs(Initializer)
		{
			IndirectLightingSHCoefficients.Bind(Initializer.ParameterMap, TEXT("IndirectLightingSHCoefficients"));
			PointSkyBentNormal.Bind(Initializer.ParameterMap, TEXT("PointSkyBentNormal"));
			CubemapShaderParameters.Bind(Initializer.ParameterMap);
			CubemapAmbient.Bind(Initializer.ParameterMap, TEXT("bCubemapAmbient"));
		}

		virtual bool Serialize(FArchive& Ar)
		{
			bool bShaderHasOutdatedParameters = FHairWorksBasePs::Serialize(Ar);

			Ar << IndirectLightingSHCoefficients << PointSkyBentNormal << CubemapShaderParameters << CubemapAmbient;

			return bShaderHasOutdatedParameters;
		}

		void SetParameters(FRHICommandListImmediate& RHICmdList, const FSceneView& View, const NvHw_ConstantBuffer& HairConstBuffer, const TArray<FTexture2DRHIRef>& HairTextures, ID3D11ShaderResourceView* HairSrvs[NvHw::ShaderResourceType::COUNT_OF], const FVector4 IndirectLight[3], const FVector4& InPointSkyBentNormal)
		{
			FHairWorksBasePs::SetParameters(RHICmdList, View, HairConstBuffer, HairTextures, HairSrvs);

			SetShaderValueArray(RHICmdList, GetPixelShader(), IndirectLightingSHCoefficients, IndirectLight, 3);
			SetShaderValue(RHICmdList, GetPixelShader(), PointSkyBentNormal, InPointSkyBentNormal);

			const bool bCubemapAmbient = View.FinalPostProcessSettings.ContributingCubemaps.Num() > 0;
			SetShaderValue(RHICmdList, GetPixelShader(), CubemapAmbient, bCubemapAmbient);
			if(bCubemapAmbient)
			{
				CubemapShaderParameters.SetParameters(RHICmdList, GetPixelShader(), View.FinalPostProcessSettings.ContributingCubemaps[0]);
			}
		}

		static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
		{
			FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		}

		FShaderParameter IndirectLightingSHCoefficients;
		FShaderParameter PointSkyBentNormal;
		FCubemapShaderParameters CubemapShaderParameters;
		FShaderParameter CubemapAmbient;
	};

	IMPLEMENT_SHADER_TYPE(, FHairWorksBasePassPs, TEXT("HairWorks"), TEXT("BasePassPs"), SF_Pixel);

	class FHairWorksColorizePs: public FHairWorksBasePs, public FHairWorksBaseShader
	{
		DECLARE_SHADER_TYPE(FHairWorksColorizePs, Global);

		FHairWorksColorizePs()
		{}

		FHairWorksColorizePs(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
			: FHairWorksBasePs(Initializer)
		{}

		using FHairWorksBasePs::SetParameters;

		static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
		{
			FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		}
	};

	IMPLEMENT_SHADER_TYPE(, FHairWorksColorizePs, TEXT("HairWorks"), TEXT("ColorizePs"), SF_Pixel);

	class FHairWorksShadowDepthPs : public FGlobalShader, public FHairWorksBaseShader
	{
		DECLARE_SHADER_TYPE(FHairWorksShadowDepthPs, Global);

		FHairWorksShadowDepthPs()
		{}

		FHairWorksShadowDepthPs(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
			: FGlobalShader(Initializer)
		{
			ShadowParams.Bind(Initializer.ParameterMap, TEXT("ShadowParams"));
		}

		bool Serialize(FArchive& Ar) override
		{
			bool bSerialized = FGlobalShader::Serialize(Ar);
			Ar << ShadowParams;
			return bSerialized;
		}

		static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
		{
			FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		}

		FShaderParameter ShadowParams;
	};

	IMPLEMENT_SHADER_TYPE(, FHairWorksShadowDepthPs, TEXT("HairWorks"), TEXT("ShadowDepthMain"), SF_Pixel);

	class FCopyDepthPs: public FGlobalShader, public FHairWorksBaseShader
	{
		DECLARE_SHADER_TYPE(FCopyDepthPs, Global);

		FCopyDepthPs()
		{}

		FCopyDepthPs(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
			: FGlobalShader(Initializer)
		{
			SceneDepthTexture.Bind(Initializer.ParameterMap, TEXT("SceneDepthTexture"));
		}

		virtual bool Serialize(FArchive& Ar)
		{
			bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
			Ar << SceneDepthTexture;
			return bShaderHasOutdatedParameters;
		}

		static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
		{
			FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		}

		FShaderResourceParameter SceneDepthTexture;
	};

	IMPLEMENT_SHADER_TYPE(, FCopyDepthPs, TEXT("HairWorks"), TEXT("CopyDepthPs"), SF_Pixel);

	class FResolveDepthShader: public FGlobalShader, public FHairWorksBaseShader	// Original class name is FResolveDepthPs. But it causes streaming error with FResolveDepthPS, which allocates numerous memory.
	{
		DECLARE_SHADER_TYPE(FResolveDepthShader, Global);

		FResolveDepthShader()
		{}

		FResolveDepthShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
			: FGlobalShader(Initializer)
		{
			DepthTexture.Bind(Initializer.ParameterMap, TEXT("DepthTexture"));
			StencilTexture.Bind(Initializer.ParameterMap, TEXT("StencilTexture"));
		}

		virtual bool Serialize(FArchive& Ar)
		{
			bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
			Ar << DepthTexture << StencilTexture;
			return bShaderHasOutdatedParameters;
		}

		static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
		{
			FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		}

		FShaderResourceParameter DepthTexture;
		FShaderResourceParameter StencilTexture;
	};

	IMPLEMENT_SHADER_TYPE(, FResolveDepthShader, TEXT("HairWorks"), TEXT("ResolveDepthPs"), SF_Pixel);

	class FResolveOpaqueDepthPs: public FGlobalShader, public FHairWorksBaseShader
	{
		DECLARE_SHADER_TYPE(FResolveOpaqueDepthPs, Global);

		FResolveOpaqueDepthPs()
		{}

		FResolveOpaqueDepthPs(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
			: FGlobalShader(Initializer)
		{
			DepthTexture.Bind(Initializer.ParameterMap, TEXT("DepthTexture"));
			HairColorTexture.Bind(Initializer.ParameterMap, TEXT("HairColorTexture"));
		}

		virtual bool Serialize(FArchive& Ar)
		{
			bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
			Ar << DepthTexture << HairColorTexture;
			return bShaderHasOutdatedParameters;
		}

		static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
		{
			FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		}

		FShaderResourceParameter DepthTexture;
		FShaderResourceParameter HairColorTexture;
	};

	IMPLEMENT_SHADER_TYPE(, FResolveOpaqueDepthPs, TEXT("HairWorks"), TEXT("ResolveOpaqueDepthPs"), SF_Pixel);

	class FCopyVelocityPs: public FGlobalShader, public FHairWorksBaseShader
	{
		DECLARE_SHADER_TYPE(FCopyVelocityPs, Global);

		FCopyVelocityPs()
		{}

		FCopyVelocityPs(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
			: FGlobalShader(Initializer)
		{
			VelocityTexture.Bind(Initializer.ParameterMap, TEXT("VelocityTexture"));
			DepthTexture.Bind(Initializer.ParameterMap, TEXT("DepthTexture"));
		}

		virtual bool Serialize(FArchive& Ar)
		{
			bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
			Ar << VelocityTexture << DepthTexture;
			return bShaderHasOutdatedParameters;
		}

		static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
		{
			FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		}

		FShaderResourceParameter VelocityTexture;
		FShaderResourceParameter DepthTexture;
	};

	IMPLEMENT_SHADER_TYPE(, FCopyVelocityPs, TEXT("HairWorks"), TEXT("CopyVelocityPs"), SF_Pixel);

	class FBlendLightingColorPs: public FGlobalShader, public FHairWorksBaseShader
	{
		DECLARE_SHADER_TYPE(FBlendLightingColorPs, Global);

		FBlendLightingColorPs()
		{}

		FBlendLightingColorPs(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
			: FGlobalShader(Initializer)
		{
			AccumulatedColorTexture.Bind(Initializer.ParameterMap, TEXT("AccumulatedColorTexture"));
			PrecomputedLightTexture.Bind(Initializer.ParameterMap, TEXT("PrecomputedLightTexture"));
		}

		virtual bool Serialize(FArchive& Ar)
		{
			bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
			Ar << AccumulatedColorTexture << PrecomputedLightTexture;
			return bShaderHasOutdatedParameters;
		}

		static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
		{
			FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		}

		FShaderResourceParameter AccumulatedColorTexture;
		FShaderResourceParameter PrecomputedLightTexture;
	};

	IMPLEMENT_SHADER_TYPE(, FBlendLightingColorPs, TEXT("HairWorks"), TEXT("BlendLightingColorPs"), SF_Pixel);

	// Constant buffer for per instance data
	IMPLEMENT_UNIFORM_BUFFER_STRUCT(FHairInstanceDataShaderUniform, TEXT("HairInstanceData"))

	// 
	template<typename FPixelShader, typename Funtion>
	void DrawFullScreen(FRHICommandListImmediate& RHICmdList, Funtion SetShaderParameters, bool bBlend = false, bool bDepth = false)
	{
		// Set render states
		RHICmdList.SetRasterizerState(GetStaticRasterizerState<false>(FM_Solid, CM_None));

		if(bDepth)
			RHICmdList.SetDepthStencilState(TStaticDepthStencilState<true, CF_Always>::GetRHI());
		else
			RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());

		if(bBlend)
			RHICmdList.SetBlendState(TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha>::GetRHI());
		else
			RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());

		// Set shader
		TShaderMapRef<FScreenVS> VertexShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
		TShaderMapRef<FPixelShader> PixelShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));

		static FGlobalBoundShaderState BoundShaderState;
		SetGlobalBoundShaderState(RHICmdList, ERHIFeatureLevel::SM5, BoundShaderState, GScreenVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

		// Set shader parameters
		SetShaderParameters(**PixelShader);

		// Draw
		const auto Size = FSceneRenderTargets::Get(RHICmdList).GetBufferSizeXY();
		RHICmdList.SetViewport(0, 0, 0, Size.X, Size.Y, 1);

		DrawRectangle(
			RHICmdList,
			0, 0,
			Size.X, Size.Y,
			0, 0,
			Size.X, Size.Y,
			Size,
			Size,
			*VertexShader
			);
	}

	void SetupViews(TArray<FViewInfo>& Views)
	{
		for(auto& View : Views)
		{
			check(View.VisibleHairs.Num() == 0);

			for(auto* PrimitiveInfo: View.VisibleDynamicPrimitives)
			{
				auto& ViewRelevance = View.PrimitiveViewRelevanceMap[PrimitiveInfo->GetIndex()];
				if(ViewRelevance.bHairWorks)
				{
					View.VisibleHairs.Add(PrimitiveInfo);
				}
			}
		}
	}

	void FindFreeElementInPool(FRHICommandList& RHICmdList, const FPooledRenderTargetDesc& Desc, TRefCountPtr<IPooledRenderTarget> &Out, const TCHAR* InDebugName)
	{
		// There is bug. When a render target is created from a existing pointer, AllocationLevelInKB is not decreased. This cause assertion failure in FRenderTargetPool::GetStats(). So we have to release it first.
		if(Out != nullptr)
		{
			if(!Out->GetDesc().Compare(Desc, true))
			{
				GRenderTargetPool.FreeUnusedResource(Out);
				Out = nullptr;
			}
		}

		GRenderTargetPool.FindFreeElement(RHICmdList, Desc, Out, InDebugName);

		// Release useless resolved render resource. Because of the reason mentioned above, we do in only in the macro.
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
		if(Out->GetDesc().NumSamples > 1)
			Out->GetRenderTargetItem().ShaderResourceTexture = nullptr;
#endif
	}

	// Create velocity buffer if necessary
	void AllocVelocityBuffer(FRHICommandList& RHICmdList, const TArray<FViewInfo>& Views)
	{
		HairRenderTargets->VelocityBuffer = nullptr;

		if (!CVarHairOutputVelocity.GetValueOnRenderThread())
			return;

		bool bNeedsVelocity = false;

		for(int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			const FViewInfo& View = Views[ViewIndex];

			bool bTemporalAA = (View.FinalPostProcessSettings.AntiAliasingMethod == AAM_TemporalAA) && !View.bCameraCut;
			bool bMotionBlur = IsMotionBlurEnabled(View);

			bNeedsVelocity |= bMotionBlur || bTemporalAA;
		}

		if(bNeedsVelocity)
		{
			check(HairRenderTargets->GBufferA);

			auto Desc = HairRenderTargets->GBufferA->GetDesc();
			Desc.Format = PF_G16R16;
			FindFreeElementInPool(RHICmdList, Desc, HairRenderTargets->VelocityBuffer, TEXT("HairGBufferC"));
		}
	}

	void AllocRenderTargets(FRHICommandList& RHICmdList, const FIntPoint& Size)
	{
		// Get MSAA level
		int SampleCount = CVarHairMsaaLevel.GetValueOnRenderThread();
		if(SampleCount >= 8)
			SampleCount = 8;
		else if(SampleCount >= 4)
			SampleCount = 4;
		else if(SampleCount >= 2)
			SampleCount = 2;
		else
			SampleCount = 1;

		// GBuffers
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(Size, PF_B8G8R8A8, FClearValueBinding::Transparent, TexCreate_None, TexCreate_RenderTargetable, false));
		Desc.NumSamples = SampleCount;
		FindFreeElementInPool(RHICmdList, Desc, HairRenderTargets->GBufferA, TEXT("HairGBufferA"));
		Desc.Flags |= ETextureCreateFlags::TexCreate_SRGB;	// SRGB for diffuse
		FindFreeElementInPool(RHICmdList, Desc, HairRenderTargets->GBufferB, TEXT("HairGBufferB"));
		Desc.Flags &= ~ETextureCreateFlags::TexCreate_SRGB;
		FindFreeElementInPool(RHICmdList, Desc, HairRenderTargets->GBufferC, TEXT("HairGBufferC"));
		Desc.Format = PF_FloatRGBA;
		FindFreeElementInPool(RHICmdList, Desc, HairRenderTargets->PrecomputedLight, TEXT("HairPrecomputedLight"));

		// Color buffer
		Desc.NumSamples = 1;
		Desc.Format = PF_FloatRGBA;
		FindFreeElementInPool(RHICmdList, Desc, HairRenderTargets->AccumulatedColor, TEXT("HairAccumulatedColor"));

		// Depth buffer
		Desc = FPooledRenderTargetDesc::Create2DDesc(Size, PF_DepthStencil, FClearValueBinding::DepthFar, TexCreate_None, TexCreate_DepthStencilTargetable, false);
		Desc.NumSamples = SampleCount;
		FindFreeElementInPool(RHICmdList, Desc, HairRenderTargets->HairDepthZ, TEXT("HairDepthZ"));

		HairRenderTargets->StencilSRV = RHICreateShaderResourceView((FTexture2DRHIRef&)HairRenderTargets->HairDepthZ->GetRenderTargetItem().TargetableTexture, 0, 1, PF_X24_G8);

		Desc.NumSamples = 1;
		FindFreeElementInPool(RHICmdList, Desc, HairRenderTargets->HairDepthZForShadow, TEXT("HairDepthZForShadow"));

		// Reset light attenuation
		HairRenderTargets->LightAttenuation = nullptr;
	}

	void CopySceneDepth(FRHICommandListImmediate& RHICmdList)
	{
		DrawFullScreen<FCopyDepthPs>(
			RHICmdList,
			[&](FCopyDepthPs& Shader){SetTextureParameter(RHICmdList, Shader.GetPixelShader(), Shader.SceneDepthTexture, FSceneRenderTargets::Get(RHICmdList).GetSceneDepthTexture()); },
			false,
			true
			);
	}

	bool ViewsHasHair(const TArray<FViewInfo>& Views)
	{
		for(auto& View : Views)
		{
			if(View.VisibleHairs.Num())
				return true;
		}

		return false;
	}

	void RenderBasePass(FRHICommandListImmediate& RHICmdList, TArray<FViewInfo>& Views)
	{
		// Clear accumulated color
		SCOPED_DRAW_EVENT(RHICmdList, RenderHairBasePass);

		SetRenderTarget(RHICmdList, HairRenderTargets->AccumulatedColor->GetRenderTargetItem().TargetableTexture, nullptr, ESimpleRenderTargetMode::EClearColorExistingDepth);

		// Prepare velocity buffer
		AllocVelocityBuffer(RHICmdList, Views);

		// Setup render targets
		FRHIRenderTargetView RenderTargetViews[5] = {
			FRHIRenderTargetView(HairRenderTargets->GBufferA->GetRenderTargetItem().TargetableTexture), 
			FRHIRenderTargetView(HairRenderTargets->GBufferB->GetRenderTargetItem().TargetableTexture), 
			FRHIRenderTargetView(HairRenderTargets->GBufferC->GetRenderTargetItem().TargetableTexture),
			FRHIRenderTargetView(HairRenderTargets->PrecomputedLight->GetRenderTargetItem().TargetableTexture),
			FRHIRenderTargetView(HairRenderTargets->VelocityBuffer ? HairRenderTargets->VelocityBuffer->GetRenderTargetItem().TargetableTexture : nullptr),
		};

		// UE4 doesn't clear all targets if there's a null render target in the array. So let's manually clear each of them.
		for(auto& RenderTarget : RenderTargetViews)
		{
			if(RenderTarget.Texture != nullptr)
				SetRenderTarget(RHICmdList, RenderTarget.Texture, nullptr, ESimpleRenderTargetMode::EClearColorExistingDepth);
		}

		FRHISetRenderTargetsInfo RenderTargetsInfo(5, RenderTargetViews, FRHIDepthRenderTargetView(HairRenderTargets->HairDepthZ->GetRenderTargetItem().TargetableTexture));
		RenderTargetsInfo.SetClearDepthStencil(true, 0);

		RHICmdList.SetRenderTargetsAndClear(RenderTargetsInfo);

		// Copy scene depth to hair depth buffer.
		DrawFullScreen<FCopyDepthPs>(
			RHICmdList,
			[&](FCopyDepthPs& Shader){SetTextureParameter(RHICmdList, Shader.GetPixelShader(), Shader.SceneDepthTexture, FSceneRenderTargets::Get(RHICmdList).GetSceneDepthTexture()); },
			false,
			true
			);

		// Render states
		RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());
		auto DepthStencilState = TStaticDepthStencilState<true, CF_GreaterEqual, true, CF_Always, SO_Keep, SO_Keep, SO_Replace, true, CF_Always, SO_Keep, SO_Keep, SO_Replace>::GetRHI();

		// Draw hairs
		FHairInstanceDataShaderUniform HairShaderUniformStruct;
		TArray<TPair<FHairWorksSceneProxy*, int>, SceneRenderingAllocator> HairStencilValues;	// We use the same stencil value for a hair existing in multiple views

		for(auto& View : Views)
		{
			// Set render states
			const auto& ViewRect = View.ViewRect;

			RHICmdList.SetViewport(ViewRect.Min.X, ViewRect.Min.Y, 0, ViewRect.Max.X, ViewRect.Max.Y, 1);
			// Draw hair instances
			int NewStencilValue = 1;
			HairStencilValues.Reserve(View.VisibleHairs.Num());

			for(auto& PrimitiveInfo : View.VisibleHairs)
			{
				auto& HairSceneProxy = static_cast<FHairWorksSceneProxy&>(*PrimitiveInfo->Proxy);
				if(HairSceneProxy.GetHairInstanceId() == NvHw::HAIR_INSTANCE_ID_NULL)
					continue;

				// Skip colorize
				NvHw::HairInstanceDescriptor HairDescriptor;
				GHairWorksSDK->getInstanceDescriptor(HairSceneProxy.GetHairInstanceId(), HairDescriptor);

				if(HairDescriptor.m_colorizeMode != NvHw::ColorizeMode::NONE)
				{
					if(View.Family->EngineShowFlags.CompositeEditorPrimitives)
					{
						continue;
					}
					else
					{
						HairDescriptor.m_colorizeMode = NvHw::ColorizeMode::NONE;
						GHairWorksSDK->updateInstanceDescriptor(HairSceneProxy.GetHairInstanceId(), HairDescriptor);
					}
				}

				// Find stencil value for this hair
				auto* UsedStencil = HairStencilValues.FindByPredicate(
					[&](const TPair<FHairWorksSceneProxy*, int>& HairAndStencil)
				{
					return HairAndStencil.Key == &HairSceneProxy;
				}
				);

				int StencilValue;

				if(UsedStencil != nullptr)
				{
					StencilValue = UsedStencil->Value;
				}
				else
				{
					StencilValue = NewStencilValue;

					// Add for later use
					HairStencilValues.Add(TPairInitializer<FHairWorksSceneProxy*, int>(&HairSceneProxy, StencilValue));

					// Accumulate stencil value
					check(NewStencilValue <= UCHAR_MAX);
					NewStencilValue = (NewStencilValue + 1) % HairInstanceMaterialArraySize;
				}

				// Set stencil state
				RHICmdList.SetDepthStencilState(DepthStencilState, StencilValue);

				// Setup hair instance data uniform
				HairShaderUniformStruct.Spec0_SpecPower0_Spec1_SpecPower1[StencilValue] = FVector4(
					HairDescriptor.m_specularPrimary,
					HairDescriptor.m_specularPowerPrimary,
					HairDescriptor.m_specularSecondary,
					HairDescriptor.m_specularPowerSecondary
					);
				HairShaderUniformStruct.Spec1Offset_DiffuseBlend_ReceiveShadows_ShadowSigma[StencilValue] = FVector4(
					HairDescriptor.m_specularSecondaryOffset,
					HairDescriptor.m_diffuseBlend,
					HairDescriptor.m_receiveShadows,
					HairDescriptor.m_shadowSigma * (254.f / 255.f)
					);
				HairShaderUniformStruct.GlintStrength[StencilValue] = FVector4(
					HairDescriptor.m_glintStrength
					);

				// Pass camera information
				auto ViewMatrices = View.ViewMatrices;

				GHairWorksSDK->setViewProjection(reinterpret_cast<const gfsdk_float4x4&>(ViewMatrices.ViewMatrix.M), reinterpret_cast<const gfsdk_float4x4&>(ViewMatrices.ProjMatrix.M), NvHw::HandednessHint::LEFT);
				GHairWorksSDK->setPrevViewProjection(reinterpret_cast<const gfsdk_float4x4&>(View.PrevViewMatrices.ViewMatrix.M), reinterpret_cast<const gfsdk_float4x4&>(View.PrevViewMatrices.ProjMatrix.M), NvHw::HandednessHint::LEFT);

				// Setup shader
				TShaderMapRef<FScreenVS> VertexShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				TShaderMapRef<FHairWorksBasePassPs> PixelShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));

				static FGlobalBoundShaderState BoundShaderState;

				SetGlobalBoundShaderState(
					RHICmdList,
					ERHIFeatureLevel::SM5,
					BoundShaderState,
					GSimpleElementVertexDeclaration.VertexDeclarationRHI,
					*VertexShader,
					*PixelShader
					);

				// Setup shader constants
				FVector4 IndirectLight[sizeof(FSHVectorRGB2) / sizeof(FVector4)] = {FVector4(0, 0, 0, 0), FVector4(0, 0, 0, 0), FVector4(0, 0, 0, 0)};
				FVector4 SkyBentNormal(0, 0, 0, 1);
				if(PrimitiveInfo->IndirectLightingCacheAllocation != nullptr
					&& PrimitiveInfo->IndirectLightingCacheAllocation->IsValid()
					)
				{
					const FIndirectLightingCacheAllocation& LightingAllocation = *PrimitiveInfo->IndirectLightingCacheAllocation;
					if(View.Family->EngineShowFlags.GlobalIllumination)
					{
						IndirectLight[0] = LightingAllocation.SingleSamplePacked[0];
						IndirectLight[1] = LightingAllocation.SingleSamplePacked[1];
						IndirectLight[2] = LightingAllocation.SingleSamplePacked[2];
					}
					SkyBentNormal = LightingAllocation.CurrentSkyBentNormal;
				}

				NvHw::ShaderConstantBuffer ConstantBuffer;
				GHairWorksSDK->prepareShaderConstantBuffer(HairSceneProxy.GetHairInstanceId(), ConstantBuffer);

				ID3D11ShaderResourceView* HairSrvs[NvHw::ShaderResourceType::COUNT_OF];
				GHairWorksSDK->getShaderResources(HairSceneProxy.GetHairInstanceId(), NV_NULL, NvHw::ShaderResourceType::COUNT_OF, Nv::Dx11Type::getPtr(HairSrvs));

				PixelShader->SetParameters(RHICmdList, View, reinterpret_cast<NvHw_ConstantBuffer&>(ConstantBuffer), HairSceneProxy.GetTextures(), HairSrvs, IndirectLight, SkyBentNormal);

				// Flush render states
				RHICmdList.DrawPrimitive(0, 0, 0, 0);

				// Draw
				HairSceneProxy.Draw();
			}
		}

		// Setup hair materials lookup table
		HairRenderTargets->HairInstanceDataShaderUniform = TUniformBufferRef<FHairInstanceDataShaderUniform>::CreateUniformBufferImmediate(HairShaderUniformStruct, UniformBuffer_SingleFrame);
		
		// Copy hair depth to receive shadow
		SetRenderTarget(RHICmdList, nullptr, HairRenderTargets->HairDepthZForShadow->GetRenderTargetItem().TargetableTexture);

		DrawFullScreen<FResolveDepthShader>(
			RHICmdList,
			[&](FResolveDepthShader& Shader)
		{
			SetTextureParameter(RHICmdList, Shader.GetPixelShader(), Shader.DepthTexture, HairRenderTargets->HairDepthZ->GetRenderTargetItem().TargetableTexture); 
			SetSRVParameter(RHICmdList, Shader.GetPixelShader(), Shader.StencilTexture, HairRenderTargets->StencilSRV);
		},
			false,
			true
			);

		// Copy depth for translucency occlusion
		SetRenderTarget(RHICmdList, nullptr, FSceneRenderTargets::Get(RHICmdList).GetSceneDepthSurface());

		DrawFullScreen<FResolveOpaqueDepthPs>(
			RHICmdList,
			[&](FResolveOpaqueDepthPs& Shader)
		{
			SetTextureParameter(RHICmdList, Shader.GetPixelShader(), Shader.DepthTexture, HairRenderTargets->HairDepthZ->GetRenderTargetItem().TargetableTexture);
			SetTextureParameter(RHICmdList, Shader.GetPixelShader(), Shader.HairColorTexture, HairRenderTargets->PrecomputedLight->GetRenderTargetItem().TargetableTexture);
		},
			false,
			true
			);
	}

	void RenderVelocities(FRHICommandListImmediate& RHICmdList, TRefCountPtr<IPooledRenderTarget>& VelocityRT)
	{
		// Resolve MSAA velocity
		if(!HairRenderTargets->VelocityBuffer)
			return;

		auto SetShaderParameters = [&](FCopyVelocityPs& Shader)
		{
			SetTextureParameter(RHICmdList, Shader.GetPixelShader(), Shader.VelocityTexture, HairRenderTargets->VelocityBuffer->GetRenderTargetItem().TargetableTexture);
			SetTextureParameter(RHICmdList, Shader.GetPixelShader(), Shader.DepthTexture, HairRenderTargets->HairDepthZ->GetRenderTargetItem().TargetableTexture);
		};

		DrawFullScreen<FCopyVelocityPs>(RHICmdList, SetShaderParameters);
	}

	void BeginRenderingSceneColor(FRHICommandListImmediate& RHICmdList)
	{
		FTextureRHIParamRef RenderTargetsRHIs[2] = {
			FSceneRenderTargets::Get(RHICmdList).GetSceneColorSurface(),
			HairRenderTargets->AccumulatedColor->GetRenderTargetItem().TargetableTexture,
		};

		SetRenderTargets(RHICmdList, 2, RenderTargetsRHIs, FSceneRenderTargets::Get(RHICmdList).GetSceneDepthSurface(), ESimpleRenderTargetMode::EExistingColorAndDepth, FExclusiveDepthStencil::DepthRead_StencilWrite);
	}

	void BlendLightingColor(FRHICommandListImmediate& RHICmdList)
	{
		FSceneRenderTargets::Get(RHICmdList).BeginRenderingSceneColor(RHICmdList);

		DrawFullScreen<FBlendLightingColorPs>(
			RHICmdList,
			[&](FBlendLightingColorPs& Shader)
		{
			SetTextureParameter(RHICmdList, Shader.GetPixelShader(), Shader.AccumulatedColorTexture, HairRenderTargets->AccumulatedColor->GetRenderTargetItem().TargetableTexture);
			SetTextureParameter(RHICmdList, Shader.GetPixelShader(), Shader.PrecomputedLightTexture, HairRenderTargets->PrecomputedLight->GetRenderTargetItem().TargetableTexture);
		},
			true
			);
	}

	void RenderVisualization(FRHICommandListImmediate& RHICmdList, const FViewInfo& View)
	{
		// Render hairs
		SCOPED_DRAW_EVENT(RHICmdList, RenderHairVisualization);

		const auto& ViewRect = View.ViewRect;

		// Setup render state
		RHICmdList.SetDepthStencilState(TStaticDepthStencilState<>::GetRHI());

		// Setup shader for colorize
		TShaderMapRef<FScreenVS> VertexShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
		TShaderMapRef<FHairWorksColorizePs> PixelShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));

		static FGlobalBoundShaderState BoundShaderState;

		SetGlobalBoundShaderState(
			RHICmdList,
			ERHIFeatureLevel::SM5,
			BoundShaderState,
			GSimpleElementVertexDeclaration.VertexDeclarationRHI,
			*VertexShader,
			*PixelShader
			);

		// Render colorize
		for(auto& PrimitiveInfo : View.VisibleHairs)
		{
			// Skin none colorize
			auto& HairSceneProxy = static_cast<FHairWorksSceneProxy&>(*PrimitiveInfo->Proxy);

			NvHw::HairInstanceDescriptor HairDescriptor;
			GHairWorksSDK->getInstanceDescriptor(HairSceneProxy.GetHairInstanceId(), HairDescriptor);

			if (HairDescriptor.m_colorizeMode == NvHw::ColorizeMode::NONE)
				continue;

			// Setup camera
			GHairWorksSDK->setViewProjection(reinterpret_cast<const gfsdk_float4x4&>(View.ViewMatrices.ViewMatrix), reinterpret_cast<const gfsdk_float4x4&>(View.ViewMatrices.ProjMatrix), NvHw::HandednessHint::LEFT);

			// Setup shader constants
			NvHw::ShaderConstantBuffer ConstantBuffer;
			GHairWorksSDK->prepareShaderConstantBuffer(HairSceneProxy.GetHairInstanceId(), ConstantBuffer);

			ID3D11ShaderResourceView* HairSrvs[NvHw::ShaderResourceType::COUNT_OF];
			GHairWorksSDK->getShaderResources(HairSceneProxy.GetHairInstanceId(), NV_NULL, NvHw::ShaderResourceType::COUNT_OF, Nv::Dx11Type::getPtr(HairSrvs));

			PixelShader->SetParameters(RHICmdList, View, reinterpret_cast<NvHw_ConstantBuffer&>(ConstantBuffer), HairSceneProxy.GetTextures(), HairSrvs);

			// Flush render states
			RHICmdList.DrawPrimitive(0, 0, 0, 0);

			// Draw
			HairSceneProxy.Draw(FHairWorksSceneProxy::EDrawType::Normal);
		}

		// Render visualization
		for(auto& PrimitiveInfo : View.VisibleHairs)
		{
			// Draw hair
			auto& HairSceneProxy = static_cast<FHairWorksSceneProxy&>(*PrimitiveInfo->Proxy);

			GHairWorksSDK->setViewProjection(reinterpret_cast<const gfsdk_float4x4&>(View.ViewMatrices.ViewMatrix), reinterpret_cast<const gfsdk_float4x4&>(View.ViewMatrices.ProjMatrix), NvHw::HandednessHint::LEFT);

			HairSceneProxy.Draw(FHairWorksSceneProxy::EDrawType::Visualization);
		}
	}

	void StepSimulation()
	{
		if (GHairWorksSDK == nullptr)
			return;

		static uint32 LastFrameNumber = -1;
		if (LastFrameNumber != GFrameNumberRenderThread)
		{
			LastFrameNumber = GFrameNumberRenderThread;

			GHairWorksSDK->stepSimulation();
		}
	}

	void RenderShadow(FRHICommandListImmediate& RHICmdList, const FProjectedShadowInfo& Shadow, const FProjectedShadowInfo::PrimitiveArrayType& SubjectPrimitives, const FViewInfo& View)
	{
		SCOPED_DRAW_EVENT(RHICmdList, RenderHairShadow);

		for (auto PrimitiveIdx = 0; PrimitiveIdx < SubjectPrimitives.Num(); ++PrimitiveIdx)
		{
			// Skip
			auto* PrimitiveInfo = SubjectPrimitives[PrimitiveIdx];
			auto& ViewRelavence = View.PrimitiveViewRelevanceMap[PrimitiveInfo->GetIndex()];
			if (!ViewRelavence.bHairWorks)
				continue;

			auto& HairSceneProxy = static_cast<FHairWorksSceneProxy&>(*PrimitiveInfo->Proxy);
			if (HairSceneProxy.GetHairInstanceId() == NvHw::HairInstanceId::HAIR_INSTANCE_ID_NULL)
				continue;

			NvHw::HairInstanceDescriptor HairDesc;
			GHairWorksSDK->getInstanceDescriptor(HairSceneProxy.GetHairInstanceId(), HairDesc);
			if(!HairDesc.m_castShadows)
				continue;

			// Setup render states and shaders
			TShaderMapRef<FScreenVS> VertexShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));

			if(Shadow.CascadeSettings.bOnePassPointLightShadow)
			{
				// Setup camera
				const FBoxSphereBounds& PrimitiveBounds = HairSceneProxy.GetBounds();

				FViewMatrices ViewMatrices[6];
				bool Visible[6];
				for (int32 FaceIndex = 0; FaceIndex < 6; FaceIndex++)
				{
					ViewMatrices[FaceIndex].ViewMatrix = Shadow.OnePassShadowViewProjectionMatrices[FaceIndex];
					Visible[FaceIndex] = Shadow.OnePassShadowFrustums[FaceIndex].IntersectBox(PrimitiveBounds.Origin, PrimitiveBounds.BoxExtent);
				}

				gfsdk_float4x4 HairViewMatrices[6];
				gfsdk_float4x4 HairProjMatrices[6];
				for (int FaceIdx = 0; FaceIdx < 6; ++FaceIdx)
				{
					HairViewMatrices[FaceIdx] = *(gfsdk_float4x4*)ViewMatrices[FaceIdx].ViewMatrix.M;
					HairProjMatrices[FaceIdx] = *(gfsdk_float4x4*)ViewMatrices[FaceIdx].ProjMatrix.M;
				}

				GHairWorksSDK->setCubeMapViewProjection(HairViewMatrices, HairProjMatrices, Visible, NvHw::HandednessHint::LEFT);

				// Setup shader
				static FGlobalBoundShaderState BoundShaderState;
				SetGlobalBoundShaderState(RHICmdList, ERHIFeatureLevel::SM5, BoundShaderState, GSimpleElementVertexDeclaration.VertexDeclarationRHI,
					*VertexShader, nullptr);
			}
			else
			{
				// Setup camera
				FViewMatrices ViewMatrices;
				ViewMatrices.ViewMatrix = FTranslationMatrix(Shadow.PreShadowTranslation) * Shadow.SubjectAndReceiverMatrix;
				GHairWorksSDK->setViewProjection(reinterpret_cast<const gfsdk_float4x4&>(ViewMatrices.ViewMatrix.M), reinterpret_cast<const gfsdk_float4x4&>(ViewMatrices.ProjMatrix.M), NvHw::HandednessHint::LEFT);

				// Setup shader
				TShaderMapRef<FHairWorksShadowDepthPs> PixelShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));

				static FGlobalBoundShaderState BoundShaderState;
				SetGlobalBoundShaderState(RHICmdList, ERHIFeatureLevel::SM5, BoundShaderState, GSimpleElementVertexDeclaration.VertexDeclarationRHI,
					*VertexShader, *PixelShader);

				SetShaderValue(RHICmdList, PixelShader->GetPixelShader(), PixelShader->ShadowParams, FVector2D(Shadow.GetShaderDepthBias() * CVarHairShadowBiasScale.GetValueOnRenderThread(), Shadow.InvMaxSubjectDepth));
			}

			// Flush render states
			RHICmdList.DrawPrimitive(0, 0, 0, 0);

			// Draw hair
			HairSceneProxy.Draw(FHairWorksSceneProxy::EDrawType::Shadow);
		}
	}
}
// @third party code - END HairWorks
