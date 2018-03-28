// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/StaticMeshActor.h"
#include "AreaLightActor.generated.h"

namespace VXGI
{
	class IAreaLightTexture;
}

/**
 * A plane that emits light into the world through VXAL.
 */
UCLASS()
class ENGINE_API AAreaLightActor : public AStaticMeshActor
{
	GENERATED_UCLASS_BODY()
public:
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere)
	FLinearColor LightColor;

	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, meta = (ClampMin = "0.0"))
	float LightIntensityDiffuse;

	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, meta = (ClampMin = "0.0"))
	float LightIntensitySpecular;

	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere)
	uint32 bEnableLight : 1;

	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere)
	uint32 bEnableShadows : 1;

	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere)
	uint32 bEnableScreenSpaceShadows : 1;

	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Quality;

	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScreenSpaceShadowQuality;

	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, meta = (ClampMin = "0.25", ClampMax = "2.0"))
	float DirectionalSamplingRate;

	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, meta = (ClampMin = "0.125", ClampMax = "1.0"))
	float MaxProjectedArea;

	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LightSurfaceOffset;

	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "0.99"))
	float TemporalWeight;

	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TemporalDetailReconstruction;

	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere)
	uint32 bEnableNeighborhoodColorClamping : 1;

	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, meta = (ClampMin = "0.1", ClampMax = "100.0", UIMax = "5.0"))
	float NeighborhoodClampingWidth;

	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere)
	class UTexture2D* LightTexture;

	bool bVxgiTextureUpdateRequired;
	class VXGI::IAreaLightTexture* VxgiTexture;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR	
};
