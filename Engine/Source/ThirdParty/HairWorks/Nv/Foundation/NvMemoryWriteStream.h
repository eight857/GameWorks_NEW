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

#ifndef NV_MEMORY_WRITE_STREAM_H
#define NV_MEMORY_WRITE_STREAM_H

#include <Nv/Foundation/NvStream.h>
#include <Nv/Foundation/NvMemory.h>

namespace Nv {

/*! Simple memory implementation of WriteStream. */
class MemoryWriteStream: public WriteStream
{
	NV_DECLARE_STACK_POLYMORPHIC_CLASS(MemoryWriteStream, WriteStream)

	NV_INLINE virtual Int64 write(const Void* data, Int64 numBytes) NV_OVERRIDE;
	NV_INLINE virtual Void flush() NV_OVERRIDE;
	NV_INLINE virtual Void close() NV_OVERRIDE;
	NV_INLINE virtual Bool isClosed() NV_OVERRIDE;

		/// Get the data
	NV_FORCE_INLINE const UInt8* getData() const { return m_data; }
		/// Get the size
	NV_FORCE_INLINE SizeT getSize() const { return m_position; }
		
	/// Ctor with filename. Check if open was successful with isClosed() != false
	NV_INLINE MemoryWriteStream(Void* data, SizeT capacity, MemoryAllocator* allocator);
	NV_INLINE MemoryWriteStream(MemoryAllocator* allocator) : m_data(NV_NULL), m_capacity(0), m_position(0), m_allocator(allocator), m_isClosed(false) {}
	NV_INLINE ~MemoryWriteStream();

	protected:
	Bool m_isClosed;
	SizeT m_position;					///< Position of write head (ie total size of contents)
	SizeT m_capacity;					///< Total available capacity
	MemoryAllocator* m_allocator;		///< If null, all memory must be allocated outside 
	UInt8* m_data;				///< Buffer which is m_capacity long
};


MemoryWriteStream::MemoryWriteStream(Void* data, SizeT capacity, MemoryAllocator* allocator) :
	m_data((UInt8*)data), 
	m_capacity(capacity), 
	m_position(0), 
	m_allocator(allocator),
	m_isClosed(false)
{
}

MemoryWriteStream::~MemoryWriteStream()
{
	if (m_allocator && m_data)
	{
		m_allocator->deallocate(m_data, m_capacity);
	}
}

Int64 MemoryWriteStream::write(const Void* buffer, Int64 numBytesIn)
{
	NV_ASSERT(numBytesIn >= 0);
	if (m_isClosed)
	{
		return 0;
	}
	NV_ASSERT(numBytesIn < ~SizeT(0));

	SizeT numBytes = SizeT(numBytesIn);

	if (m_position + numBytes > m_capacity)
	{
		if (m_allocator)
		{
			const SizeT minExpandSize = 4096;

			// Expand size is always greatest of capacity/2, minExpandSize, and the numBytes 
			SizeT expandSize = (m_capacity / 2);
			expandSize = (expandSize < numBytes) ? numBytes : expandSize;
			expandSize = (expandSize < minExpandSize) ? minExpandSize : expandSize;

			m_data = (UInt8*)m_allocator->reallocate(m_data, m_capacity, m_position, m_capacity + expandSize);			
			if (!m_data)
			{
				numBytes = m_capacity - m_position;
			}
			else
			{
				m_capacity = m_capacity + expandSize;
			}
		}
		else
		{
			// We can only set on the buffer we have
			numBytes = m_capacity - m_position;
		}
	}

	if (numBytes > 0)
	{
		Memory::copy(m_data + m_position, buffer, numBytes);
		m_position += numBytes;
	}
	return numBytes;
}

Void MemoryWriteStream::flush()
{
}

Void MemoryWriteStream::close()
{
	m_isClosed = true;
}

Bool MemoryWriteStream::isClosed()
{
	return m_isClosed;
}

} // namespace Nv

#endif // NV_MEMORY_WRITE_STREAM_H