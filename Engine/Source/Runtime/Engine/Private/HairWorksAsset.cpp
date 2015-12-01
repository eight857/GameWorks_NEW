// @third party code - BEGIN HairWorks
#include "EnginePrivate.h"
#include "HairWorksSDK.h"
#include "Engine/HairWorksAsset.h"

UHairWorksAsset::UHairWorksAsset(const class FObjectInitializer& ObjectInitializer):
	Super(ObjectInitializer),
	AssetId(GFSDK_HairAssetID_NULL)
{
}

UHairWorksAsset::~UHairWorksAsset()
{
	if(AssetId != GFSDK_HairAssetID_NULL)
		GHairWorksSDK->FreeHairAsset(AssetId);
}

void UHairWorksAsset::Serialize(FArchive & Ar)
{
	Super::Serialize(Ar);
}
// @third party code - END HairWorks
