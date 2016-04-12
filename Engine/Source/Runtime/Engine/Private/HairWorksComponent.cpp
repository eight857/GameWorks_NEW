// @third party code - BEGIN HairWorks
#include "EnginePrivate.h"
#include <Nv/Common/NvCoMemoryReadStream.h>
#include "HairWorksSDK.h"
#include "Engine/HairWorksMaterial.h"
#include "Engine/HairWorksAsset.h"
#include "HairWorksSceneProxy.h"
#include "Components/HairWorksPinTransformComponent.h"
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
	, HairInstanceId(NvHair::INSTANCE_ID_NULL)
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
	if(HairInstanceId != NvHair::INSTANCE_ID_NULL)
	{
		HairWorks::GetSDK()->freeInstance(HairInstanceId);
		HairInstanceId = NvHair::INSTANCE_ID_NULL;
	}
}

FPrimitiveSceneProxy* UHairWorksComponent::CreateSceneProxy()
{
	if(HairInstanceId == NvHair::INSTANCE_ID_NULL)
		return nullptr;

	return new FHairWorksSceneProxy(this, HairInstanceId);
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
	if (HairInstanceId == NvHair::INSTANCE_ID_NULL)
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
	if(HairInstance.Hair->AssetId == NvHair::INSTANCE_ID_NULL)
	{
		// Create hair asset
		NvCo::MemoryReadStream ReadStream(HairInstance.Hair->AssetData.GetData(), HairInstance.Hair->AssetData.Num());
		HairWorks::GetSDK()->loadAsset(&ReadStream, HairInstance.Hair->AssetId, nullptr, &HairWorks::GetAssetConversionSettings());
	}

	// Initialize hair instance
	if(HairInstanceId == NvHair::INSTANCE_ID_NULL)
	{
		HairWorks::GetSDK()->createInstance(HairInstance.Hair->AssetId, HairInstanceId);

		// Disable this instance at first.
		NvHair::InstanceDescriptor HairInstanceDesc;
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

	// Delete this instance for next frame.
	if(HairInstanceId == NvHair::INSTANCE_ID_NULL)
		return;

	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		HairDisableInstance,
		nvidia::HairWorks::InstanceId, HairInstanceId, HairInstanceId,
		{
			HairWorks::GetSDK()->freeInstance(HairInstanceId);
		}
	);

	HairInstanceId = NvHair::INSTANCE_ID_NULL;
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

	DynamicData->Textures.SetNumZeroed(NvHair::ETextureType::COUNT_OF);

	HairWorks::GetSDK()->getInstanceDescriptorFromAsset(HairInstance.Hair->AssetId, DynamicData->HairInstanceDesc);
	DynamicData->HairInstanceDesc.m_visualizeBones = false;
	DynamicData->HairInstanceDesc.m_visualizeBoundingBox = false;
	DynamicData->HairInstanceDesc.m_visualizeCapsules = false;
	DynamicData->HairInstanceDesc.m_visualizeControlVertices = false;
	DynamicData->HairInstanceDesc.m_visualizeGrowthMesh = false;
	DynamicData->HairInstanceDesc.m_visualizeGuideHairs = false;
	DynamicData->HairInstanceDesc.m_visualizeHairInteractions = false;
	DynamicData->HairInstanceDesc.m_visualizePinConstraints = false;
	DynamicData->HairInstanceDesc.m_visualizeShadingNormals = false;
	DynamicData->HairInstanceDesc.m_visualizeShadingNormalBone = false;
	DynamicData->HairInstanceDesc.m_visualizeSkinnedGuideHairs = false;

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

	// Setup pins
	if(HairInstance.Hair->PinsUpdateFrameNumber != GFrameNumber)
	{
		HairInstance.Hair->PinsUpdateFrameNumber = GFrameNumber;

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			HairUpdatePins,
			NvHair::AssetId, AssetId, HairInstance.Hair->AssetId,
			const TArray<FHairWorksPin>, EnginePins, HairInstance.Hair->HairMaterial->Pins,
			{
				TArray<NvHair::Pin> Pins;
				Pins.AddDefaulted(EnginePins.Num());
				HairWorks::GetSDK()->getPins(AssetId, 0, Pins.Num(), Pins.GetData());

				for(auto PinIndex = 0; PinIndex < Pins.Num(); ++PinIndex)
				{
					auto& Pin = Pins[PinIndex];

					const auto& SrcPin = EnginePins[PinIndex];

					Pin.m_useDynamicPin = SrcPin.bDynamicPin;
					Pin.m_doLra = SrcPin.bTetherPin;
					Pin.m_pinStiffness = SrcPin.Stiffness;
					Pin.m_influenceFallOff = SrcPin.InfluenceFallOff;
					Pin.m_influenceFallOffCurve = reinterpret_cast<const gfsdk_float4&>(SrcPin.InfluenceFallOffCurve);
				}

				HairWorks::GetSDK()->setPins(AssetId, 0, Pins.Num(), Pins.GetData());
			}
		);
	}

	// Add pin meshes
	DynamicData->PinMeshes.AddDefaulted(HairInstance.Hair->HairMaterial->Pins.Num());

	for(auto* ChildComponent : AttachChildren)
	{
		// Find pin transform component
		auto* PinComponent = Cast<UHairWorksPinTransformComponent>(ChildComponent);
		if(PinComponent == nullptr)
			continue;

		if(PinComponent->PinIndex < 0 || PinComponent->PinIndex >= DynamicData->PinMeshes.Num())
			continue;

		// Collect pin meshes
		auto& PinMeshes = DynamicData->PinMeshes[PinComponent->PinIndex];

		TFunction<bool(const USceneComponent*)> AddPinMesh;
		AddPinMesh = [&](const USceneComponent* Component)
		{
			// Add mesh
			auto* PrimitiveComponent = Cast<UPrimitiveComponent>(Component);

			if(PrimitiveComponent != nullptr && PrimitiveComponent->SceneProxy != nullptr)
			{
				FHairWorksSceneProxy::FPinMesh PinMesh;
				PinMesh.Mesh = PrimitiveComponent->SceneProxy;
				PinMesh.LocalTransform = (PrimitiveComponent->ComponentToWorld * ComponentToWorld.Inverse()).ToMatrixWithScale();

				PinMeshes.Add(PinMesh);
			}

			// Find in children
			Component->AttachChildren.FindByPredicate(AddPinMesh);

			return false;
		};

		AddPinMesh(PinComponent);
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

void UHairWorksComponent::UpdateBones() const
{
	// Apply bone mapping
	if(HairInstanceId == NvHair::INSTANCE_ID_NULL || ParentSkeleton == nullptr || ParentSkeleton->SkeletalMesh == nullptr)
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
	HairWorks::GetSDK()->updateSkinningMatrices(HairInstanceId, BoneMatrices.Num(), reinterpret_cast<gfsdk_float4x4*>(BoneMatrices.GetData()));
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
