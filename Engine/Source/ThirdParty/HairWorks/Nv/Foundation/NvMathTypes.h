// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2014 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#ifndef NV_MATH_TYPES_H
#define NV_MATH_TYPES_H

namespace Nv 
{

/// 2D point or vector
typedef struct Float2
{
	float x;
	float y;
} Float2;

/// 3D point or vector
typedef struct Float3
{
	float x;
	float y;
	float z;
} Float3;

/// 4D point or vector
typedef struct Float4
{
	float x;
	float y;
	float z;
	float w;
} Float4;

/// 3x3 matrix, implicit row major
typedef struct Float3x3
{
	float m11, m12, m13;
	float m21, m22, m23;
	float m31, m32, m33;
} Float3x3;

/// 4x4 matrix, implicit row major
typedef struct Float4x4
{
	float m11, m12, m13, m14;
	float m21, m22, m23, m24;
	float m31, m32, m33, m34;
	float m41, m42, m43, m44;
} Float4x4;

/// Quaternion
typedef struct Quat
{
	float x, y, z, w;
} Quat;

/// Rigid body transformation
typedef struct Transform
{
	Quat q;
	Float3 p;
} Transform;

/// Axis-aligned bounding box
typedef struct Bounds3
{
	Float3 minimum;
	Float3 maximum;
} Bounds3;

/// Plane, defined as all points (r,s,t) such that r*x + s*y + t*z + w = 0.
typedef struct Plane
{
	float x, y, z, w;
} Plane;

} // namespace Nv

#endif // NV_MATH_TYPES_H
