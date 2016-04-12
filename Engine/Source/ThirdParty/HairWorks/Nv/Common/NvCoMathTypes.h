/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#ifndef NV_CO_MATH_TYPES_H
#define NV_CO_MATH_TYPES_H

#include <Nv/Core/1.0/NvTypes.h>

/** \addtogroup common
@{
*/

namespace nvidia  {
namespace Common {

// Vec types are unaligned. 
struct Vec2
{
	Float32 x, y;
};

struct Vec3
{
	Float32 x, y, z;
};

struct Vec4
{
	Float32 x, y, z, w;
};

struct Mat4
{
	Vec4 rows[4];
};

/// Quaternion
struct Quaternion
{
	Float32 x, y, z, w;
};

struct DualQuaternion
{
	// Interpolation is from q0 to q1
	Quaternion q0, q1;
};

struct Transform3
{
	Quaternion q;
	Vec3 p;
};

struct Bounds3
{
	Vec3 minimum;
	Vec3 maximum;
};

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Aligned versions !!!!!!!!!!!!!!!!!!!!!!! */

/// AlignedVec types are aligned for faster Simd type operations

/// 2D point or vector
NV_ALIGN(8, struct AlignedVec2)
{
	Float32 x, y;
};

/// 4D point or vector
NV_ALIGN(16, struct AlignedVec4)
{
	Float32 x, y, z, w;
};

/// 3 matrix, implicit row major
/// NOTE that the rows are Vec4 to allow easy map to
struct AlignedMat3
{
	AlignedVec4 rows[3];
};

/// 4x4 matrix, implicit row major
struct AlignedMat4
{
	AlignedVec4 rows[4];	
};

/// Quaternion
NV_ALIGN(16, struct AlignedQuaternion)
{
	Float32 x, y, z, w;
};

/// Rigid body transformation
struct AlignedTransform3
{
	AlignedQuaternion q;
	AlignedVec4 p;
};

/// Axis-aligned bounding box
struct AlignedBounds3
{
	AlignedVec4 minimum;
	AlignedVec4 maximum;
};

struct AlignedDualQuaternion
{
	// Interpolation is from q0 to q1
	AlignedQuaternion q0, q1;
};


/* Some simple methods to set up */

NV_FORCE_INLINE Void setUnitX(Vec4& in) { in.x = 1.0f; in.y = 0.0f; in.z = 0.0f; in.w = 0.0f; }
NV_FORCE_INLINE Void setUnitY(Vec4& in) { in.x = 0.0f; in.y = 1.0f; in.z = 0.0f; in.w = 0.0f; }
NV_FORCE_INLINE Void setUnitZ(Vec4& in) { in.x = 0.0f; in.y = 0.0f; in.z = 1.0f; in.w = 0.0f; }
NV_FORCE_INLINE Void setUnitW(Vec4& in) { in.x = 0.0f; in.y = 0.0f; in.z = 0.0f; in.w = 1.0f; }

NV_FORCE_INLINE Void setAll(Vec4& in, Float32 v) { in.x = in.y = in.z = in.w = v; }
NV_FORCE_INLINE Void setAll(Vec3& in, Float32 v) { in.x = in.y = in.z = v; }

NV_FORCE_INLINE Void setIdentity(Mat4& in) { setUnitX(in.rows[0]); setUnitY(in.rows[1]); setUnitZ(in.rows[2]); setUnitW(in.rows[3]); }

} // namespace Common 
} // namespace nvidia

/** @} */

#endif // NV_CO_MATH_TYPES_H
