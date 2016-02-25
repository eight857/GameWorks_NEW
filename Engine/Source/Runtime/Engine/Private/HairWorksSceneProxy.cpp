// @third party code - BEGIN HairWorks
#include "EnginePrivate.h"
#include <Nv/Foundation/NvMemoryReadStream.h>
#include "AllowWindowsPlatformTypes.h"
#include <Nv/Platform/Dx11/Foundation/NvDx11Handle.h>
#include "HideWindowsPlatformTypes.h"
#include "HairWorksSDK.h"
#include "Engine/HairWorksAsset.h"
#include "HairWorksSceneProxy.h"

// Debug render console variables.
#define HairVisualizationCVarDefine(Name)	\
	static TAutoConsoleVariable<int> CVarHairVisualization##Name(TEXT("r.HairWorks.Visualization.") TEXT(#Name), 0, TEXT(""), ECVF_RenderThreadSafe)

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
	HairInstanceId(NvHw::HAIR_INSTANCE_ID_NULL)
{
}

FHairWorksSceneProxy::~FHairWorksSceneProxy()
{
	if (HairInstanceId != NvHw::HAIR_INSTANCE_ID_NULL)
		GHairWorksSDK->freeHairInstance(HairInstanceId);
}

uint32 FHairWorksSceneProxy::GetMemoryFootprint(void) const
{
	return 0;
}

void FHairWorksSceneProxy::Draw(EDrawType DrawType)const
{
	if(HairInstanceId == NvHw::HAIR_INSTANCE_ID_NULL)
		return;

	if (DrawType == EDrawType::Visualization)
	{
		GHairWorksSDK->renderVisualization(HairInstanceId);
	}
	else
	{
		// Special for shadow
		NvHw::HairInstanceDescriptor HairDesc;
		GHairWorksSDK->getInstanceDescriptor(HairInstanceId, HairDesc);

		if (DrawType == EDrawType::Shadow)
		{
			HairDesc.m_useBackfaceCulling = false;

			GHairWorksSDK->updateInstanceDescriptor(HairInstanceId, HairDesc);
		}

		// Handle shader cache.
		NvHw::ShaderCacheSettings ShaderCacheSetting;
		ShaderCacheSetting.setFromInstanceDescriptor(HairDesc);
		check(HairTextures.Num() == NvHw::EHairTextureType::COUNT_OF);
		for(int i = 0; i < NvHw::EHairTextureType::COUNT_OF; i++)
		{
			ShaderCacheSetting.setTextureUsed(i, HairTextures[i] != nullptr);
		}

		GHairWorksSDK->addToShaderCache(ShaderCacheSetting);

		// Draw
		NvHw::ShaderSettings HairShaderSettings;
		HairShaderSettings.m_useCustomConstantBuffer = true;
		HairShaderSettings.m_shadowPass = (DrawType == EDrawType::Shadow);

		GHairWorksSDK->renderHairs(HairInstanceId, &HairShaderSettings);
	}
}

void FHairWorksSceneProxy::CreateRenderThreadResources()
{
	FPrimitiveSceneProxy::CreateRenderThreadResources();

	if (GHairWorksSDK == nullptr)
		return;

	// Initialize hair asset
	if (Hair.AssetId == NvHw::HAIR_ASSET_ID_NULL)
	{
		// Create hair asset
		Nv::MemoryReadStream ReadStream(Hair.AssetData.GetData(), Hair.AssetData.Num());
		GHairWorksSDK->loadHairAsset(&ReadStream, Hair.AssetId, nullptr, &GHairWorksConversionSettings);
	}

	// Setup bone look up table
	BoneNameToIdx.Empty(Hair.BoneNames.Num());
	for(auto Idx = 0; Idx < Hair.BoneNames.Num(); ++Idx)
	{
		BoneNameToIdx.Add(Hair.BoneNames[Idx], Idx);
	}

	// Initialize hair instance
	GHairWorksSDK->createHairInstance(Hair.AssetId, HairInstanceId);
}

FPrimitiveViewRelevance FHairWorksSceneProxy::GetViewRelevance(const FSceneView* View)const
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
	if (HairInstanceId == NvHw::HAIR_INSTANCE_ID_NULL)
		return;

	// Update bones
	GHairWorksSDK->updateSkinningMatrices(HairInstanceId, DynamicData.BoneMatrices.Num(), (gfsdk_float4x4*)DynamicData.BoneMatrices.GetData());

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
	GHairWorksSDK->updateInstanceDescriptor(HairInstanceId, HairDesc);	// Mainly for simulation.

	// Update textures
	check(DynamicData.Textures.Num() == NvHw::EHairTextureType::COUNT_OF);
	HairTextures.SetNumZeroed(NvHw::EHairTextureType::COUNT_OF);
	for(auto Idx = 0; Idx < HairTextures.Num(); ++Idx)
	{
		auto* Texture = DynamicData.Textures[Idx];
		if(Texture == nullptr)
			continue;

		if(Texture->Resource == nullptr)
			continue;

		HairTextures[Idx] = static_cast<FTexture2DResource*>(Texture->Resource)->GetTexture2DRHI();
	}

	for (auto Idx = 0; Idx < NvHw::EHairTextureType::COUNT_OF; ++Idx)
	{
		extern ID3D11ShaderResourceView* (*HairWorksGetSrvFromTexture)(FRHITexture2D*);

		auto TextureRef = HairTextures[Idx];
		GHairWorksSDK->setTexture(HairInstanceId, (NvHw::HairTextureType::Enum)Idx, Nv::Dx11Type::getHandle(HairWorksGetSrvFromTexture(TextureRef.GetReference())));
	}
}
// @third party code - END HairWorks
