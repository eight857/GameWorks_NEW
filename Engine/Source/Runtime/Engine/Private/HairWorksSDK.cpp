// @third party code - BEGIN HairWorks
#include "EnginePrivate.h"

#include "HairWorksSDK.h"

ENGINE_API GFSDK_HairSDK* GHairWorksSDK = nullptr;
ENGINE_API GFSDK_HairConversionSettings GHairWorksConversionSettings;
ENGINE_API ID3D11DeviceContext* GHairWorksDeviceContext = nullptr;
ID3D11ShaderResourceView* (*HairWorksGetSrvFromTexture)(FRHITexture2D*) = nullptr;

DEFINE_LOG_CATEGORY(LogHairWorks);

ENGINE_API void HairWorksInitialize(ID3D11Device* D3DDevice, ID3D11DeviceContext* D3DContext, ID3D11ShaderResourceView* GetSrvFromTexture(FRHITexture2D*))
{
	// Initialize SDK
	FString LibPath = FPaths::EngineDir() / TEXT("Binaries/ThirdParty/HairWorks/GFSDK_HairWorks.win");

#if PLATFORM_64BITS
	LibPath += TEXT("64");
#else
	LibPath += TEXT("32");
#endif

	LibPath += TEXT(".dll");

	GHairWorksSDK = GFSDK_LoadHairSDK(TCHAR_TO_ANSI(*LibPath));
	if(GHairWorksSDK == nullptr)
	{
		UE_LOG(LogHairWorks, Error, TEXT("Failed to initialize HairWorks."));
		return;
	}

	GHairWorksSDK->InitRenderResources(D3DDevice, D3DContext);

	GHairWorksDeviceContext = D3DContext;

	HairWorksGetSrvFromTexture = GetSrvFromTexture;

	GHairWorksConversionSettings.m_targetHandednessHint = GFSDK_HAIR_HANDEDNESS_HINT::GFSDK_HAIR_LEFT_HANDED;
	GHairWorksConversionSettings.m_targetUpAxisHint = GFSDK_HAIR_UP_AXIS_HINT::GFSDK_HAIR_Z_UP;
}

ENGINE_API void HairWorksShutDown()
{
	if(GHairWorksSDK == nullptr)
		return;

	GHairWorksSDK->Release();
	GHairWorksSDK = nullptr;
}
// @third party code - END HairWorks
