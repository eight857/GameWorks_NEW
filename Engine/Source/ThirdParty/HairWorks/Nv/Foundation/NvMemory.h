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

#ifndef NV_MEMORY_H
#define NV_MEMORY_H

namespace Nv {

struct Memory
{
		/// Copy size bytes from src to dst. Note if src and dst overlap, the result is undefined!
		/// @param dst The destination to write the src data to
		/// @param src The source of the data
		/// @param size The amount of bytes to copy from src to dst
	NV_FORCE_INLINE static Void copy(void* NV_RESTRICT dst,  const void* NV_RESTRICT src, SizeT size) { ::memcpy(dst, src, size); }
		/// Zero memory at dst
		/// @param dst The area of memory to zero
		/// @param size The amount of bytes to zero 
	NV_FORCE_INLINE static Void zero(void* NV_RESTRICT dst, SizeT size) { ::memset(dst, 0, size); }
		/// Move bytes from src to dst, taking into account any overlap
		/// @param dst Where to move data to
		/// @param src Where to move from
		/// @param size The amount of data to move
	NV_FORCE_INLINE static Void move(void* dst, void* src, SizeT size) { ::memmove(dst, src, size); }
		/// Set all of the bytes at dst to the lowest byte in value
		/// @param dst Where to write to
		/// @param value The byte value to write 
		/// @param size The amount of bytes to write 
	NV_FORCE_INLINE static Void set(void* dst, Int value, SizeT size) { ::memset(dst, int(value), size); }

		/// Zero the in value type
		/// @param in The value type to zero 
	template <typename T>
	NV_FORCE_INLINE static Void zero(T& in) { ::memset(&in, 0, sizeof(T)); }
};

} // namespace Nv

#endif // NV_MEMORY_H
