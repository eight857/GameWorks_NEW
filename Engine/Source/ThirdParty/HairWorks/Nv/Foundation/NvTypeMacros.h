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

#ifndef NV_TYPE_MACROS_H
#define NV_TYPE_MACROS_H

#include "NvAllocateMacros.h"

namespace Nv
{

//! Macros used for type definitions

/// NOTE! That types allocated with this definition, can only be deleted from their most derived 
/// type. That is generally going to be the case, otherwise the dtor will be incorrect.
#define NV_DECLARE_CLASS_BASE(type) \
	public: \
	typedef type ThisType; \
	NV_CLASS_KNOWN_SIZE_ALLOC

#define NV_DECLARE_CLASS(type, parent) \
	public: \
	typedef parent Parent; \
	typedef type ThisType; \
	NV_CLASS_KNOWN_SIZE_ALLOC

// 
#define NV_DECLARE_POLYMORPHIC_CLASS_BASE(type) \
	public: \
	typedef type ThisType; \
	NV_CLASS_UNKNOWN_SIZE_ALLOC

#define NV_DECLARE_POLYMORPHIC_CLASS(type, parent) \
	public: \
	typedef parent Parent; \
	typedef type ThisType; \
	NV_CLASS_UNKNOWN_SIZE_ALLOC


// Type is an interface - it is abstract and can only be created as part of a derived type 
#define NV_DECLARE_INTERFACE_BASE(type) \
	public: \
	typedef type ThisType; \
	NV_CLASS_NO_ALLOC

#define NV_DECLARE_INTERFACE(type, parent) \
	public: \
	typedef parent Parent; \
	typedef type ThisType; \
	NV_CLASS_NO_ALLOC

// The type is polymorphic but cannot be new'd, deleted but can only be created on the stack
#define NV_DECLARE_STACK_POLYMORPHIC_CLASS_BASE(type) \
	NV_DECLARE_INTERFACE_BASE(type)

#define NV_DECLARE_STACK_POLYMORPHIC_CLASS(type, parent) \
	NV_DECLARE_INTERFACE(type, parent)

} // namespace Nv

#endif // NV_TYPE_ALLOCATE_H