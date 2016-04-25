// @third party code - BEGIN HairWorks
#include "EnginePrivate.h"
#include <Nv/Common/NvCoMemoryReadStream.h>
#include "HairWorksSDK.h"
#include "Engine/HairWorksMaterial.h"
#include "Engine/HairWorksAsset.h"
#include "HairWorksSceneProxy.h"
#include "Components/HairWorksPinTransformComponent.h"
#include "Components/HairWorksComponent.h"

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
	check(HairInstanceId == NvHair::INSTANCE_ID_NULL);
}

FPrimitiveSceneProxy* UHairWorksComponent::CreateSceneProxy()
{
	return new FHairWorksSceneProxy(this, HairInstanceId);
}

void UHairWorksComponent::OnAttachmentChanged()
{
	// Parent as skeleton
	ParentSkeleton = Cast<USkinnedMeshComponent>(AttachParent);

	// Setup bone mapping
	SetupBoneMapping();

	// Refresh render data
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

void UHairWorksComponent::SendRenderDynamicData_Concurrent()
{
	Super::SendRenderDynamicData_Concurrent();

	// Send data for rendering
	SendHairDynamicData();
}

void UHairWorksComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Mark to send dynamic data
	MarkRenderDynamicDataDirty();
}

bool UHairWorksComponent::ShouldCreateRenderState() const
{
	return HairWorks::GetSDK() != nullptr && HairInstance.Hair != nullptr;
}

void UHairWorksComponent::CreateRenderState_Concurrent()
{
	// Initialize hair asset
	check(HairWorks::GetSDK() != nullptr && HairInstance.Hair != nullptr);

	auto& HairSdk = *HairWorks::GetSDK();
	if(HairInstance.Hair->AssetId == NvHair::ASSET_ID_NULL)
	{
		// Create hair asset
		NvCo::MemoryReadStream ReadStream(HairInstance.Hair->AssetData.GetData(), HairInstance.Hair->AssetData.Num());
		HairSdk.loadAsset(&ReadStream, HairInstance.Hair->AssetId, nullptr, &HairWorks::GetAssetConversionSettings());
	}

	// Initialize hair instance
	HairSdk.createInstance(HairInstance.Hair->AssetId, HairInstanceId);
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

	// Setup bone look up table
	BoneNameToIdx.Empty(HairInstance.Hair->BoneNames.Num());
	for(auto Idx = 0; Idx < HairInstance.Hair->BoneNames.Num(); ++Idx)
	{
		BoneNameToIdx.Add(HairInstance.Hair->BoneNames[Idx], Idx);
	}

	// Call super
	Super::CreateRenderState_Concurrent();

	// Setup bone mapping
	SetupBoneMapping();

	// Update proxy
	SendHairDynamicData(true);	// Ensure correct visual effect at first frame.
}

void UHairWorksComponent::DestroyRenderState_Concurrent()
{
	Super::DestroyRenderState_Concurrent();

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

void UHairWorksComponent::SendHairDynamicData(bool bForceSkinning)const
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

	// Setup skinning
	if(ParentSkeleton != nullptr)
	{
		DynamicData->BoneMatrices.Init(FMatrix::Identity, BoneIndices.Num());

		for(auto Idx = 0; Idx < BoneIndices.Num(); ++Idx)
		{
			const uint16& IdxInParent = BoneIndices[Idx];
			if(IdxInParent >= ParentSkeleton->GetSpaceBases().Num())
				continue;

			const auto Matrix = ParentSkeleton->GetSpaceBases()[IdxInParent].ToMatrixWithScale();

			DynamicData->BoneMatrices[Idx] = ParentSkeleton->SkeletalMesh->RefBasesInvMatrix[IdxInParent] * Matrix;
		}

		DynamicData->bForceSkinning = bForceSkinning;
	}

	// Setup pins
	if(HairInstance.Hair->PinsUpdateFrameNumber != GFrameNumber && HairInstance.Hair->HairMaterial->Pins.Num() > 0)
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
