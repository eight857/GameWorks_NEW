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

#include "NvString.h"


namespace Nv {

// Leave it writable -> it will only ever get written with zero. If it's made const, it might be put in const memory, and cause a fault on write
static Char s_emptyChars[] = { 0 };

/* static */const SubString SubString::s_empty(NV_NULL, 0);
/* static */const String String::s_empty(s_emptyChars, 0, 1, NV_NULL);

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! SubString !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

SubString::SubString(InitCstr, const Char* in)
{
	SizeT size = ::strlen(in);
	m_chars = const_cast<Char*>(in);
	m_size = IndexT(size);	
}

Bool SubString::equals(const ThisType& rhs) const
{
	if (this == &rhs)
	{
		return true;
	}
	if (rhs.m_size != m_size)
	{
		return false;
	}
	if (m_chars != rhs.m_chars)
	{
		const IndexT size = m_size;
		for (IndexT i = 0; i < size; i++)
		{
			if (m_chars[i] != rhs.m_chars[i])
			{
				return false;
			}
		}
	}
	return true;
}

IndexT SubString::indexOf(Char c) const
{
	const Char* chars = m_chars;
	const IndexT size = m_size;
	for (IndexT i = 0; i < size; i++)
	{
		if (chars[i] == c)
		{
			return i;
		}
	}
	return -1;
}

IndexT SubString::indexOf(Char c, IndexT from) const
{
	NV_ASSERT(from >= 0);
	const Char* chars = m_chars;
	const IndexT size = m_size;
	for (IndexT i = from; i < size; i++)
	{
		if (chars[i] == c)
		{
			return i;
		}
	}
	return -1;
}

IndexT SubString::reverseIndexOf(Char c) const
{
	const Char* start = m_chars;
	const Char* cur = start + m_size - 1;
	for (; cur >= start; --cur)
	{
		if (*cur == c)
		{
			return IndexT(PtrDiffT(cur - start));
		}
	}
	return -1;
}

SubString SubString::subStringWithEnd(IndexT start, IndexT end) const
{
	const IndexT size = m_size;
	// Handle negative indices
	start = (start < 0) ? (size + start) : start;
	end = (end < 0) ? (size + end) : end;

	NV_ASSERT(start >= 0 && end >= start);

	// Clamp to end
	end = (end >= size) ? size : end;
	start = (start > end) ? end : start;

	return SubString(m_chars + start, end - start);
}

SubString SubString::subStringWithStart(IndexT start, IndexT subSize) const
{
	NV_ASSERT(subSize >= 0);
	const IndexT size = m_size;
	// Handle negative wrap around, and clamp if off end
	if (start < 0)
	{
		start = start + size;
		start = (start < 0) ? 0 : start;
	}
	else
	{
		start = (start > size) ? size : start;
	}
	// Handle size
	subSize = (start + subSize > size) ? (size - start) : subSize;
	return SubString(m_chars + start, subSize);
}

SubString SubString::head(IndexT end) const
{
	const IndexT size = m_size;
	if (end < 0)
	{
		end = size + end;
		end = (end < 0) ? 0 : end;
	}
	else
	{
		end = (end > size) ? size : end;
	}
	return SubString(m_chars, end);
}

SubString SubString::tail(IndexT start) const
{
	const IndexT size = m_size;
	if (start < 0)
	{
		start = size + start;
		start = (start < 0) ? 0 : start;
	}
	else
	{
		start = (start > size) ? size : start;
	}
	return SubString(m_chars + start, size - start);
}

Bool SubString::isAscii() const
{
	NV_COMPILE_TIME_ASSERT(sizeof(Int8) == sizeof(Char));
	const Int8* chars = (const Int8*)(m_chars);
	const IndexT size = m_size;
	for (IndexT i = 0; i < size; i++)
	{
		if (chars[i] < 0)
		{
			return false;
		}
	}
	return true;
}

Bool SubString::equalsCstr(const Char* rhs) const
{
	const Char*const start = m_chars;
	const IndexT size = m_size;
	for (IndexT i = 0; i < size; i++)
	{
		const Char c = rhs[i];
		if (c != start[i] || c == 0)
		{
			return false;
		}
	}
	return rhs[size] == 0;
}

Bool SubString::equalsI(const ThisType& rhs) const
{
	if (&rhs == this)
	{
		return true;
	}
	if (m_size != rhs.m_size)
	{
		return false;
	}
	if (m_chars == rhs.m_chars)
	{
		return true;
	}
	for (IndexT i = 0; i < m_size; i++)
	{
		Char c = m_chars[i];
		Char rhsC = rhs.m_chars[i];
		// Make lower case
		c = (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
		rhsC = (rhsC >= 'A' && rhsC <= 'Z') ? (rhsC - 'A' + 'a') : rhsC;
		if (c != rhsC)
		{
			return false;
		}
	}
	return true;
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! String !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */


String::~String()
{
	if (m_allocator && m_capacity > 0)
	{
		m_allocator->deallocate(m_chars, m_capacity);
	}
}

/* static */IndexT String::calcInitialCapacity(IndexT size)
{
	return (size < 16) ? 16 : size;
}

/* static */IndexT String::calcCapacity(IndexT capacity, IndexT newCapacity)
{
	if (newCapacity < 16)
	{
		return 16;
	}
	// If the newCapacity more than doubles the old capacity -> assume it's enough
	if (newCapacity - capacity > (capacity >> 1))
	{
		return newCapacity;
	}
	if (newCapacity < 4096)
	{
		if (capacity * 2 > newCapacity)
		{
			return capacity * 2;
		}
	}
	// Assume it's going to get bigger
	return newCapacity + (newCapacity / 2);
}

void String::_ctor(const SubString& rhs)
{
	MemoryAllocator* allocator = MemoryAllocator::getInstance();
	if (rhs.m_size > 0)
	{
		// Need to make space
		IndexT capacity = calcInitialCapacity(rhs.m_size);
		m_chars = (Char*)allocator->allocate(capacity);
		m_capacity = capacity;
		m_size = rhs.m_size;

		Memory::copy(m_chars, rhs.m_chars, sizeof(Char) * rhs.m_size);
	}
	else
	{
		m_capacity = 0;
		m_size = 0;
		m_chars = NV_NULL;
	}
	m_allocator = allocator;
}


void String::_ctor(const SubString& rhs, MemoryAllocator* allocator)
{
	if (rhs.m_size > 0)
	{
		// Need to make space
		IndexT capacity = calcInitialCapacity(rhs.m_size);
		m_chars = (Char*)allocator->allocate(capacity);
		m_capacity = capacity;
		m_size = rhs.m_size;

		Memory::copy(m_chars, rhs.m_chars, sizeof(Char) * rhs.m_size);
	}
	else
	{
		m_capacity = 0;
		m_size = 0;
		m_chars = NV_NULL;
	}
	m_allocator = allocator;
}

Void String::swap(ThisType& rhs)
{
	Op::swap(m_chars, rhs.m_chars);
	Op::swap(m_size, rhs.m_size);
	Op::swap(m_capacity, rhs.m_capacity);
	Op::swap(m_allocator, rhs.m_allocator);
}

Void String::_concat(Char c)
{
	NV_ASSERT(m_size + 1 > m_capacity);
	NV_ASSERT(m_allocator);
	if (m_allocator)
	{
		IndexT newCapacity = calcCapacity(m_capacity, m_size + 1);
		m_chars = (Char*)m_allocator->reallocate(m_chars, m_capacity, m_size, newCapacity);
		m_capacity = newCapacity;
		m_chars[m_size++] = c;
	}
}

Void String::_concat(const SubString& rhs)
{
	// Only called if out of space
	NV_ASSERT(m_size + rhs.m_size > m_capacity);
	IndexT newCapacity = calcCapacity(m_capacity, m_size + rhs.m_size);
	Char* dstChars = (Char*)m_allocator->reallocate(m_chars, m_capacity, m_size, newCapacity);

	const Char* srcChars = containsMemory(rhs) ? (rhs.m_chars - m_chars + dstChars) : rhs.m_chars;
	
	Memory::copy(dstChars + m_size, srcChars, rhs.m_size * sizeof(Char));
	m_chars = dstChars;
	m_capacity = newCapacity;
	m_size += rhs.m_size;
}

Char* String::_getCstr()
{
	NV_ASSERT(m_size >= m_capacity);
	NV_ASSERT(m_allocator);
	if (m_allocator)
	{	
		const IndexT newCapacity = m_capacity + 1;
		m_chars = (Char*)m_allocator->reallocate(m_chars, m_capacity, m_size, newCapacity);
		m_capacity = newCapacity;
		m_chars[m_size] = 0;
		return m_chars;
	}
	return NV_NULL;
}

Void String::set(const SubString& rhs)
{
	if (this == &rhs ||
		rhs.m_chars == m_chars && rhs.m_size == m_size)
	{
		return;
	}
	// If it contains this substring, just move it
	if (containsMemory(rhs))
	{
		Memory::move(m_chars, rhs.m_chars, rhs.m_size * sizeof(Char));
		m_size = rhs.m_size;
		return;
	}
	// Check there is space to store new string
	if (m_capacity < rhs.m_size)
	{
		if (!m_allocator)
		{
			// Copy what we can and assert
			NV_ASSERT(false);
			Memory::copy(m_chars, rhs.m_chars, m_capacity * sizeof(Char));
			m_size = m_capacity;
			return;
		}
		const IndexT newCapacity = calcInitialCapacity(rhs.m_size);
		// Make sure we have space
		m_chars = (Char*)m_allocator->reallocate(m_chars, m_capacity, 0, newCapacity);
		m_capacity = newCapacity;
	}
	Memory::copy(m_chars, rhs.m_chars, rhs.m_size * sizeof(Char));
	m_size = rhs.m_size;
}

Void String::set(const SubString& rhs, MemoryAllocator* newAlloc)
{
	if (newAlloc == m_allocator)
	{
		return set(rhs);	
	}
	String work(rhs, newAlloc);
	swap(work);
}

Char* String::requireSpace(IndexT space)
{
	NV_ASSERT(space >= 0);
	const IndexT minCapacity = m_size + space;
	if (minCapacity > m_capacity)
	{
		NV_ASSERT(m_allocator);
		if (m_allocator)
		{
			const IndexT newCapacity = calcCapacity(m_capacity, minCapacity);
			m_chars = (Char*)m_allocator->reallocate(m_chars, m_capacity, m_size, newCapacity);
			m_capacity = newCapacity;	
		}
	}
	return m_chars + m_size;
}

} // namespace Nv

