// @third party code - BEGIN HairWorks
#include "EnginePrivate.h"
#include "HairWorksSDK.h"
#include "Engine/HairWorksAsset.h"
#include "HairWorksSceneProxy.h"

// Debug render console variables.
#define HairVisualizationCVarDefine(Name)	\
	static TAutoConsoleVariable<int> CVarHairVisualization##Name(TEXT("r.HairWorks.Visualization."#Name),	0, TEXT(""), ECVF_RenderThreadSafe)

static TAutoConsoleVariable<int> CVarHairVisualizationHair(TEXT("r.HairWorks.Visualization.")TEXT("Hair"), 1, TEXT(""), ECVF_RenderThreadSafe);
HairVisualizationCVarDefine(GuideCurves);
HairVisualizationCVarDefine(SkinnedGuideCurves);
HairVisualizationCVarDefine(ControlPoints);
HairVisualizationCVarDefine(GrowthMesh);
HairVisualizationCVarDefine(Bones);
HairVisualizationCVarDefine(BoundingBox);
HairVisualizationCVarDefine(CollisionCapsules);
HairVisualizationCVarDefine(HairInteraction);
HairVisualizationCVarDefine(PinConstraints);
HairVisualizationCVarDefine(ShadingNormal);
HairVisualizationCVarDefine(ShadingNormalCenter);

#undef HairVisualizationCVarDefine

FHairWorksSceneProxy::FHairWorksSceneProxy(const UPrimitiveComponent* InComponent, UHairWorksAsset& InHair) :
	FPrimitiveSceneProxy(InComponent),
	Hair(InHair),
	HairInstanceId(GFSDK_HairInstanceID_NULL)
{
}

FHairWorksSceneProxy::~FHairWorksSceneProxy()
{
	if (HairInstanceId != GFSDK_HairInstanceID_NULL)
		GHairWorksSDK->FreeHairInstance(HairInstanceId);
}

uint32 FHairWorksSceneProxy::GetMemoryFootprint(void) const
{
	return 0;
}

void FHairWorksSceneProxy::Draw(EDrawType DrawType)const
{
	if(HairInstanceId == GFSDK_HairInstanceID_NULL)
		return;

	if (DrawType == EDrawType::Visualization)
	{
		GHairWorksSDK->RenderVisualization(HairInstanceId);
	}
	else
	{
		// Special for shadow
		GFSDK_HairInstanceDescriptor HairDesc;
		GHairWorksSDK->CopyCurrentInstanceDescriptor(HairInstanceId, HairDesc);

		if (DrawType == EDrawType::Shadow)
		{
			HairDesc.m_useBackfaceCulling = false;

			GHairWorksSDK->UpdateInstanceDescriptor(HairInstanceId, HairDesc);
		}

		// Handle shader cache.
		GFSDK_HairShaderCacheSettings ShaderCacheSetting;
		ShaderCacheSetting.SetFromInstanceDescriptor(HairDesc);
		check(HairTextures.Num() == GFSDK_HAIR_NUM_TEXTURES);
		for(int i = 0; i < GFSDK_HAIR_NUM_TEXTURES; i++)
		{
			ShaderCacheSetting.isTextureUsed[i] = (HairTextures[i] != nullptr);
		}

		GHairWorksSDK->AddToShaderCache(ShaderCacheSetting);

		// Draw
		GFSDK_HairShaderSettings HairShaderSettings;
		HairShaderSettings.m_useCustomConstantBuffer = true;
		HairShaderSettings.m_shadowPass = (DrawType == EDrawType::Shadow);

		GHairWorksSDK->RenderHairs(HairInstanceId, &HairShaderSettings);
	}
}

void FHairWorksSceneProxy::CreateRenderThreadResources()
{
	FPrimitiveSceneProxy::CreateRenderThreadResources();

	if (GHairWorksSDK == nullptr)
		return;

	// Initialize hair asset
	if (Hair.AssetId == GFSDK_HairAssetID_NULL)
	{
		// Create hair asset
		GHairWorksSDK->LoadHairAssetFromMemory(Hair.AssetData.GetData(), Hair.AssetData.Num(), &Hair.AssetId, nullptr, &GHairWorksConversionSettings);
	}

	// Setup bone look up table
	BoneNameToIdx.Empty(Hair.BoneNames.Num());
	for(auto Idx = 0; Idx < Hair.BoneNames.Num(); ++Idx)
	{
		BoneNameToIdx.Add(Hair.BoneNames[Idx], Idx);
	}

	// Initialize hair instance
	GHairWorksSDK->CreateHairInstance(Hair.AssetId, &HairInstanceId);
}

FPrimitiveViewRelevance FHairWorksSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance ViewRel;
	ViewRel.bDrawRelevance = IsShown(View);
	ViewRel.bShadowRelevance = IsShadowCast(View);
	ViewRel.bDynamicRelevance = true;
	ViewRel.bRenderInMainPass = false;	// Hair is rendered in a special path.

	ViewRel.bHairWorks = View->Family->EngineShowFlags.HairWorks;

	return ViewRel;
}

void FHairWorksSceneProxy::UpdateDynamicData_RenderThread(const FDynamicRenderData & DynamicData)
{
	if (HairInstanceId == GFSDK_HairInstanceID_NULL)
		return;

	// Update bones
	GHairWorksSDK->UpdateSkinningMatrices(HairInstanceId, DynamicData.BoneMatrices.Num(), (gfsdk_float4x4*)DynamicData.BoneMatrices.GetData());

	// Update normal center bone
	auto HairDesc = DynamicData.HairInstanceDesc;

	auto* BoneIdx = BoneNameToIdx.Find(DynamicData.NormalCenterBoneName);
	if(BoneIdx != nullptr)
		HairDesc.m_hairNormalBoneIndex = *BoneIdx;
	else
		HairDesc.m_hairNormalWeight = 0;

	// Merge global visualization flags.
#define HairVisualizationCVarUpdate(CVarName, MemberVarName)	\
	HairDesc.m_visualize##MemberVarName |= CVarHairVisualization##CVarName.GetValueOnRenderThread() != 0

	HairDesc.m_drawRenderHairs &= CVarHairVisualizationHair.GetValueOnRenderThread() != 0;
	HairVisualizationCVarUpdate(GuideCurves, GuideHairs);
	HairVisualizationCVarUpdate(SkinnedGuideCurves, SkinnedGuideHairs);
	HairVisualizationCVarUpdate(ControlPoints, ControlVertices);
	HairVisualizationCVarUpdate(GrowthMesh, GrowthMesh);
	HairVisualizationCVarUpdate(Bones, Bones);
	HairVisualizationCVarUpdate(BoundingBox, BoundingBox);
	HairVisualizationCVarUpdate(CollisionCapsules, Capsules);
	HairVisualizationCVarUpdate(HairInteraction, HairInteractions);
	HairVisualizationCVarUpdate(PinConstraints, PinConstraints);
	HairVisualizationCVarUpdate(ShadingNormal, ShadingNormals);
	HairVisualizationCVarUpdate(ShadingNormalCenter, ShadingNormalBone);

#undef HairVisualizerCVarUpdate

	// Other
	HairDesc.m_modelToWorld = (gfsdk_float4x4&)GetLocalToWorld().M;
	HairDesc.m_useViewfrustrumCulling = false;

	// Set parameters to HairWorks
	GHairWorksSDK->UpdateInstanceDescriptor(HairInstanceId, HairDesc);	// Mainly for simulation.

	// Update textures
	check(DynamicData.Textures.Num() == GFSDK_HAIR_NUM_TEXTURES);
	HairTextures.SetNumZeroed(GFSDK_HAIR_NUM_TEXTURES);
	for(auto Idx = 0; Idx < HairTextures.Num(); ++Idx)
	{
		auto* Texture = DynamicData.Textures[Idx];
		if(Texture == nullptr)
			continue;

		if(Texture->Resource == nullptr)
			continue;

		HairTextures[Idx] = static_cast<FTexture2DResource*>(Texture->Resource)->GetTexture2DRHI();
	}

	for (auto Idx = 0; Idx < GFSDK_HAIR_NUM_TEXTURES; ++Idx)
	{
		extern ID3D11ShaderResourceView* (*HairWorksGetSrvFromTexture)(FRHITexture2D*);

		auto TextureRef = HairTextures[Idx];
		GHairWorksSDK->SetTextureSRV(HairInstanceId, (GFSDK_HAIR_TEXTURE_TYPE)Idx, HairWorksGetSrvFromTexture(TextureRef.GetReference()));
	}
}
// @third party code - END HairWorks
