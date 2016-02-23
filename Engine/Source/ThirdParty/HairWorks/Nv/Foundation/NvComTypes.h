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

#ifndef _NV_COM_TYPES_H_
#define _NV_COM_TYPES_H_

#include "NvResult.h"

#define NV_MCALL NV_STDCALL

// Macro for declaring if a method is no throw. Should be set before the return parameter. 
#ifdef NV_WINDOWS_FAMILY
#	define NV_NO_THROW __declspec(nothrow) 
#else
#	define NV_NO_THROW
#endif

namespace Nv {

struct Guid
{
	UInt32 m_data1;     ///< Low field of the timestamp
	UInt16 m_data2;     ///< Middle field of the timestamp
	UInt16 m_data3;     ///< High field of the timestamp with multiplexed version number
	UInt8  m_data4[8];  ///< 0, 1 = clock_seq_hi_and_reserved, clock_seq_low, followed by 'spatially unique node' (48 bits) 
};

/// ! Must be kept in sync with IUnknown
class IForwardUnknown
{
public:
	virtual NV_NO_THROW Result NV_MCALL QueryInterface(const Guid& iid, Void* objOut) = 0;
	virtual NV_NO_THROW UInt NV_MCALL AddRef() = 0;
	virtual NV_NO_THROW UInt NV_MCALL Release() = 0;
};

} // namespace Nv

#endif
