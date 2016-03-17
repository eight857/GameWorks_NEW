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

DEFINE_LOG_CATEGORY(LogHairWorks);

namespace HairWorks{
	// Logger
	class Logger: public Nv::Logger
	{
		virtual void log(Nv::ELogSeverity severity, const Nv::Char* text, const Nv::Char* function, const Nv::Char* filename, Nv::Int lineNumber) override
		{
			ELogVerbosity::Type UELogVerb;

			switch(severity)
			{
			case Nv::ELogSeverity::DEBUG_INFO:
				UELogVerb = ELogVerbosity::Log;
				break;
			case Nv::ELogSeverity::INFO:
				UELogVerb = ELogVerbosity::Display;
				break;
			case Nv::ELogSeverity::WARNING:
				UELogVerb = ELogVerbosity::Warning;
				break;
			case Nv::ELogSeverity::NON_FATAL_ERROR:
				UELogVerb = ELogVerbosity::Error;
				break;
			case Nv::ELogSeverity::FATAL_ERROR:
				UELogVerb = ELogVerbosity::Fatal;
				break;
			default:
				UELogVerb = ELogVerbosity::All;
				break;
			}

			if(!LogHairWorks.IsSuppressed(UELogVerb))
			{
				FMsg::Logf(filename, lineNumber, LogHairWorks.GetCategoryName(), UELogVerb, TEXT("%s"), ANSI_TO_TCHAR(text));
			}
		}
	};

	ID3DHelper* D3DHelper = nullptr;

	ENGINE_API NvHw::HairSdk* SDK = nullptr;

	ENGINE_API NvHw::ConversionSettings AssetConversionSettings;

	ENGINE_API NvHw::HairSdk* GetSDK()
	{
		return SDK;
	}

	ENGINE_API const NvHw::ConversionSettings& GetAssetConversionSettings()
	{
		return AssetConversionSettings;
	}

	ENGINE_API ID3DHelper& GetD3DHelper()
	{
		check(GetSDK());
		return *D3DHelper;
	}

	ENGINE_API void Initialize(ID3D11Device& D3DDevice, ID3DHelper& InD3DHelper)
	{
		// Check feature level.
		if(D3DDevice.GetFeatureLevel() < D3D_FEATURE_LEVEL_11_0)
		{
			UE_LOG(LogHairWorks, Error, TEXT("Need D3D_FEATURE_LEVEL_11_0."));
			return;
		}

		// Check multi thread support
		if((D3DDevice.GetCreationFlags() & D3D11_CREATE_DEVICE_SINGLETHREADED) != 0)
		{
			UE_LOG(LogHairWorks, Error, TEXT("Can't work with D3D11_CREATE_DEVICE_SINGLETHREADED."));
			return;
		}

		static Logger logger;

		// Initialize SDK
		FString LibPath = FPaths::EngineDir() / TEXT("Binaries/ThirdParty/HairWorks/NvHairWorksDx11.win");

#if PLATFORM_64BITS
		LibPath += TEXT("64");
#else
		LibPath += TEXT("32");
#endif

		LibPath += TEXT(".dll");

		SDK = NvHw::loadHairSdk(TCHAR_TO_ANSI(*LibPath), NV_HW_VERSION, nullptr, &logger);
		if(SDK == nullptr)
		{
			UE_LOG(LogHairWorks, Error, TEXT("Failed to initialize HairWorks."));
			return;
		}

		SDK->initRenderResources(Nv::Dx11Type::getHandle(&D3DDevice));

		D3DHelper = &InD3DHelper;

		AssetConversionSettings.m_targetHandednessHint = NvHw::HandednessHint::LEFT;
		AssetConversionSettings.m_targetUpAxisHint = NvHw::AxisHint::Z_UP;
	}

	ENGINE_API void ShutDown()
	{
		if(SDK == nullptr)
			return;

		SDK->release();
		SDK = nullptr;
	}
}
// @third party code - END HairWorks
