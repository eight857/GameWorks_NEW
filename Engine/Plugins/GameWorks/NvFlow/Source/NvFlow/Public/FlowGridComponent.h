
/**
 *	NvFlow - Grid Component
 */
#pragma once

// NvFlow begin

#include "FlowGridSceneProxy.h"
#include "FlowGridComponent.generated.h"

#if STATS

DECLARE_STATS_GROUP(TEXT("Flow"), STATGROUP_Flow, STATCAT_Advanced);

enum EFlowStats
{
	// Container stats
	STAT_Flow_Tick,
	STAT_Flow_UpdateShapes,
	STAT_Flow_UpdateColorMap,
	STAT_Flow_SimulateGrids,
	STAT_Flow_RenderGrids,
	STAT_Flow_GridCount,
	STAT_Flow_EmitterCount,
	STAT_Flow_ColliderCount,
};

DECLARE_CYCLE_STAT_EXTERN(TEXT("RenderThread, Simulate Grids"), STAT_Flow_SimulateGrids, STATGROUP_Flow, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("RenderThread, Render Grids"), STAT_Flow_RenderGrids, STATGROUP_Flow, );

#endif

struct FTimeStepper
{
	float DeltaTime;
	float TimeError;
	float FixedDt;
	int32 MaxSteps;

	FTimeStepper();
	int32 GetNumSteps(float TimeStep);
};

UCLASS(ClassGroup = Physics, config = Engine, editinlinenew, HideCategories = (Physics, Collision, Activation, PhysX), meta = (BlueprintSpawnableComponent), MinimalAPI)
class UFlowGridComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	/** The flow grid asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Grid)
	class UFlowGridAsset* FlowGridAsset;

	UFUNCTION(BlueprintCallable, Category=Grid)
	class UFlowGridAsset* CreateOverrideAsset();

	UFUNCTION(BlueprintCallable, Category = Grid)
	void SetOverrideAsset(class UFlowGridAsset* asset);

	class UFlowGridAsset* FlowGridAssetOverride;
	class UFlowGridAsset** FlowGridAssetCurrent;

	/** If true, Flow Grid will collide with emitter/colliders. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Grid)
	uint32 bFlowGridCollisionEnabled : 1;

	// UObject interface
	virtual void CreatePhysicsState() override;
	virtual void DestroyPhysicsState() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

#if WITH_EDITOR	
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
#endif
	// End of UObject interface

	// Begin UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End UActorComponent interface

	// USceneComponent interface
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport = ETeleportType::None) override;

	// Begin UPrimitiveComponent interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	// End UPrimitiveComponent interface.

	FFlowGridProperties FlowGridProperties;
protected:

	// Begin UActorComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const override;
	virtual void SendRenderTransform_Concurrent() override;
	virtual void SendRenderDynamicData_Concurrent() override;
	// End UActorComponent Interface

	void UpdateShapes();

	FTimeStepper TimeStepper;
};

// NvFlow end