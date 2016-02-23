// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright (c) 2013-2015 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#include "NvApiHandle.h"

#include <Nv/Foundation/NvLogger.h>

// For sprintf...
#include <stdio.h>

namespace Nv {

/* static */const Char* ApiHandle::getApiText(EApiType apiType)
{
	switch (apiType)
	{
		case ApiType::DX11:		return "Dx11";
		case ApiType::DX12:		return "Dx12";
		case ApiType::VULCAN:	return "Vulcan";
		case ApiType::METAL:	return "Metal";
		case ApiType::OPEN_GL:	return "OpenGl";
		default: return "Unknown";
	}
}

/* static */Bool ApiHandle::isGenericCastFailure(Int fromType, Int toType, EApiType apiType)
{
	EApiType fromApiType = getApiType(fromType);
	EApiType toApiType = getApiType(toType);
	return (fromApiType != toApiType || fromApiType != apiType);
}

/// Log that there is a cast failure
/* static */Void ApiHandle::logCastFailure(Int fromType, Int toType, EApiType apiType)
{
	EApiType fromApiType = getApiType(fromType);
	EApiType toApiType = getApiType(toType);

	char buffer[1024];
	if (fromApiType != toApiType)
	{
		sprintf_s(buffer, NV_COUNT_OF(buffer), "Cannot convert type - different apis %s->%s for API expected is %s", getApiText(getApiType(fromType)), getApiText(getApiType(toType)), getApiText(apiType));

		NV_LOG_WARN(buffer);
		return;
	}
	if (fromApiType != apiType)
	{
		sprintf_s(buffer, NV_COUNT_OF(buffer), "Expecting something in api %s, but have %s", getApiText(getApiType(fromType)), getApiText(apiType));
		NV_LOG_WARN(buffer);
		return;
	}

	// Handle the generic situation, where all we know is the cast can't happen
	{
		sprintf_s(buffer, NV_COUNT_OF(buffer), "Cannot cast %s %d to %s %d", getApiText(fromApiType), getSubType(fromType), getApiText(toApiType), getSubType(toType));
		NV_LOG_WARN(buffer);
	}
}

/* static */Void ApiHandle::logSubTypeCastFailure(const Char* fromSubText, const Char* toSubText, EApiType apiType)
{
	char buffer[1024];
	sprintf_s(buffer, NV_COUNT_OF(buffer), "Cannot cast %s to %s on api %s", fromSubText, toSubText, getApiText(apiType));
	NV_LOG_WARN(buffer);
}

} // namespace Nv
