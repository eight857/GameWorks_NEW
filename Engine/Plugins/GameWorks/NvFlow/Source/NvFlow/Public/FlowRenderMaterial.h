#pragma once

// NvFlow begin

#include "FlowRenderMaterial.generated.h"


UCLASS(BlueprintType, hidecategories=object, MinimalAPI)
class UFlowRenderMaterial : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Global alpha scale for adjust net opacity without color map changes, applied after saturate(alpha) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float AlphaScale;

	/** 1.0 makes material blend fully additive */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float AdditiveFactor;

	/** Component mask for colormap, control what channel drives color map X axis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Export, Category = "Rendering")
	FLinearColor ColorMapCompMask;
	
	/** Component mask to control which channel(s) modulation the alpha */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Export, Category = "Rendering")
	FLinearColor AlphaCompMask;
	
	/** Component mask to control which channel(s) modulates the intensity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Export, Category = "Rendering")
	FLinearColor IntensityCompMask;

	/** Color curve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Export, Category = "Rendering")
	class UCurveLinearColor* ColorMap;

	/** Color curve minimum X value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (UIMin = -1.0f, UIMax = 1.0f))
	float ColorMapMinX;

	/** Color curve maximum X value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (UIMin = -1.0f, UIMax = 1.0f))
	float ColorMapMaxX;

	/** Offsets alpha before saturate(alpha) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Export, Category = "Rendering")
	float AlphaBias;

	/** Offsets intensity before modulating color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Export, Category = "Rendering")
	float IntensityBias;

};

// NvFlow end
