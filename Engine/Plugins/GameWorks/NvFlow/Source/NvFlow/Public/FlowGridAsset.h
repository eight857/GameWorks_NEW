
#pragma once

// NvFlow begin

#include "GridInteractionNvFlow.h"
#include "FlowGridAsset.generated.h"

UENUM()
enum EFlowGridDimension
{
	EFGD_256 = 8,
	EFGD_512 = 9,
	EFGD_1024 = 10,
};

UCLASS(BlueprintType, hidecategories=object, MinimalAPI)
class UFlowGridAsset : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Grid cell size: defines resolution of simulation*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	float		GridCellSize;

	/** Grid dimension: dimension * cellSize defines size of simulation domain*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	TEnumAsByte<EFlowGridDimension> VirtualGridDimension;

	/** Allows increase of maximum number of cells*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	float		MemoryLimitScale;

	/** Simulation update rate in updates per second*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	float		SimulationRate;

	/** If true, block allocation will update faster at the cost of extra overhead.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	uint32		bLowLatencyMapping : 1;

	/** If true, multiAdapter is used if supported*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	uint32		bMultiAdapterEnabled : 1;

	/** If true, grid affects GPU particles*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	uint32		bParticlesInteractionEnabled : 1;

	/** Enum indicating what interaction channel this object has */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	TEnumAsByte<enum EInteractionChannelNvFlow> InteractionChannel;

	/** Custom Channels for Responses */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	struct FInteractionResponseContainerNvFlow ResponseToInteractionChannels;

	/** If true, higher res density and volume rendering are disabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	uint32		bParticleModeEnabled : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	float		ParticleToGridAccelTimeConstant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	float		ParticleToGridDecelTimeConstant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	float		ParticleToGridThresholdMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	float		GridToParticleAccelTimeConstant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	float		GridToParticleDecelTimeConstant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	float		GridToParticleThresholdMultiplier;

	/** Gravity vector for use by buoyancy*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	FVector		Gravity;

	/** If true, run older less accurate pressure solver*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pressure")
	uint32		bPressureLegacyMode : 1;

	/** Enum indicating what type of object this should be considered as */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	TEnumAsByte<enum ECollisionChannel> ObjectType;

	/** Custom Channels for Responses*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	struct FCollisionResponseContainer	ResponseToChannels;

	/** Adaptive ScreenPercentage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	uint32		bAdaptiveScreenPercentage : 1;

	/** Target Frame Time for Adaptive, in ms */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (UIMin = 0.f, UIMax = 50.f))
	float		AdaptiveTargetFrameTime;

	/** Maximum ScreenPercentage, Default Value is Adaptive is disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (UIMin = 0.1f, UIMax = 1.0f))
	float		MaxScreenPercentage;

	/** Minimum ScreenPercentage when Adaptive is enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (UIMin = 0.1f, UIMax = 1.0f))
	float		MinScreenPercentage;

	/** Debug rendering*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	uint32		bDebugWireframe : 1;
	static bool sGlobalDebugDraw;
	static uint32 sGlobalRenderChannel;
	static uint32 sGlobalRenderMode;
	static uint32 sGlobalMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Field")
	uint32		bDistanceFieldCollisionEnabled : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Field")
	float		MinActiveDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Field")
	float		MaxActiveDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Field")
	float		VelocitySlipFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Field")
	float		VelocitySlipThickness;

	// Helper methods

	FORCEINLINE static float GetFlowToUE4Scale() { return 100.0f; }

	FORCEINLINE static uint32 GetVirtualGridDimension(TEnumAsByte<EFlowGridDimension> GridDimension) { return static_cast<uint32>(1 << GridDimension); }

	FORCEINLINE static float GetVirtualGridExtent(float GridCellSize, TEnumAsByte<EFlowGridDimension> GridDimension) 
	{ 
		return GridCellSize * GetVirtualGridDimension(GridDimension) * 0.5f;
	}

	FORCEINLINE float GetVirtualGridDimension() const { return GetVirtualGridDimension(VirtualGridDimension); }
	FORCEINLINE float GetVirtualGridExtent() const { return GetVirtualGridExtent(GridCellSize, VirtualGridDimension); }
};

// NvFlow end