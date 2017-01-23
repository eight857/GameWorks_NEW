#pragma once

// NvFlow begin

#include "FlowMaterial.generated.h"

USTRUCT()
struct FFlowMaterialPerComponent
{
	GENERATED_USTRUCT_BODY()

	/** Higher values reduce component value faster (exponential decay curve) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damping", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float Damping;

	/** Fade component value rate in units / sec */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fade", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float Fade;

	/** Higher values make a sharper appearance, but with more artifacts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MacCormack Correction", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float MacCormackBlendFactor;

	/** Minimum absolute value to apply MacCormack correction. Increasing can improve performance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MacCormack Correction", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float MacCormackBlendThreshold;
										
	/** Relative importance of component value for allocation, 0.0 means not important */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Allocation", meta = (ClampMin = 0.0f, UIMin = 0.f, UIMax = 1.0f))
	float AllocWeight;

	/** Minimum component value magnitude that is considered relevant */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Allocation", meta = (ClampMin = 0.0f, UIMin = 0.f, UIMax = 1.0f))
	float AllocThreshold;
};

UCLASS(BlueprintType, hidecategories=object, MinimalAPI)
class UFlowMaterial : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Velocity component parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Components)
	FFlowMaterialPerComponent Velocity;

	/** Density component parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Components)
	FFlowMaterialPerComponent Density;

	/** Temperature component parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Components)
	FFlowMaterialPerComponent Temperature;

	/** Fuel component parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Components)
	FFlowMaterialPerComponent Fuel;

	/** Higher values increase rotation, reduce laminar flow */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vorticity)
	float VorticityStrength;

	/** 0.f means zero velocity magnitude influence on vorticity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vorticity)
	float VorticityVelocityMask;

	/** Minimum temperature for combustion */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combustion)
	float IgnitionTemp;

	/** Burn amount per unit temperature above ignitionTemp */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combustion)
	float BurnPerTemp;

	/** Fuel consumed per unit burn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combustion)
	float FuelPerBurn;
	
	/** Temperature increase per unit burn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combustion)
	float TempPerBurn;

	/** Density increase per unit burn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combustion)
	float DensityPerBurn;

	/** Expansion per unit burn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combustion)
	float DivergencePerBurn;

	/** Buoyant force per unit temperature */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combustion)
	float BuoyancyPerTemp;

	/** Cooling rate, exponential */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combustion)
	float CoolingRate;

	/** Alpha scale */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float AlphaScale;

	/** Color curve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Export, Category = "Rendering")
	class UCurveLinearColor* ColorMap;

	/** Color curve minimum X value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (UIMin = -1.0f, UIMax = 1.0f))
	float ColorMapMinX;

	/** Color curve maximum X value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (UIMin = -1.0f, UIMax = 1.0f))
	float ColorMapMaxX;
};

// NvFlow end
