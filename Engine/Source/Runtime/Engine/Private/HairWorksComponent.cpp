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

FActorComponentInstanceData * UHairWorksComponent::GetComponentInstanceData() const
{
	/**
	 *  Component instance cached data class for HairWorks components.
	 *  Copies HairInstance and HairMaterial. Because HairInstance contains instanced reference, HairMaterial, so it's not automatically copied. And HairMaterial is a instance reference, so it's not automatically copied either.
	 */
	class FHairWorksComponentInstanceData: public FPrimitiveComponentInstanceData
	{
	public:
		FHairWorksComponentInstanceData(const UHairWorksComponent& SourceComponent)
			:FPrimitiveComponentInstanceData(&SourceComponent)
		{
			if(!SourceComponent.IsEditableWhenInherited())
				return;

			class FHairInstancePropertyWriter: public FObjectWriter
			{
			public:
				FHairInstancePropertyWriter(const UHairWorksComponent& HairComp, TArray<uint8>& InBytes)
					: FObjectWriter(InBytes)
				{
					auto* Archetype = CastChecked<UHairWorksComponent>(HairComp.GetArchetype());

					FHairWorksInstance::StaticStruct()->SerializeTaggedProperties(
						*this,
						(uint8*)&HairComp.HairInstance,
						FHairWorksInstance::StaticStruct(),
						(uint8*)&Archetype->HairInstance
					);

					UHairWorksMaterial::StaticClass()->SerializeTaggedProperties(
						*this,
						(uint8*)HairComp.HairInstance.HairMaterial,
						UHairWorksMaterial::StaticClass(),
						(uint8*)Archetype->HairInstance.HairMaterial
					);
				}

				virtual bool ShouldSkipProperty(const UProperty* InProperty) const override
				{
					return (InProperty->HasAnyPropertyFlags(CPF_Transient | CPF_ContainsInstancedReference | CPF_InstancedReference)
						|| !InProperty->HasAnyPropertyFlags(CPF_Edit | CPF_Interp));
				}

			private:
			} HairInstancePropertyWriter(SourceComponent, SavedProperties);
		}

		virtual void ApplyToComponent(UActorComponent* Component, const ECacheApplyPhase CacheApplyPhase) override
		{
			FPrimitiveComponentInstanceData::ApplyToComponent(Component, CacheApplyPhase);

			if(CacheApplyPhase != ECacheApplyPhase::PostUserConstructionScript || SavedProperties.Num() == 0)
				return;

			class FHairInstancePropertyReader: public FObjectReader
			{
			public:
				FHairInstancePropertyReader(UHairWorksComponent& InComponent, TArray<uint8>& InBytes)
					: FObjectReader(InBytes)
				{
					FHairWorksInstance::StaticStruct()->SerializeTaggedProperties(
						*this,
						(uint8*)&InComponent.HairInstance,
						FHairWorksInstance::StaticStruct(),
						nullptr
					);

					UHairWorksMaterial::StaticClass()->SerializeTaggedProperties(
						*this,
						(uint8*)InComponent.HairInstance.HairMaterial,
						UHairWorksMaterial::StaticClass(),
						nullptr
					);
				}
			} HairInstancePropertyReader(*CastChecked<UHairWorksComponent>(Component), SavedProperties);
		}

	protected:
		TArray<uint8> SavedProperties;
	};

	return new FHairWorksComponentInstanceData(*this);
}

bool UHairWorksComponent::ShouldCreateRenderState() const
{
	return HairWorks::GetSDK() != nullptr && HairInstance.Hair != nullptr && HairInstance.Hair->AssetId != NvHair::ASSET_ID_NULL;
}

void UHairWorksComponent::CreateRenderState_Concurrent()
{
	// Initialize hair instance
	check(HairWorks::GetSDK() != nullptr && HairInstance.Hair != nullptr && HairInstance.Hair->AssetId != NvHair::ASSET_ID_NULL);

	auto& HairSdk = *HairWorks::GetSDK();
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

	FName HairNormalCenter;

	// Always load from asset to propagate visualization flags
	if(HairInstance.Hair->HairMaterial != nullptr)
	{
		HairInstance.Hair->HairMaterial->GetHairInstanceParameters(DynamicData->HairInstanceDesc, DynamicData->Textures);
		HairNormalCenter = HairInstance.Hair->HairMaterial->HairNormalCenter;
	}

	// Load from component hair material
	if(HairInstance.HairMaterial != nullptr && HairInstance.bOverride)
	{
		NvHair::InstanceDescriptor OverideHairDesc;
		HairInstance.HairMaterial->GetHairInstanceParameters(OverideHairDesc, DynamicData->Textures);
		HairNormalCenter = HairInstance.HairMaterial->HairNormalCenter;

		// Propagate visualization flags
#define HairWorksMergeVisFlag(FlagName) OverideHairDesc.m_visualize##FlagName |= DynamicData->HairInstanceDesc.m_visualize##FlagName

		HairWorksMergeVisFlag(Bones);
		HairWorksMergeVisFlag(BoundingBox);
		HairWorksMergeVisFlag(Capsules);
		HairWorksMergeVisFlag(ControlVertices);
		HairWorksMergeVisFlag(GrowthMesh);
		HairWorksMergeVisFlag(GuideHairs);
		HairWorksMergeVisFlag(HairInteractions);
		HairWorksMergeVisFlag(PinConstraints);
		HairWorksMergeVisFlag(ShadingNormals);
		HairWorksMergeVisFlag(ShadingNormalBone);
		HairWorksMergeVisFlag(SkinnedGuideHairs);
#undef HairWorksMergeVisFlag

		if(OverideHairDesc.m_colorizeMode == NvHair::ColorizeMode::NONE)
			OverideHairDesc.m_colorizeMode = DynamicData->HairInstanceDesc.m_colorizeMode;

		DynamicData->HairInstanceDesc = OverideHairDesc;
	}

	// Disable simulation
	if(bForceSkinning)
		DynamicData->HairInstanceDesc.m_simulate = false;

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

void UHairWorksComponent::Serialize(FArchive & Ar)
{
	Super::Serialize(Ar);

	// When it's duplicated in Blueprints editor, instanced reference is shared, not duplicated. This should be a bug. So we have to duplicate hair material, which is a instanced reference, by ourselves.
	if(Ar.IsLoading() && HairInstance.HairMaterial->GetOuter() != this)
	{
		HairInstance.HairMaterial = CastChecked<UHairWorksMaterial>(StaticDuplicateObject(HairInstance.HairMaterial, this));
	}

	check(HairInstance.HairMaterial->GetOuter() == this);

	// Fix object flag for old assets
	HairInstance.HairMaterial->SetFlags(GetMaskedFlags(RF_PropagateToSubObjects));
}

void UHairWorksComponent::PostInitProperties()
{
	// Inherits parent flags. One purpose is to avoid "Graph is linked to private object(s) in an external package." error in UPackage::SavePackage(). Another purpose is to inherit archetype flag.
	HairInstance.HairMaterial = NewObject<UHairWorksMaterial>(this, NAME_None, GetMaskedFlags(RF_PropagateToSubObjects));

	Super::PostInitProperties();
}

// @third party code - END HairWorks
