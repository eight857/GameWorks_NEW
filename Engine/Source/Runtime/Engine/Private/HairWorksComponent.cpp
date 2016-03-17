// @third party code - BEGIN HairWorks
#include "EnginePrivate.h"
#include <Nv/Foundation/NvMemoryReadStream.h>
#include "HairWorksSDK.h"
#include "Engine/HairWorksMaterial.h"
#include "Engine/HairWorksAsset.h"
#include "HairWorksSceneProxy.h"
#include "Components/HairWorksComponent.h"

/** 
 *  Component instance cached data class for HairWorks components. 
 *  Copies HairWorksInstance and HairWorksMaterial.
 */
class FHairWorksComponentInstanceData: public FPrimitiveComponentInstanceData
{
public:
	FHairWorksComponentInstanceData(const UHairWorksComponent* SourceComponent);

	virtual void ApplyToComponent(UActorComponent* Component, const ECacheApplyPhase CacheApplyPhase) override;

protected:
	FHairWorksInstance HairInstance;
};

UHairWorksComponent::UHairWorksComponent(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, HairInstanceId(NvHw::HAIR_INSTANCE_ID_NULL)
{
	// No need to select
	bSelectable = false;

	// Setup shadow
	CastShadow = true;
	bAffectDynamicIndirectLighting = false;
	bAffectDistanceFieldLighting = false;
	bCastInsetShadow = true;
	bCastStaticShadow = false;

	// Setup tick
	bAutoActivate = true;
	bTickInEditor = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

	// Create hair material
	HairInstance.HairMaterial = ObjectInitializer.CreateDefaultSubobject<UHairWorksMaterial>(this, FName(*UHairWorksMaterial::StaticClass()->GetName()));
	HairInstance.HairMaterial->SetFlags(EObjectFlags::RF_Public);	// To avoid "Graph is linked to private object(s) in an external package." error in UPackage::SavePackage().
}

UHairWorksComponent::~UHairWorksComponent()
{
	if(HairInstanceId != NvHw::HAIR_INSTANCE_ID_NULL)
	{
		HairWorks::GetSDK()->freeHairInstance(HairInstanceId);
		HairInstanceId = NvHw::HAIR_INSTANCE_ID_NULL;
	}
}

FPrimitiveSceneProxy* UHairWorksComponent::CreateSceneProxy()
{
	if(HairInstanceId == NvHw::HAIR_INSTANCE_ID_NULL)
		return nullptr;

	return new FHairWorksSceneProxy(this, *HairInstance.Hair, HairInstanceId);
}

void UHairWorksComponent::OnAttachmentChanged()
{
	// Parent as skeleton
	ParentSkeleton = Cast<USkinnedMeshComponent>(AttachParent);

	// Setup bone mapping
	SetupBoneMapping();

	MarkRenderDynamicDataDirty();
}

FBoxSphereBounds UHairWorksComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (HairInstanceId == NvHw::HAIR_INSTANCE_ID_NULL)
		return FBoxSphereBounds(EForceInit::ForceInit);

	gfsdk_float3 HairBoundMin, HairBoundMax;
	HairWorks::GetSDK()->getBounds(HairInstanceId, HairBoundMin, HairBoundMax);

	FBoxSphereBounds Bounds(FBox(reinterpret_cast<FVector&>(HairBoundMin), reinterpret_cast<FVector&>(HairBoundMax)));

	return Bounds.TransformBy(LocalToWorld);
}

void UHairWorksComponent::OnRegister()
{
	Super::OnRegister();

	if(HairWorks::GetSDK() == nullptr || HairInstance.Hair == nullptr)
		return;

	// Initialize hair asset
	if(HairInstance.Hair->AssetId == NvHw::HAIR_ASSET_ID_NULL)
	{
		// Create hair asset
		Nv::MemoryReadStream ReadStream(HairInstance.Hair->AssetData.GetData(), HairInstance.Hair->AssetData.Num());
		HairWorks::GetSDK()->loadHairAsset(&ReadStream, HairInstance.Hair->AssetId, nullptr, &HairWorks::GetAssetConversionSettings());
	}

	// Initialize hair instance
	if(HairInstanceId == NvHw::HAIR_INSTANCE_ID_NULL)
	{
		HairWorks::GetSDK()->createHairInstance(HairInstance.Hair->AssetId, HairInstanceId);

		// Disable this instance at first.
		NvHw::HairInstanceDescriptor HairInstanceDesc;
		HairWorks::GetSDK()->getInstanceDescriptor(HairInstanceId, HairInstanceDesc);
		if(HairInstanceDesc.m_enable)
		{
			HairInstanceDesc.m_enable = false;
			HairWorks::GetSDK()->updateInstanceDescriptor(HairInstanceId, HairInstanceDesc);
		}

		// Setup bone look up table
		BoneNameToIdx.Empty(HairInstance.Hair->BoneNames.Num());
		for(auto Idx = 0; Idx < HairInstance.Hair->BoneNames.Num(); ++Idx)
		{
			BoneNameToIdx.Add(HairInstance.Hair->BoneNames[Idx], Idx);
		}
	}

	// Setup bone mapping
	SetupBoneMapping();

	// Update bones
	UpdateBones();
}

void UHairWorksComponent::OnUnregister()
{
	Super::OnUnregister();

	// Disable this instance to save GPU time.
	if(HairInstanceId == NvHw::HAIR_INSTANCE_ID_NULL)
		return;

	NvHw::HairInstanceDescriptor HairInstanceDesc;
	HairWorks::GetSDK()->getInstanceDescriptor(HairInstanceId, HairInstanceDesc);

	if(!HairInstanceDesc.m_enable)
		return;

	HairInstanceDesc.m_enable = false;

	// Disable for next frame.
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		HairDisableInstance,
		Nv::HairWorks::HairInstanceId, HairInstanceId, HairInstanceId,
		NvHw::HairInstanceDescriptor, HairInstanceDesc, HairInstanceDesc,
		{
			HairWorks::GetSDK()->updateInstanceDescriptor(HairInstanceId, HairInstanceDesc);
		}
	);
}

void UHairWorksComponent::SendRenderDynamicData_Concurrent()
{
	Super::SendRenderDynamicData_Concurrent();

	// Send data for rendering
	SendHairDynamicData();
}

void UHairWorksComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update bones, so that we can get hair bounds in game thread.
	UpdateBones();

	// Mark to send dynamic data
	MarkRenderDynamicDataDirty();
}

FActorComponentInstanceData * UHairWorksComponent::GetComponentInstanceData() const
{
	return new FHairWorksComponentInstanceData(this);
}

void UHairWorksComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

	// Update proxy
	SendHairDynamicData();	// Ensure correct visual effect at first frame.
}

void UHairWorksComponent::SendHairDynamicData()const
{
	// Setup material
	if(SceneProxy == nullptr)
		return;

	TSharedRef<FHairWorksSceneProxy::FDynamicRenderData> DynamicData(new FHairWorksSceneProxy::FDynamicRenderData);

	DynamicData->Textures.SetNumZeroed(NvHw::EHairTextureType::COUNT_OF);

	FName HairNormalCenter;

	if(HairInstance.Hair->HairMaterial != nullptr)	// Always load from asset to propagate visualization flags.
	{
		HairInstance.Hair->HairMaterial->SyncHairDescriptor(DynamicData->HairInstanceDesc, DynamicData->Textures, false);
		HairNormalCenter = HairInstance.Hair->HairMaterial->HairNormalCenter;
	}

	if(HairInstance.HairMaterial != nullptr && HairInstance.bOverride)
	{
		HairInstance.HairMaterial->SyncHairDescriptor(DynamicData->HairInstanceDesc, DynamicData->Textures, false);
		HairNormalCenter = HairInstance.HairMaterial->HairNormalCenter;
	}

	auto* BoneIdx = BoneNameToIdx.Find(HairNormalCenter);
	if(BoneIdx != nullptr)
		DynamicData->HairInstanceDesc.m_hairNormalBoneIndex = *BoneIdx;
	else
		DynamicData->HairInstanceDesc.m_hairNormalWeight = 0;

	// Send to proxy
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		HairUpdateDynamicData,
		FHairWorksSceneProxy&, ThisProxy, static_cast<FHairWorksSceneProxy&>(*SceneProxy),
		TSharedRef<FHairWorksSceneProxy::FDynamicRenderData>, DynamicData, DynamicData,
		{
			ThisProxy.UpdateDynamicData_RenderThread(*DynamicData);
		}
	);
}

void UHairWorksComponent::SetupBoneMapping()
{
	if(HairInstance.Hair == nullptr || ParentSkeleton == nullptr || ParentSkeleton->SkeletalMesh == nullptr)
		return;

	auto& Bones = ParentSkeleton->SkeletalMesh->RefSkeleton.GetRefBoneInfo();
	BoneIndices.SetNumUninitialized(HairInstance.Hair->BoneNames.Num());

	for(auto Idx = 0; Idx < BoneIndices.Num(); ++Idx)
	{
		BoneIndices[Idx] = Bones.IndexOfByPredicate(
			[&](const FMeshBoneInfo& BoneInfo){return BoneInfo.Name == HairInstance.Hair->BoneNames[Idx]; }
		);
	}
}

void UHairWorksComponent::UpdateBones() const
{
	// Apply bone mapping
	if(HairInstanceId == NvHw::HAIR_INSTANCE_ID_NULL || ParentSkeleton == nullptr || ParentSkeleton->SkeletalMesh == nullptr)
		return;

	BoneMatrices.Init(FMatrix::Identity, BoneIndices.Num());

	for(auto Idx = 0; Idx < BoneIndices.Num(); ++Idx)
	{
		const auto& IdxInParent = BoneIndices[Idx];
		if(IdxInParent >= ParentSkeleton->GetSpaceBases().Num())
			continue;

		const auto Matrix = ParentSkeleton->GetSpaceBases()[IdxInParent].ToMatrixWithScale();

		BoneMatrices[Idx] = ParentSkeleton->SkeletalMesh->RefBasesInvMatrix[IdxInParent] * Matrix;
	}

	// Update bones to HairWorks
	HairWorks::GetSDK()->updateSkinningMatrices(HairInstanceId, BoneMatrices.Num(), (gfsdk_float4x4*)BoneMatrices.GetData());
}

#if WITH_EDITOR
void UHairWorksComponent::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	// Initialize hair material when a hair asset is assigned
	if(HairInstance.Hair == nullptr || HairInstance.Hair->HairMaterial == nullptr)
		return;

	auto* PropertyNode = PropertyChangedEvent.PropertyChain.GetActiveMemberNode();

	if(
		PropertyNode->GetValue()->GetName() != "HairInstance"
		|| PropertyNode->GetNextNode() == nullptr
		|| PropertyNode->GetNextNode()->GetValue()->GetName() != "Hair"
		|| PropertyNode->GetNextNode()->GetNextNode() != nullptr
		)
		return;

	for(TFieldIterator<UProperty> PropIt(UHairWorksMaterial::StaticClass()); PropIt; ++PropIt)
	{
		auto* Property = *PropIt;
		Property->CopyCompleteValue_InContainer(HairInstance.HairMaterial, HairInstance.Hair->HairMaterial);
	}
}
#endif

FHairWorksComponentInstanceData::FHairWorksComponentInstanceData(const UHairWorksComponent * SourceComponent)
	:FPrimitiveComponentInstanceData(SourceComponent)
{
	HairInstance = SourceComponent->HairInstance;
}

void FHairWorksComponentInstanceData::ApplyToComponent(UActorComponent * Component, const ECacheApplyPhase CacheApplyPhase)
{
	FPrimitiveComponentInstanceData::ApplyToComponent(Component, CacheApplyPhase);

	// Copy HairInstance
	auto& TgtHairInstance = CastChecked<UHairWorksComponent>(Component)->HairInstance;

	TgtHairInstance.bOverride = HairInstance.bOverride;

	for(TFieldIterator<UProperty> PropIt(UHairWorksMaterial::StaticClass()); PropIt; ++PropIt)
	{
		auto* Property = *PropIt;
		Property->CopyCompleteValue_InContainer(TgtHairInstance.HairMaterial, HairInstance.HairMaterial);
	}
}
// @third party code - END HairWorks
