// @third party code - BEGIN HairWorks
#include "EnginePrivate.h"

#include "HairWorksSDK.h"

#include "AllowWindowsPlatformTypes.h"
#include <Nv/Platform/Dx11/Foundation/NvDx11Handle.h>
#pragma warning(push)
#pragma warning(disable: 4191)	// For DLL function pointer conversion
#include <Nv/Platform/Win/HairWorks/NvHwWinLoadSdk.h>
#pragma warning(pop)
#include "HideWindowsPlatformTypes.h"

ENGINE_API NvHw::HairSdk* GHairWorksSDK = nullptr;
ENGINE_API NvHw::ConversionSettings GHairWorksConversionSettings;
ENGINE_API ID3D11DeviceContext* GHairWorksDeviceContext = nullptr;
ID3D11ShaderResourceView* (*HairWorksGetSrvFromTexture)(FRHITexture2D*) = nullptr;

DEFINE_LOG_CATEGORY(LogHairWorks);

ENGINE_API void HairWorksInitialize(ID3D11Device* D3DDevice, ID3D11DeviceContext* D3DContext, ID3D11ShaderResourceView* GetSrvFromTexture(FRHITexture2D*))
{
	// Check feature level.
	if(D3DDevice->GetFeatureLevel() < D3D_FEATURE_LEVEL_11_0)
		return;

	// Initialize SDK
	FString LibPath = FPaths::EngineDir() / TEXT("Binaries/ThirdParty/HairWorks/NvHairWorksDx11.win");

#if PLATFORM_64BITS
	LibPath += TEXT("64");
#else
	LibPath += TEXT("32");
#endif

	LibPath += TEXT(".dll");

	GHairWorksSDK = NvHw::loadHairSdk(TCHAR_TO_ANSI(*LibPath));
	if(GHairWorksSDK == nullptr)
	{
		UE_LOG(LogHairWorks, Error, TEXT("Failed to initialize HairWorks."));
		return;
	}

	GHairWorksSDK->initRenderResources(Nv::Dx11Type::getHandle(D3DDevice), Nv::Dx11Type::getHandle(D3DContext));

	GHairWorksDeviceContext = D3DContext;

	HairWorksGetSrvFromTexture = GetSrvFromTexture;

	GHairWorksConversionSettings.m_targetHandednessHint = NvHw::HandednessHint::LEFT;
	GHairWorksConversionSettings.m_targetUpAxisHint = NvHw::AxisHint::Z_UP;
}

ENGINE_API void HairWorksShutDown()
{
	if(GHairWorksSDK == nullptr)
		return;

	GHairWorksSDK->release();
	GHairWorksSDK = nullptr;
}
// @third party code - END HairWorks
