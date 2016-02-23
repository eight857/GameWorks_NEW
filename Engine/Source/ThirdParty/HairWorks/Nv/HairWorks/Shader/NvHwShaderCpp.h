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

#ifndef NV_HW_SHADER_CPP_H
#define NV_HW_SHADER_CPP_H

// Code that allows the hlsl header to be included in cpp code
#ifndef _CPP
#	error "Can only be included in C++ code"
#endif

#ifndef float4
#define float4			gfsdk_float4
#endif

#ifndef float3
#define float3			gfsdk_float3
#endif

#ifndef float2
#define float2			gfsdk_float2
#endif

#ifndef float4x4
#define float4x4		gfsdk_float4x4
#endif

#ifndef row_major
#define row_major		
#endif

#ifndef float4x4
#define float4x4		gfsdk_float4x4
#endif

#ifndef NOINTERPOLATION
#define	NOINTERPOLATION					
#endif

#endif // NV_HW_SHADER_CPP_H
