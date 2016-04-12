/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

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
