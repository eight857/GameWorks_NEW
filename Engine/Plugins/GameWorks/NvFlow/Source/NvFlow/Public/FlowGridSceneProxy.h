// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
FlowGridSceneProxy.h:
=============================================================================*/

// NvFlow begin

/*=============================================================================
FFlowGridSceneProxy
=============================================================================*/

#pragma once

namespace NvFlow
{
	const float scale = 100.f;
	const float scaleInv = 1.0f / scale;
	const float sdfRadius = 0.8f;
	const float angularScale = PI / 180.f;
}

struct FFlowGridRenderParams
{
	// NvFlowVolumeRenderParams
	NvFlowVolumeRenderMode RenderMode;
	NvFlowGridTextureChannel RenderChannel;
	uint32 bAdaptiveScreenPercentage : 1;
	float AdaptiveTargetFrameTime;
	float MaxScreenPercentage;
	float MinScreenPercentage;
	uint32 bDebugWireframe : 1;

	uint32 bVolumeShadowEnabled : 1;
	float ShadowIntensityScale;
	float ShadowMinIntensity;
	NvFlowFloat4 ShadowBlendCompMask;
	float ShadowBlendBias;

	uint32 ShadowResolution;
	float ShadowFrustrumScale;
	float ShadowMinResidentScale;
	float ShadowMaxResidentScale;
};

typedef void* FlowMaterialKeyType;
typedef void* FlowRenderMaterialKeyType;

struct FFlowRenderMaterialParams : NvFlowRenderMaterialParams
{
	FlowRenderMaterialKeyType Key;
	// Color map
	TArray<FLinearColor> ColorMap;
};

struct FFlowMaterialParams
{
	NvFlowGridMaterialParams GridParams;

	TArray<FFlowRenderMaterialParams> RenderMaterials;
};

struct FFlowGridProperties
{
	// indicates if grid should be allocated
	int32 bActive : 1;

	// multi-GPU enable, requires reset if changed
	uint32 bMultiAdapterEnabled : 1;
	uint32 bAsyncComputeEnabled : 1;

	uint32 bParticlesInteractionEnabled : 1;
	TEnumAsByte<enum EInteractionChannelNvFlow> InteractionChannel;
	struct FInteractionResponseContainerNvFlow ResponseToInteractionChannels;

	uint32 bParticleModeEnabled : 1;

	float ParticleToGridAccelTimeConstant;
	float ParticleToGridDecelTimeConstant;
	float ParticleToGridThresholdMultiplier;
	float GridToParticleAccelTimeConstant;
	float GridToParticleDecelTimeConstant;
	float GridToParticleThresholdMultiplier;

	// target simulation time step
	float SubstepSize;

	// virtual extents
	FVector VirtualGridExtents;

	float GridCellSize;

	// simulation parameters
	NvFlowGridDesc GridDesc;
	NvFlowGridParams GridParams;
	TArray<NvFlowGridEmitParams> GridEmitParams;
	TArray<NvFlowGridEmitParams> GridCollideParams;
	TArray<NvFlowShapeDesc> GridEmitShapeDescs;
	TArray<NvFlowShapeDesc> GridCollideShapeDescs;


	uint32 bDistanceFieldCollisionEnabled : 1;
	float MinActiveDistance;
	float MaxActiveDistance;
	float VelocitySlipFactor;
	float VelocitySlipThickness;

	// rendering parameters
	int32 ColorMapResolution;
	FFlowGridRenderParams RenderParams;

	TArray<FlowMaterialKeyType> GridEmitMaterialKeys;
	FlowMaterialKeyType DefaultMaterialKey;

	TArray<TPair<FlowMaterialKeyType, FFlowMaterialParams> > Materials;
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