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
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#ifndef NV_ARRAY_UTIL_H
#define NV_ARRAY_UTIL_H

namespace Nv {

class MemoryAllocator;

/*! A util used for often used array operations, used with the Array template. 
NOTE! The layout MUST be kept in sync with the layout of the Array template, or things will not work */
struct ArrayUtil
{
	struct Layout
	{
		typedef Layout ThisType;

		Void* m_data;
		MemoryAllocator* m_allocator;
		IndexT m_size;
		IndexT m_capacity;

		Void swap(ThisType& rhs)
		{
			Op::swap(m_data, rhs.m_data);
			Op::swap(m_size, rhs.m_size);
			Op::swap(m_capacity, rhs.m_capacity);
			Op::swap(m_allocator, rhs.m_allocator);
		}
	};

		/// Grows the capacity so there is space for at least one new element. Assumes there isn't space currently.
	static Void expandCapacityByOne(Layout& layout, SizeT elemSize);
		/// Will make capacity at least as large as minCapacity
	static Void expandCapacity(Layout& layout, IndexT minCapacity, SizeT elemSize);
	
		/// Called when there a single element is added but there isn't space
	static IndexT calcCapacityIncrement(IndexT capacity, SizeT elemSize);
		/// Set the capacity of layout to be exactly capacity
		/// NOTE! This has special handling for user data (ie if allocator == NV_NULL)
		/// In this case nothing happens if the size is shrunk, and if it's expanded, the default allocator will be used for the space.
	static Void setCapacity(Layout& layout, IndexT capacity, SizeT elemSize);
		/// Set capacity on ctor
	static Void ctorSetCapacity(Layout& layout, IndexT capacity, SizeT elemSize, MemoryAllocator* alloc);

		/// Ctor using default Ctor all elements between first and up to last
	template <typename T>
	NV_INLINE static void ctorDefault(T* first, T* last);
		/// Ctor using value a all elements between first and up to last
	template <typename T>
	NV_INLINE static void ctor(T* first, T* last, const T& a);
		/// Copy construct first up to last using src.
	template <typename T>
	NV_INLINE static void copyCtor(T* first, T* last, const T* src);
		/// Assign
	template <typename T>
	NV_INLINE static void assign(T* first, T* last, const T* src);
		/// Dtor the array from first to last
	template <typename T>
	NV_INLINE static void dtor(T* first, T* last);
		/// True if all elements are equal
	template <typename T>
	NV_INLINE static Bool equal(const T* a, const T* b, IndexT size);
};

// ---------------------------------------------------------------------------
template <typename T>
/* static */ void ArrayUtil::ctorDefault(T* first, T* last)
{
	for (; first < last; ++first)
		new(first) T;
}
// ---------------------------------------------------------------------------
template <typename T>
/* static */void ArrayUtil::ctor(T* first, T* last, const T& a)
{
	for (; first < last; ++first)
		new(first) T(a);
}
// ---------------------------------------------------------------------------
template <typename T>
/* static */void ArrayUtil::copyCtor(T* first, T* last, const T* src)
{
	for (; first < last; ++first, ++src)
		new (first) T(*src);
}
// ---------------------------------------------------------------------------
template <typename T>
/* static */ void ArrayUtil::dtor(T* first, T* last)
{
	for (; first < last; ++first)
		first->~T();
}
// ---------------------------------------------------------------------------
template <typename T>
/* static */void ArrayUtil::assign(T* first, T* last, const T* src)
{
	for (; first < last; ++first, ++src)
		*first = * src;
}
// ---------------------------------------------------------------------------
template <typename T>
/* static */Bool ArrayUtil::equal(const T* a, const T* b, IndexT size)
{
	for (IndexT i = 0; i < size; i++)
	{
		// Use equality test, as it is more commonly defined
		if (a[i] == b[i]) 
			continue;
		return false;
	}
	return true;
}

} // namespace Nv

#endif
