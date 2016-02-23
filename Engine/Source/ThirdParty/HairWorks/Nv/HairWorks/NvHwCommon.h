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

#ifndef NV_HW_COMMON_H
#define NV_HW_COMMON_H

#include <Nv/Foundation/NvCommon.h>

// Set up the default namespaces
namespace Nv {
namespace HairWorks {
} // namespace HairWorks
namespace Hw = HairWorks;
} // namespace Nv
namespace NvHw = Nv::Hw;

#pragma pack(push,8) // Make sure we have consistent structure packings

// Legacy types...
#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int gfsdk_U32; 
typedef float gfsdk_F32; 

typedef struct  {	
	NvFloat32 x;
    NvFloat32 y;
} gfsdk_float2;

typedef struct {
    NvFloat32 x;
    NvFloat32 y;
    NvFloat32 z;
} gfsdk_float3;

typedef struct {
	NvFloat32 x;
	NvFloat32 y;
	NvFloat32 z;
	NvFloat32 w;
} gfsdk_float4;

  // implicit row major
typedef struct {
    NvFloat32 _11, _12, _13, _14;
    NvFloat32 _21, _22, _23, _24;
    NvFloat32 _31, _32, _33, _34;
    NvFloat32 _41, _42, _43, _44;
} gfsdk_float4x4;


/*! \brief Dual Quaternion data used for dual quaternion skinning option
	\details Defines a dual quaternion structure with 2 pairs of gfsdk_float4.  
	First gfsdk_float4 is interpreted as normal quaternion and the second gfsdk_float4 is interpreted as dual quaternion. */
struct gfsdk_dualquaternion
{
	gfsdk_float4 q0; // first quaternion
	gfsdk_float4 q1; // second (dual) quaternion

	gfsdk_dualquaternion() {}
	gfsdk_dualquaternion(const gfsdk_float4 iq0, const gfsdk_float4 iq1) : q0(iq0), q1(iq1) {}
};

#ifdef __cplusplus
}; //extern "C" {
#endif

#pragma pack(pop)

#endif // NV_HW_COMMON_H