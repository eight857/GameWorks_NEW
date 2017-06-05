// @third party code - BEGIN HairWorks
#pragma once

#include "PrimitiveComponent.h"
#include "Engine/HairWorksInstance.h"
#include "HairWorksComponent.generated.h"

namespace nvidia{namespace HairWorks{
	enum InstanceId;
}}

/**
* HairWorksComponent manages and renders a hair asset.
*/
UCLASS(ClassGroup = Rendering, meta = (BlueprintSpawnableComponent), HideCategories = (Collision, Base, Object, PhysicsVolume))
class ENGINE_API UHairWorksComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Hair, meta = (ShowOnlyInnerProperties))
	FHairWorksInstance HairInstance;

	//~ Begin UPrimitiveComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual void OnAttachmentChanged() override;
	//~ End UPrimitiveComponent interface

	//~ Begin UActorComponent interface
	virtual void SendRenderDynamicData_Concurrent() override;
	virtual bool ShouldCreateRenderState() const override;
	virtual void CreateRenderState_Concurrent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual FActorComponentInstanceData* GetComponentInstanceData() const override;
	//~ End UActorComponent interface

	//~ Begin USceneComponent interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ End USceneComponent interface.

	//~ Begin UObject interface.
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostInitProperties() override;
	//~ End UObject interface.

protected:
	/** Send data for rendering */
	void SendHairDynamicData(bool bForceSkinning = false)const;

	/** Bone mapping */
	void SetupBoneMapping();

	/** Update bones */
	void UpdateBoneMatrices()const;

	/** Parent skeleton */
	UPROPERTY()
	USkinnedMeshComponent* ParentSkeleton;

	/** Bone remapping */
	TArray<uint16> BoneIndices;

	/** Skinning data*/
	mutable TArray<FMatrix> BoneMatrices;
};

// @third party code - END HairWorks