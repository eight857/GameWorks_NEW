// @third party code - BEGIN HairWorks
#pragma once

namespace HairWorksRenderer
{
	void StepSimulation();
	void SetupViews(TArray<FViewInfo>& Views);
	bool ViewsHasHair(const TArray<FViewInfo>& Views);
	void AllocRenderTargets(FRHICommandList& RHICmdList, const FIntPoint& Size);
	void RenderBasePass(FRHICommandListImmediate& RHICmdList, TArray<FViewInfo>& Views);
	void RenderShadow(FRHICommandListImmediate& RHICmdList, const FProjectedShadowInfo& Shadow,const FProjectedShadowInfo::PrimitiveArrayType& SubjectPrimitives, const FViewInfo& View);
	void RenderVelocities(FRHICommandListImmediate& RHICmdList, TRefCountPtr<IPooledRenderTarget>& VelocityRT);
	void RenderVisualization(FRHICommandListImmediate& RHICmdList, const FViewInfo& View);
	void BeginRenderingSceneColor(FRHICommandListImmediate& RHICmdList);	// Add a render target for hair
	void BlendLightingColor(FRHICommandListImmediate& RHICmdList);

	static const int HairInstanceMaterialArraySize = 128;

	BEGIN_UNIFORM_BUFFER_STRUCT(FHairInstanceDataShaderUniform, )	// We would use array of structure instead of array of raw float in future.
		DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY(FVector4, Spec0_SpecPower0_Spec1_SpecPower1, [HairInstanceMaterialArraySize])
		DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY(FVector4, Spec1Offset_DiffuseBlend_ReceiveShadows_ShadowSigma, [HairInstanceMaterialArraySize])
		DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY(FVector4, GlintStrength, [HairInstanceMaterialArraySize])
	END_UNIFORM_BUFFER_STRUCT(FHairInstanceDataShaderUniform)

	class FRenderTargets
	{
	public:
		TRefCountPtr<IPooledRenderTarget> GBufferA;
		TRefCountPtr<IPooledRenderTarget> GBufferB;
		TRefCountPtr<IPooledRenderTarget> GBufferC;
		TRefCountPtr<IPooledRenderTarget> HairDepthZ;
		TRefCountPtr<IPooledRenderTarget> HairDepthZForShadow;
		TRefCountPtr<FRHIShaderResourceView> StencilSRV;
		TRefCountPtr<IPooledRenderTarget> LightAttenuation;
		TRefCountPtr<IPooledRenderTarget> VelocityBuffer;
		TRefCountPtr<IPooledRenderTarget> PrecomputedLight;
		TRefCountPtr<IPooledRenderTarget> AccumulatedColor;

		TUniformBufferRef<FHairInstanceDataShaderUniform> HairInstanceDataShaderUniform;
	};

	extern TSharedRef<FRenderTargets> HairRenderTargets;
}
// @third party code - END HairWorks
