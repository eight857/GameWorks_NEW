// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
FlowGridSceneProxy.h:
=============================================================================*/

// NvFlow begin

/*=============================================================================
FFlowGridSceneProxy
=============================================================================*/

#pragma once

enum EFlowGeometryType
{
	EFGT_eSphere = 0,
	EFGT_eBox = 1,
	EFGT_eCapsule = 2
};

struct FFlowSphere
{
	float Radius;
};

struct FFlowBox
{
	float Extends[3];
};

struct FFlowCapsule
{
	float HalfHeight;
	float Radius;
};

struct FFlowShape
{
	EFlowGeometryType GeometryType;

	union FGeometry
	{
		FFlowSphere Sphere;
		FFlowBox Box;
		FFlowCapsule Capsule;
	};

	FGeometry Geometry;

	FTransform Transform;
	FTransform PreviousTransform;
	FVector CenterOfRotationOffset;
	FVector AngularVelocity;
	FVector LinearVelocity;

	FVector CollisionCenterOfRotationOffset;
	FVector CollisionAngularVelocity;
	FVector CollisionLinearVelocity;
};

struct FFlowEmitter
{
	FFlowShape Shape;
	FVector	LinearVelocity;
	FVector	AngularVelocity;
	float Density;
	float Temperature;
	float Fuel;
	float FuelReleaseTemp;
	float FuelRelease;
	float AllocationPredict;
	float AllocationScale;
	float CollisionFactor;
	float EmitterInflate;
	float CoupleRate;
	float VelocityMask;
	float DensityMask;
	float TemperatureMask;
	float FuelMask;
	uint32 NumSubsteps;
};

struct FFlowGridProperties
{
	int32 bActive : 1;
	FVector VirtualGridExtents;
	float SubstepSize;

	//NvFlowGridDesc
	FVector HalfSize;
	FIntVector VirtualDim;
	float MemoryLimitScale;
	uint32 bMultiAdapterEnabled : 1;

	//NvFlowGridParams
	float VelocityWeight;
	float DensityWeight;
	float TempWeight;
	float FuelWeight;
	float VelocityThreshold;
	float DensityThreshold;
	float TempThreshold;
	float FuelThreshold;
	float ImportanceThreshold;
	FVector Gravity;
	float VelocityDamping;
	float DensityDamping;
	float VelocityFade;
	float DensityFade;
	float VelocityMacCormackBlendFactor;
	float DensityMacCormackBlendFactor;
	float VorticityStrength;
	float CombustionIgnitionTemperature;
	float CombustionCoolingRate;

	//NvFlowVolumeRenderParams
	float RenderingAlphaScale;
	uint32 RenderingMode;
	uint32 bAdaptiveScreenPercentage : 1;
	float AdaptiveTargetFrameTime;
	float MaxScreenPercentage;
	float MinScreenPercentage;
	uint32 bDebugWireframe : 1;

	//Color map
	TArray<FLinearColor> ColorMap;
	float ColorMapMinX;
	float ColorMapMaxX;

	//Emitters
	TArray<FFlowEmitter> Emitters;

	//Collider Shapes
	TArray<FFlowShape> Colliders;
};

class UFlowGridComponent;

class FFlowGridSceneProxy : public FPrimitiveSceneProxy
{
public:

	FFlowGridSceneProxy(UFlowGridComponent* Component);

	virtual ~FFlowGridSceneProxy();

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View) {}

	virtual void CreateRenderThreadResources() override;

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const;

	virtual uint32 GetMemoryFootprint(void) const { return(sizeof(*this) + GetAllocatedSize()); }

	uint32 GetAllocatedSize(void) const { return(FPrimitiveSceneProxy::GetAllocatedSize()); }

	void SetDynamicData_RenderThread(const FFlowGridProperties& FlowGridProperties);
	void Simulate_RenderThread(int32 NumSubSteps);

public:

	// resources managed by game thread
	FFlowGridProperties FlowGridProperties;

	// shared resources 
	int32 NumScheduledSubsteps;

	// resources managed in render thread
	void* scenePtr;
	void(*cleanupSceneFunc)(void*);
};

// NvFlow end