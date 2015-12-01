// @third party code - BEGIN HairWorks
#include "EnginePrivate.h"
#include "HairWorksSDK.h"
#include "Engine/HairWorksMaterial.h"
#include "Engine/HairWorksAsset.h"
#include "HairWorksSceneProxy.h"
#include "Components/HairWorksComponent.h"

UHairWorksComponent::UHairWorksComponent(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
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
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;	// Just mark render data dirty every frame

	// Create hair material
	HairInstance.HairMaterial = ObjectInitializer.CreateDefaultSubobject<UHairWorksMaterial>(this, FName(*UHairWorksMaterial::StaticClass()->GetName()));
	HairInstance.HairMaterial->SetFlags(EObjectFlags::RF_Public);	// To avoid "Graph is linked to private object(s) in an external package." error in UPackage::SavePackage().
}

FPrimitiveSceneProxy* UHairWorksComponent::CreateSceneProxy()
{
	if (HairInstance.Hair == nullptr)
		return nullptr;

	return new FHairWorksSceneProxy(this, *HairInstance.Hair);
}

void UHairWorksComponent::OnAttachmentChanged()
{
	// Parent as skeleton
	ParentSkeleton = Cast<USkinnedMeshComponent>(AttachParent);

	// Setup bone mapping
	SetupBoneMapping();

	// Update proxy
	SendHairDynamicData();	// For correct initial visual effect
}

FBoxSphereBounds UHairWorksComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	auto* HairSceneProxy = static_cast<FHairWorksSceneProxy*>(SceneProxy);
	if (HairSceneProxy == nullptr || HairSceneProxy->GetHairInstanceId() == GFSDK_HairAssetID_NULL)
		return FBoxSphereBounds(EForceInit::ForceInit);

	gfsdk_float3 HairBoundMin, HairBoundMax;
	GHairWorksSDK->GetBounds(HairSceneProxy->GetHairInstanceId(), &HairBoundMin, &HairBoundMax);

	FBoxSphereBounds Bounds(FBox(reinterpret_cast<FVector&>(HairBoundMin), reinterpret_cast<FVector&>(HairBoundMax)));

	return Bounds.TransformBy(LocalToWorld);
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

	// Send data every frame
	if (SceneProxy != nullptr)
	{
#if WITH_EDITOR
		if (!(GetWorld() && GetWorld()->bPostTickComponentUpdate))
			MarkRenderTransformDirty();	// Update scene cached bounds.
#endif

		MarkRenderDynamicDataDirty();
	}
}

void UHairWorksComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

	// Update static bound
	UpdateBounds();

	// TODO: Update scene cached bounds. I'm not sure whether we need it out of editor. We need to figure it out in future.
	if(!(GetWorld() && GetWorld()->bPostTickComponentUpdate))
		MarkRenderTransformDirty();

	// Setup bone mapping
	SetupBoneMapping();

	// Update proxy
	SendHairDynamicData();	// Ensure correct visual effect at first frame.
}

void UHairWorksComponent::SendHairDynamicData()const
{
	// Setup bone matrices
	if (SceneProxy == nullptr)
		return;

	TSharedRef<FHairWorksSceneProxy::FDynamicRenderData> DynamicData(new FHairWorksSceneProxy::FDynamicRenderData);

	if(ParentSkeleton != nullptr && ParentSkeleton->SkeletalMesh != nullptr)
	{
		DynamicData->BoneMatrices.SetNum(BoneIndices.Num());

		for(auto Idx = 0; Idx < BoneIndices.Num(); ++Idx)
		{
			auto& IdxInParent = BoneIndices[Idx];

			auto Matrix = ParentSkeleton->GetSpaceBases()[IdxInParent].ToMatrixWithScale();

			DynamicData->BoneMatrices[Idx] = ParentSkeleton->SkeletalMesh->RefBasesInvMatrix[IdxInParent] * Matrix;
		}
	}

	// Setup material
	DynamicData->Textures.SetNumZeroed(GFSDK_HAIR_NUM_TEXTURES);

	if(HairInstance.Hair->HairMaterial != nullptr)	// Always load from asset to propagate visualization flags.
	{
		HairInstance.Hair->HairMaterial->SyncHairDescriptor(DynamicData->HairInstanceDesc, DynamicData->Textures, false);
		DynamicData->NormalCenterBoneName = HairInstance.Hair->HairMaterial->HairNormalCenter;
	}

	if(HairInstance.HairMaterial != nullptr && HairInstance.bOverride)
	{
		HairInstance.HairMaterial->SyncHairDescriptor(DynamicData->HairInstanceDesc, DynamicData->Textures, false);
		DynamicData->NormalCenterBoneName = HairInstance.HairMaterial->HairNormalCenter;
	}


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
// @third party code - END HairWorks
