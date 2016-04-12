// @third party code - BEGIN HairWorks
#include "EnginePrivate.h"
#include <Nv/Common/NvCoMemoryReadStream.h>
#include "HairWorksSDK.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/HairWorksAsset.h"

UHairWorksAsset::UHairWorksAsset(const class FObjectInitializer& ObjectInitializer):
	Super(ObjectInitializer),
	AssetId(NvHair::ASSET_ID_NULL)
{
}

UHairWorksAsset::~UHairWorksAsset()
{
	if(AssetId != NvHair::ASSET_ID_NULL)
		HairWorks::GetSDK()->freeAsset(AssetId);
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

void UHairWorksAsset::PostLoad()
{
	Super::PostLoad();

	// Preload asset
	static TAutoConsoleVariable<int> CVarPreloadAsset(TEXT("r.HairWorks.PreloadAsset."), 1, TEXT(""), ECVF_Default);
	if(CVarPreloadAsset.GetValueOnGameThread() == 0)
		return;

	if(HairWorks::GetSDK() == nullptr)
		return;

	// Create hair asset
	check(AssetId == NvHair::ASSET_ID_NULL);
	NvCo::MemoryReadStream ReadStream(AssetData.GetData(), AssetData.Num());
	HairWorks::GetSDK()->loadAsset(&ReadStream, AssetId, nullptr, &HairWorks::GetAssetConversionSettings());
}
// @third party code - END HairWorks
