// @third party code - BEGIN HairWorks
#pragma once

#include "Engine/HairWorksInstance.h"
#include "HairWorksComponent.generated.h"

namespace Nv{namespace HairWorks{
	enum HairInstanceId;
}}

/**
* HairWorksComponent manages and renders a hair asset.
*/
UCLASS(ClassGroup = Rendering, meta = (BlueprintSpawnableComponent), HideCategories = (Collision, Base, Object, PhysicsVolume))
class ENGINE_API UHairWorksComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	~UHairWorksComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Hair, meta = (ShowOnlyInnerProperties))
	FHairWorksInstance HairInstance;

	//~ Begin UPrimitiveComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual void OnAttachmentChanged() override;
	//~ End UPrimitiveComponent interface

	//~ Begin UActorComponent interface
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void SendRenderDynamicData_Concurrent() override;
	virtual void CreateRenderState_Concurrent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual FActorComponentInstanceData* GetComponentInstanceData() const override;
	//~ End UActorComponent interface

	//~ Begin USceneComponent interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ End USceneComponent interface.

	//~ Begin UObject interface.
#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)override;
#endif
	//~ End UObject interface.

protected:
	/** Send data for rendering */
	void SendHairDynamicData()const;

	/** Bone mapping */
	void SetupBoneMapping();

	/** Set bones to HairWorks */
	void UpdateBones()const;

	/** Parent skeleton */
	UPROPERTY()
	USkinnedMeshComponent* ParentSkeleton;

	/** Bone remapping */
	TArray<uint16> BoneIndices;
	
	/** Bone look up table */
	TMap<FName, int> BoneNameToIdx;

	mutable TArray<FMatrix> BoneMatrices;

	/** The hair */
	Nv::HairWorks::HairInstanceId HairInstanceId;
};

// @third party code - END HairWorks