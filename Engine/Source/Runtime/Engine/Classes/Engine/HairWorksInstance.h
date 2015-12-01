// @third party code - BEGIN HairWorks
#pragma once

#include "HairWorksInstance.generated.h"

enum GFSDK_HairAssetID;
class UHairWorksMaterial;
class UHairWorksAsset;

USTRUCT(BlueprintType)
struct ENGINE_API FHairWorksInstance
{
	GENERATED_USTRUCT_BODY()

	/** Assign HairWorks asset here. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Asset)
	UHairWorksAsset* Hair;

	/** Whether override Hair Material of the HairWorks asset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Asset)
	bool bOverride = false;

	/** A Hair Material to override the Hair Material of the HairWorks asset. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category = Hair)
	UHairWorksMaterial* HairMaterial;
};
// @third party code - END HairWorks

