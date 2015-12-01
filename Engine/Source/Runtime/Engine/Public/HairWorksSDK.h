// @third party code - BEGIN HairWorks
#pragma once

#include "AllowWindowsPlatformTypes.h"
#pragma warning(push)
#pragma warning(disable: 4191)	// For DLL function pointer conversion
#include "../../../ThirdParty/HairWorks/GFSDK_HairWorks.h"
#pragma warning(pop)
#include "HideWindowsPlatformTypes.h"

extern ENGINE_API GFSDK_HairSDK* GHairWorksSDK;
extern ENGINE_API GFSDK_HairConversionSettings GHairWorksConversionSettings;
// @third party code - END HairWorks
