// @third party code - BEGIN HairWorks
#include "EnginePrivate.h"
#include <Nv/Common/NvCoMemoryReadStream.h>
#include "AllowWindowsPlatformTypes.h"
#include <Nv/Common/Platform/Dx11/NvCoDx11Handle.h>
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

FHairWorksSceneProxy* FHairWorksSceneProxy::HairInstances = nullptr;

FHairWorksSceneProxy::FHairWorksSceneProxy(const UPrimitiveComponent* InComponent, NvHair::AssetId InHairAssetId):
	FPrimitiveSceneProxy(InComponent),
	HairAssetId(InHairAssetId),
	HairInstanceId(NvHair::INSTANCE_ID_NULL)
{
	check(InHairAssetId != NvHair::ASSET_ID_NULL);

	HairTextures.SetNumZeroed(NvHair::ETextureType::COUNT_OF);
}

FHairWorksSceneProxy::~FHairWorksSceneProxy()
{
	if(HairInstanceId != NvHair::INSTANCE_ID_NULL)
	{
		HairWorks::GetSDK()->freeInstance(HairInstanceId);
		HairInstanceId = NvHair::INSTANCE_ID_NULL;

		Unlink();
	}
}

uint32 FHairWorksSceneProxy::GetMemoryFootprint(void) const
{
	return 0;
}

void FHairWorksSceneProxy::Draw(FRHICommandList& RHICmdList, EDrawType DrawType)const
{
	if(RHICmdList.Bypass())
		DoRender(RHICmdList, DrawType);
	else
	{
		struct FRHICmdDraw: public FRHICommand<FRHICmdDraw>
		{
			const FHairWorksSceneProxy& HairProxy;
			EDrawType DrawType;

			FRHICmdDraw(const FHairWorksSceneProxy& InHairProxy, EDrawType InDrawType)
				:HairProxy(InHairProxy), DrawType(InDrawType){}

			void Execute(FRHICommandListBase& CmdList)
			{
				HairProxy.DoRender(CmdList, DrawType);
			}
		};

		new (RHICmdList.AllocCommand<FRHICmdDraw>()) FRHICmdDraw(*this, DrawType);
	}
}

void FHairWorksSceneProxy::SetPinMatrices(const TArray<FMatrix>& PinMatrices)
{
	FScopeLock ObjObjectsLock(&ThreadLock);

	HairPinMatrices = PinMatrices;
}

const TArray<FMatrix>& FHairWorksSceneProxy::GetPinMatrices()
{	
	FScopeLock ObjObjectsLock(&ThreadLock);

	return HairPinMatrices;
}

FHairWorksSceneProxy * FHairWorksSceneProxy::GetHairInstances()
{
	return HairInstances;
}

void FHairWorksSceneProxy::DoRender(FRHICommandListBase & RHICmdList, EDrawType DrawType) const
{
	// Can't call it here. It changes render states, so we must call it earlier before we set render states. 
	//HairWorks::GetSDK()->preRender(RenderInterp);

	// Flush render states
	HairWorks::GetD3DHelper().CommitShaderResources(RHICmdList.GetContext());

	// Draw
	if (DrawType == EDrawType::Visualization)
	{
		HairWorks::GetSDK()->renderVisualization(HairInstanceId);
	}
	else
	{
		// Special for shadow
		NvHair::InstanceDescriptor HairDesc;
		HairWorks::GetSDK()->getInstanceDescriptor(HairInstanceId, HairDesc);

		if (DrawType == EDrawType::Shadow)
		{
			HairDesc.m_useBackfaceCulling = false;
			HairDesc.m_useViewfrustrumCulling = false;

			HairWorks::GetSDK()->updateInstanceDescriptor(HairInstanceId, HairDesc);
		}

		// Handle shader cache.
		NvHair::ShaderCacheSettings ShaderCacheSetting;
		ShaderCacheSetting.setFromInstanceDescriptor(HairDesc);
		check(HairTextures.Num() == NvHair::ETextureType::COUNT_OF);
		for(int i = 0; i < NvHair::ETextureType::COUNT_OF; i++)
		{
			ShaderCacheSetting.setTextureUsed(i, HairTextures[i] != nullptr);
		}

		HairWorks::GetSDK()->addToShaderCache(ShaderCacheSetting);

		// Draw
		NvHair::ShaderSettings HairShaderSettings;
		HairShaderSettings.m_useCustomConstantBuffer = true;
		HairShaderSettings.m_shadowPass = (DrawType == EDrawType::Shadow);

		HairWorks::GetSDK()->renderHairs(HairInstanceId, &HairShaderSettings);
	}
}

FPrimitiveViewRelevance FHairWorksSceneProxy::GetViewRelevance(const FSceneView* View)const
{
	FPrimitiveViewRelevance ViewRel;
	ViewRel.bDrawRelevance = IsShown(View);
	ViewRel.bShadowRelevance = IsShadowCast(View);
	ViewRel.bDynamicRelevance = true;
	ViewRel.bRenderInMainPass = false;	// Hair is rendered in a special path.

	ViewRel.bHairWorks = View->Family->EngineShowFlags.HairWorks && HairInstanceId != NvHair::INSTANCE_ID_NULL;

	return ViewRel;
}

void FHairWorksSceneProxy::CreateRenderThreadResources()
{
	// Initialize hair instance
	check(HairWorks::GetSDK() != nullptr &&  HairAssetId != NvHair::ASSET_ID_NULL && HairInstanceId == NvHair::INSTANCE_ID_NULL);

	auto& HairSdk = *HairWorks::GetSDK();
	HairSdk.createInstance(HairAssetId, HairInstanceId);
	if(HairInstanceId == NvHair::INSTANCE_ID_NULL)
		return;

	// Disable this instance at first.
	NvHair::InstanceDescriptor HairInstanceDesc;
	HairSdk.getInstanceDescriptor(HairInstanceId, HairInstanceDesc);
	if(HairInstanceDesc.m_enable)
	{
		HairInstanceDesc.m_enable = false;
		HairSdk.updateInstanceDescriptor(HairInstanceId, HairInstanceDesc);
	}

	// Add to list
	LinkHead(HairInstances);
}

void FHairWorksSceneProxy::OnTransformChanged()
{
	// Update new matrix to hair
	FPrimitiveSceneProxy::OnTransformChanged();

	if(HairInstanceId == NvHair::INSTANCE_ID_NULL)
		return;

	NvHair::InstanceDescriptor InstDesc;
	HairWorks::GetSDK()->getInstanceDescriptor(HairInstanceId, InstDesc);

	InstDesc.m_modelToWorld = (gfsdk_float4x4&)GetLocalToWorld().M;

	HairWorks::GetSDK()->updateInstanceDescriptor(HairInstanceId, InstDesc);
}

void FHairWorksSceneProxy::UpdateDynamicData_RenderThread(const FDynamicRenderData& DynamicData)
{
	// Set skinning data
	if(DynamicData.BoneMatrices.Num() > 0)
	{
		HairWorks::GetSDK()->updateSkinningMatrices(
			HairInstanceId, DynamicData.BoneMatrices.Num(),
			reinterpret_cast<const gfsdk_float4x4*>(DynamicData.BoneMatrices.GetData())
		);

		if(CurrentSkinningMatrices.Num() > 0)
			PrevSkinningMatrices = CurrentSkinningMatrices;
		else
			PrevSkinningMatrices = DynamicData.BoneMatrices;

		CurrentSkinningMatrices = DynamicData.BoneMatrices;
	}

	// Update normal center bone
	auto HairDesc = DynamicData.HairInstanceDesc;

	// Merge global visualization flags.
#define HairVisualizationCVarUpdate(CVarName, MemberVarName)	\
	HairDesc.m_visualize##MemberVarName |= CVarHairVisualization##CVarName.GetValueOnRenderThread() != 0

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

	HairDesc.m_drawRenderHairs &= CVarHairVisualizationHair.GetValueOnRenderThread() != 0;

	// Other parameters
	HairDesc.m_modelToWorld = (gfsdk_float4x4&)GetLocalToWorld().M;

	// Set parameters to HairWorks
	HairWorks::GetSDK()->updateInstanceDescriptor(HairInstanceId, HairDesc);	// Mainly for simulation.

	// Update textures
	check(DynamicData.Textures.Num() == NvHair::ETextureType::COUNT_OF);
	HairTextures.SetNumZeroed(NvHair::ETextureType::COUNT_OF);
	for(auto Idx = 0; Idx < HairTextures.Num(); ++Idx)
	{
		auto* Texture = DynamicData.Textures[Idx];
		if(Texture == nullptr)
			continue;

		if(Texture->Resource == nullptr)
			continue;

		HairTextures[Idx] = static_cast<FTexture2DResource*>(Texture->Resource)->GetTexture2DRHI();
	}

	for (auto Idx = 0; Idx < NvHair::ETextureType::COUNT_OF; ++Idx)
	{
		auto TextureRef = HairTextures[Idx];
		HairWorks::GetSDK()->setTexture(HairInstanceId, (NvHair::ETextureType)Idx, NvCo::Dx11Type::wrap(HairWorks::GetD3DHelper().GetShaderResourceView(TextureRef.GetReference())));
	}

	// Add pin meshes
	HairPinMeshes = DynamicData.PinMeshes;
}
// @third party code - END HairWorks
