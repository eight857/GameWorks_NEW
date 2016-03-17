// @third party code - BEGIN HairWorks
#include "EnginePrivate.h"
#include "HairWorksSDK.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/HairWorksAsset.h"

UHairWorksAsset::UHairWorksAsset(const class FObjectInitializer& ObjectInitializer):
	Super(ObjectInitializer),
	AssetId(NvHw::HAIR_ASSET_ID_NULL)
{
}

UHairWorksAsset::~UHairWorksAsset()
{
	if(AssetId != NvHw::HAIR_ASSET_ID_NULL)
		HairWorks::GetSDK()->freeHairAsset(AssetId);
}

void UHairWorksAsset::Serialize(FArchive & Ar)
{
	Super::Serialize(Ar);
}

void UHairWorksAsset::PostInitProperties()
{
#if WITH_EDITORONLY_DATA
	if(!HasAnyFlags(RF_ClassDefaultObject))
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
#endif

	Super::PostInitProperties();
}
// @third party code - END HairWorks
