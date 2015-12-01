// @third party code - BEGIN HairWorks
#include "AssetToolsPrivatePCH.h"
#include "Engine/HairWorksAsset.h"
#include "AssetTypeActions_HairWorks.h"

FText FAssetTypeActions_HairWorks::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_HairWorks", "HairWorks");
}

UClass* FAssetTypeActions_HairWorks::GetSupportedClass() const
{
	return UHairWorksAsset::StaticClass();
}

void FAssetTypeActions_HairWorks::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const
{
	for (auto& Asset : TypeAssets)
	{
		auto* Hair = CastChecked<UHairWorksAsset>(Asset);
		Hair->AssetImportData->ExtractFilenames(OutSourceFilePaths);
	}
}
// @third party code - END HairWorks
