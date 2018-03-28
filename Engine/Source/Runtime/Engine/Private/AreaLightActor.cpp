// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AreaLightActor.cpp: Area light actor class implementation.
=============================================================================*/

#include "Engine/AreaLightActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"


#define LOCTEXT_NAMESPACE "AreaLightActor"

AAreaLightActor::AAreaLightActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, LightColor(FLinearColor::White)
	, LightIntensityDiffuse(1.0f)
	, LightIntensitySpecular(1.0f)
	, bEnableLight(true)
	, LightTexture(nullptr)
	, VxgiTexture(nullptr)
	, bVxgiTextureUpdateRequired(false)
{
#if WITH_GFSDK_VXGI
	VXGI::AreaLight Default;
	Quality = Default.quality;
	DirectionalSamplingRate = Default.directionalSamplingRate;
	MaxProjectedArea = Default.maxProjectedArea;
	LightSurfaceOffset = Default.lightSurfaceOffset;
	bEnableShadows = Default.enableOcclusion;
	bEnableScreenSpaceShadows = Default.enableScreenSpaceOcclusion;
	ScreenSpaceShadowQuality = Default.screenSpaceOcclusionQuality;
	TemporalWeight = Default.temporalReprojectionWeight;
	TemporalDetailReconstruction = Default.temporalReprojectionDetailReconstruction;
	bEnableNeighborhoodColorClamping = Default.enableNeighborhoodClamping;
	NeighborhoodClampingWidth = Default.neighborhoodClampingWidth;
#else
	// Some values to initialize the fields. They don't really matter.
	Quality = 0.5f;
	DirectionalSamplingRate = 1.f;
	MaxProjectedArea = 0.75f;
	LightSurfaceOffset = 0.5f;
	bEnableShadows = true;
	bEnableScreenSpaceShadows = false;
	ScreenSpaceShadowQuality = 0.5f;
	TemporalWeight = 0.5f;
	TemporalDetailReconstruction = 0.5f;
	bEnableNeighborhoodColorClamping = false;
	NeighborhoodClampingWidth = 1.0f;
#endif
}

#if WITH_EDITOR
void AAreaLightActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property != nullptr)
	{
		if (PropertyChangedEvent.Property->GetFName() == FName(TEXT("LightTexture")))
		{
			bVxgiTextureUpdateRequired = true;
		}
	}
}
#endif // WITH_EDITOR	

#undef LOCTEXT_NAMESPACE

