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

#ifndef NV_ALLOCATE_MACROS_H
#define NV_ALLOCATE_MACROS_H

#include <Nv/Foundation/NvCommon.h>
#include <Nv/Foundation/NvMemoryAllocator.h>

namespace Nv
{

//! Macros used for setting up allocation on types

#define NV_OPERATOR_NEW_PLACEMENT_NEW \
    static NV_FORCE_INLINE void* operator new(::Nv::SizeT /* size */, void* place) { return place; }  \
    static NV_FORCE_INLINE void operator delete(void* /* place */, void* /*in */) { } 

/// Operator new when the size of the type is not known on delete 
#define NV_OPERATOR_NEW_UNKNOWN_SIZE \
	NV_FORCE_INLINE static void* operator new(::Nv::SizeT size) \
	{ \
		return ::Nv::MemoryAllocator::getInstance()->simpleAllocate(size); \
	} \
    NV_FORCE_INLINE static void operator delete(void* data)  \
	{ \
		::Nv::MemoryAllocator::getInstance()->simpleDeallocate(data); \
	} 

/// Operator new for a type that size does not change, or is known to always be deleted from the most derived type
/// NOTE! Only works if the type defines ThisType, as the NV_DECLARE_CLASS macros do
#define NV_OPERATOR_NEW_KNOWN_SIZE \
	NV_FORCE_INLINE static void* operator new(::Nv::SizeT size) \
	{ \
		NV_ASSERT(size == sizeof(ThisType)); \
		return ::Nv::MemoryAllocator::getInstance()->allocate(size); \
	} \
    NV_FORCE_INLINE static void operator delete(void* data)  \
	{ \
		::Nv::MemoryAllocator::getInstance()->deallocate(data, sizeof(ThisType)); \
	} 

#define NV_OPERATOR_NEW_DISABLE \
	private: \
	static void* operator new(::Nv::SizeT size); \
	public: \
	static void operator delete(void*) { NV_ASSERT(!"Incorrect allocator or use of delete"); } 

#define NV_CLASS_UNKNOWN_SIZE_ALLOC \
	NV_OPERATOR_NEW_UNKNOWN_SIZE \
    NV_OPERATOR_NEW_PLACEMENT_NEW 

#define NV_CLASS_KNOWN_SIZE_ALLOC \
	NV_OPERATOR_NEW_KNOWN_SIZE \
    NV_OPERATOR_NEW_PLACEMENT_NEW 

#define NV_CLASS_NO_ALLOC \
	NV_OPERATOR_NEW_DISABLE

} // namespace Nv

#endif // NV_TYPE_ALLOCATE_H