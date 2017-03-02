/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#ifndef NV_HAIR_COMMON_H
#define NV_HAIR_COMMON_H

#include <Nv/Common/NvCoCommon.h>

/** \addtogroup HairWorks
@{
*/

// Set up the default namespaces
namespace nvidia {
namespace HairWorks {
} // namespace HairWorks
namespace Hair = HairWorks;
} // namespace nvidia
namespace NvHair = nvidia::Hair;

#pragma pack(push,8) // Make sure we have consistent structure packings

typedef struct  {	
	float x;
	float y;
} gfsdk_float2;

typedef struct {
	float x;
	float y;
	float z;
} gfsdk_float3;

typedef struct {
	float x;
	float y;
	float z;
	float w;
} gfsdk_float4;

  // implicit row major
typedef struct {
	float _11, _12, _13, _14;
	float _21, _22, _23, _24;
	float _31, _32, _33, _34;
	float _41, _42, _43, _44;
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

#pragma pack(pop)

/** @} */

#endif // NV_HAIR_COMMON_H