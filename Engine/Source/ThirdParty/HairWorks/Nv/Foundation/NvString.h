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

#ifndef NV_STRING_H
#define NV_STRING_H

#include <Nv/Foundation/NvCommon.h>
#include <Nv/Foundation/NvTypeMacros.h>
#include <Nv/Foundation/NvMemory.h>

namespace Nv {

class String;
/*! A subpart of a string. In and of itself it has no memory management.
String is encoded in utf8. Note that a SubString is not designed to be zero terminated, allowing for substring to be taken without 
memory management */
class SubString
{
	friend class String;
	NV_DECLARE_CLASS_BASE(SubString);
	
		/// Type to allow initializing from a C style string (ie zero terminated). 
	enum InitCstr { INIT_CSTR };

		/// True if the memory of rhs is inside of this
	NV_FORCE_INLINE Bool containsMemory(const SubString& rhs) const { return rhs.m_chars >= m_chars && rhs.m_chars + rhs.m_size <= m_chars + m_size; } 

		/// For iterating over chars
	NV_FORCE_INLINE const Char* begin() const { return m_chars; }
		/// For iterating over chars
	NV_FORCE_INLINE const Char* end() const { return m_chars + m_size; }

		/// Get the size
	NV_FORCE_INLINE IndexT getSize() const { return m_size; }

		/// Returns first occurrence of character c, or -1 if not found
	IndexT indexOf(Char c) const;
	IndexT indexOf(Char c, IndexT from) const;

		/// Looks for first instance of c starting from the end. If not found returns -1.
	IndexT reverseIndexOf(Char c) const;

		/// Get a substring, from start up until (but not including end). Negative indices wrap around
	SubString subStringWithEnd(IndexT start, IndexT end) const;

		/// Get a substring from start, including size characters. Start can be negative and if so wraps around.
	SubString subStringWithStart(IndexT start, IndexT subSize) const;
		/// Takes the head number of characters, or size whichever is less. Can use negative numbers to wrap around.
	SubString head(IndexT end) const;
		/// Takes the characters from start until the end. Can use negative numbers to wrap around.
	SubString tail(IndexT start) const;

		/// Substrings contain utf8. Returns true if the contents is all ascii encoded.
	Bool isAscii() const; 

		/// True if exactly equal
	Bool equals(const ThisType& rhs) const;
		/// True if equal case insensitive
	Bool equalsI(const ThisType& rhs) const;

		/// True if equals a c string (ie zero terminated)
	Bool equalsCstr(const Char* rhs) const;

		/// ==
	NV_FORCE_INLINE Bool operator==(const ThisType& rhs) const { return equals(rhs); }
		/// !=
	NV_FORCE_INLINE Bool operator!=(const ThisType& rhs) const { return !equals(rhs); }

		/// Construct from a zero terminated C style string
	SubString(InitCstr, const Char* in);
		/// Ctor with size
	NV_FORCE_INLINE SubString(const Char* chars, SizeT size):m_chars(const_cast<Char*>(chars)), m_size(IndexT(size)) {}

		/// String literal constructor (typically used with string literals)
	template <SizeT SIZE>
	NV_FORCE_INLINE SubString(const Char (&in)[SIZE]):m_chars(const_cast<Char*>(in)), m_size(IndexT(SIZE)) {}

		/// Get the empty substring
	static const SubString& getEmpty() { return s_empty; }

	protected:
	Char* m_chars;	///< Start of the string (NOTE! not const, because String derives from it)
	IndexT m_size;	///< Size

	NV_FORCE_INLINE SubString() {}

	private:

	static const SubString s_empty; 

	// Disabled! So not eroneously initialized with say a Char buffer[80]; type declaration 
	// If a buffer is to be used to store the characters and is zero terminated use SubString sub(SubString::INIT_CSTR, buffer);
	// If the length is known, just use SubString sub(buffer, size);
	template <SizeT SIZE> 
	NV_FORCE_INLINE SubString(Char* in);
};

/*! Normally stored non zero terminated. Use getZeroTerminated to get a zero terminated string, for use in typical C style string functions.
The type derives from SubString, such that can be easily used as a SubString that is mutable and manages the characters storage. */
class String: public SubString
{
	NV_DECLARE_CLASS(String, SubString);

		/// Immutable iteration
	NV_FORCE_INLINE const Char* begin() const { return m_chars; }
	NV_FORCE_INLINE const Char* end() const { return m_chars + m_size; }

		/// Mutable iteration 
	NV_FORCE_INLINE Char* begin() { return m_chars; }
	NV_FORCE_INLINE Char* end() { return m_chars + m_size; } 

		/// ==
		/// NOTE Equality is only on contents not on the allocator used
	NV_FORCE_INLINE Bool operator==(const ThisType& rhs) const { return equals(rhs); } 
		/// !=
		/// NOTE Inequality is only on contents not on the allocator used
	NV_FORCE_INLINE Bool operator!=(const ThisType& rhs) const { return !equals(rhs); }

		/// Concat a substring
	NV_FORCE_INLINE ThisType& concat(const SubString& rhs);
		/// Concat a char
	NV_FORCE_INLINE ThisType& concat(Char c);

		/// Reduce the size 
	NV_FORCE_INLINE Void reduceSize(IndexT size);
		/// Change the size by the specified delta. 
	NV_FORCE_INLINE Void changeSize(IndexT delta);
		/// Set to a specific size (must be in range 0 - capacity)
	NV_FORCE_INLINE Void setSize(IndexT size);

		/// Makes sure there is space at the end of the size specified
	Char* requireSpace(IndexT space);
	
		/// Get as a C style string (ie zero terminated)
	NV_FORCE_INLINE Char* getCstr();
	const Char* getCstr() const { return const_cast<ThisType*>(this)->getCstr(); } 

		/// Swap
	Void swap(ThisType& rhs);

		/// Set contents
	Void set(const SubString& rhs);
		/// Set using the specific alloc
	Void set(const SubString& rhs, MemoryAllocator* alloc);

		/// Get the set memory allocator
	NV_FORCE_INLINE MemoryAllocator* getAllocator() const { return m_allocator; }
		/// The total capacity for storing chars. 
	NV_FORCE_INLINE IndexT getCapacity() const { return m_capacity; }

		/// Assign
	ThisType& operator=(const SubString& rhs) { set(rhs); return *this; }

		/// Ctor
	NV_FORCE_INLINE String(const SubString& rhs) { _ctor(rhs); }
	NV_FORCE_INLINE String(const SubString& rhs, MemoryAllocator* alloc) { _ctor(rhs, alloc); }

	NV_FORCE_INLINE String(const ThisType& rhs) { _ctor(rhs); }
	NV_FORCE_INLINE String(const ThisType& rhs, MemoryAllocator* alloc) { _ctor(rhs, alloc); }

	NV_FORCE_INLINE String(Char* chars, IndexT size, IndexT capacity, MemoryAllocator* alloc = NV_NULL);
		/// Default Ctor
	NV_FORCE_INLINE String():Parent(NV_NULL, 0), m_capacity(0), m_allocator(MemoryAllocator::getInstance()) {}

		/// Dtor
	~String();

#ifdef NV_HAS_MOVE_SEMANTICS
		/// Move Ctor
	NV_FORCE_INLINE String(ThisType&& rhs):Parent(rhs), m_capacity(rhs.m_capacity), m_allocator(rhs.m_allocator) { rhs.m_allocator = NV_NULL; }
		/// Move assign
	NV_FORCE_INLINE String& operator=(ThisType&& rhs) { swap(rhs); return *this; }
#endif

		/// Get the empty string
	static const String& getEmpty() { return s_empty; }
		/// Works out the appropriate initial capacity
	static IndexT calcInitialCapacity(IndexT size);
	static IndexT calcCapacity(IndexT oldCapacity, IndexT newCapacity);

	protected:

	Void _ctor(const SubString& rhs);
	Void _ctor(const SubString& rhs, MemoryAllocator* alloc);

	Void _concat(Char c);
	Void _concat(const SubString& rhs);
	Char* _getCstr();

	IndexT m_capacity;					///< Total capacity
	MemoryAllocator* m_allocator;		///< Allocator that backs memory (if NV_NULL memory is not freed)

	private:
	static const String s_empty;
};

// ---------------------------------------------------------------------------
NV_FORCE_INLINE String& String::concat(Char c)
{
	if (m_size < m_capacity)
	{
		m_chars[m_size++] = c;
	}
	else
	{
		_concat(c);
	}
	return *this;
}
// ---------------------------------------------------------------------------
NV_FORCE_INLINE String& String::concat(const SubString& rhs)
{
	if (m_size + rhs.m_size <= m_capacity)
	{
		if (rhs.m_size > 0)
		{
			Memory::copy(m_chars + m_size, rhs.m_chars, rhs.m_size * sizeof(Char));
		}
		m_size += rhs.m_size;
	}
	else
	{
		_concat(rhs);
	}
	return *this; 
}
// ---------------------------------------------------------------------------
NV_FORCE_INLINE Char* String::getCstr() 
{
	if (m_size < m_capacity)
	{
		m_chars[m_size] = 0;
		return m_chars;
	}
	else
	{
		return _getCstr();
	}
}
// ---------------------------------------------------------------------------
NV_FORCE_INLINE Void String::reduceSize(IndexT size)
{
	NV_ASSERT(size >= 0 && size <= m_size);
	m_size = size;
}
// ---------------------------------------------------------------------------
Void String::changeSize(IndexT delta)
{
	IndexT newSize = m_size + delta;
	NV_ASSERT(newSize >= 0 && newSize <= m_capacity);
	m_size = newSize;
}
// ---------------------------------------------------------------------------
Void String::setSize(IndexT size)
{
	NV_ASSERT(size >= 0 && size <= m_capacity);
	m_size = size;
}
// ---------------------------------------------------------------------------
String::String(Char* chars, IndexT size, IndexT capacity, MemoryAllocator* alloc):
	Parent(chars, size),
	m_capacity(capacity),
	m_allocator(alloc)
{
}

} // namespace Nv

#endif // NV_STRING_H