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

#ifndef NV_WIN_MEMORY_ALLOCATOR_H
#define NV_WIN_MEMORY_ALLOCATOR_H

#include <Nv/Foundation/NvCommon.h>

#include <Nv/Foundation/NvMemoryAllocator.h>

namespace Nv
{

/// Windows implementation of memory allocator
class WinMemoryAllocator: public MemoryAllocator
{
public:
	virtual ~WinMemoryAllocator() {}

	void* simpleAllocate(SizeT size) NV_OVERRIDE;
	void simpleDeallocate(const void* pointer) NV_OVERRIDE;
	void* allocate(SizeT size) NV_OVERRIDE;
	void deallocate(const void* ptr, SizeT size) NV_OVERRIDE;
	void* reallocate(void* ptr, SizeT oldSize, SizeT oldUsed, SizeT newSize) NV_OVERRIDE;
	void* alignedAllocate(SizeT size, SizeT align) NV_OVERRIDE;
	void alignedDeallocate(const void* ptr, SizeT align, SizeT size) NV_OVERRIDE;
	void* alignedReallocate(void* ptr, SizeT align, SizeT oldSize, SizeT oldUsed, SizeT newSize) NV_OVERRIDE;

		/// Get the singleton
	NV_FORCE_INLINE static WinMemoryAllocator* getSingleton() { return &s_singleton; }

private:
	static WinMemoryAllocator s_singleton;

	WinMemoryAllocator() {}
};

} // namespace Nv

#endif // NV_WIN_MEMORY_ALLOCATOR_H