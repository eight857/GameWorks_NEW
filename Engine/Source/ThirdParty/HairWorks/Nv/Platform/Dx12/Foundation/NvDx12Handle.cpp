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

#include <Nv/Foundation/NvCommon.h>

#include <Nv/Foundation/NvLogger.h>

#include "NvDx12Handle.h"

namespace Nv {

/* static */const Char* Dx12Type::getSubTypeText(Dx12SubType::Enum subType)
{
	switch (subType)
	{
		// Make sure all of your subtypes are represented here...
		case Dx12SubType::COUNT_OF: break;
		case Dx12SubType::UNKNOWN:	return "Unknown";
		case Dx12SubType::CONTEXT:	return "ID3D11DeviceContext";
		case Dx12SubType::DEVICE:	return "ID3D11Device";
		case Dx12SubType::BUFFER:	return "ID3D11Buffer";
		case Dx12SubType::FLOAT32:	return "Float32";
		case Dx12SubType::DEPTH_STENCIL_VIEW: return "ID3D11DepthStencilView";
		case Dx12SubType::SHADER_RESOURCE_VIEW: return "ID3D11ShaderResourceView";
	}
	NV_ALWAYS_ASSERT();
	return "Unknown";
}

/* static */Void* Dx12Type::handlePtrCast(Int fromType, Int toType)
{
	// Handle null type
	if (fromType == 0)
	{
		return NV_NULL;
	}
	castFailure(fromType, toType);
	return NV_NULL;
}

/* static */Void* Dx12Type::handleCast(Int fromType, Int toType)
{
	// Handle null type
	if (fromType == 0)
	{
		return NV_NULL;
	}
	castFailure(fromType, toType);
	return NV_NULL;
}

/* static */Void Dx12Type::logCastFailure(Int fromType, Int toType)
{
	// Handles all the classic cast failures
	if (!ApiHandle::isGenericCastFailure(fromType, toType, ApiType::DX11))
	{
		// Must be the right api, but wrong subType
		Dx12SubType::Enum fromSubType = (Dx12SubType::Enum)ApiHandle::getSubType(fromType);
		Dx12SubType::Enum toSubType = (Dx12SubType::Enum)ApiHandle::getSubType(toType);
		return ApiHandle::logSubTypeCastFailure(getSubTypeText(fromSubType), getSubTypeText(toSubType), ApiType::DX11);
	}
	return ApiHandle::logCastFailure(fromType, toType, ApiType::DX11);
}

/* static */Void Dx12Type::castFailure(Int fromType, Int toType)
{
	logCastFailure(fromType, toType);
	// Make it assert on a debug build
	NV_ALWAYS_ASSERT();
}

} // namespace Nv
