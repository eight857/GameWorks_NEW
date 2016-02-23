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

#ifndef NV_MEMORY_READ_STREAM_H
#define NV_MEMORY_READ_STREAM_H

#include <Nv/Foundation/NvStream.h>
#include <Nv/Foundation/NvMemory.h>

namespace Nv {

/*! Simple read implementation of ReadStream. 
NOTE! This implementation does not copy the memory passed to it, so for the stream to be functional 
it must be in scope for the same time as the stream */
class MemoryReadStream: public ReadStream
{
	NV_DECLARE_STACK_POLYMORPHIC_CLASS(MemoryReadStream, ReadStream)

	NV_INLINE virtual Int64 read(Void* buffer, Int64 numBytes) NV_OVERRIDE;
	NV_INLINE virtual Int64 seek(ESeekOrigin origin, Int64 changeInBytes) NV_OVERRIDE;
	NV_INLINE virtual Int64 tell() NV_OVERRIDE;
	NV_INLINE virtual Void close() NV_OVERRIDE;
	NV_INLINE virtual Bool isClosed() NV_OVERRIDE;

	/// Ctor with filename. Check if open was successful with isClosed() != false
	MemoryReadStream(const Void* data, SizeT size):m_data((const UInt8*)data), m_size(size), m_position(0) {}
	~MemoryReadStream() { close(); }

	SizeT m_position;
	SizeT m_size;
	const UInt8* m_data;	
};

Int64 MemoryReadStream::read(Void* buffer, Int64 numBytesIn)
{
	NV_ASSERT(numBytesIn >= 0);
	NV_ASSERT(m_position + numBytesIn < Int64(~SizeT(0)));
	SizeT newPos = m_position + SizeT(numBytesIn);
	newPos = (newPos > m_size) ? m_size : newPos;
	SizeT size = newPos - m_position;
	if (size > 0)
	{
		Memory::copy(buffer, m_data + m_position, SizeT(size));
	}
	m_position = newPos;
	return newPos;
}

Int64 MemoryReadStream::seek(ESeekOrigin origin, Int64 changeInBytes)
{
	Int64 newPos;
	// Work out new pos relative to the origin
	switch (origin)
	{
		default:
		case SeekOrigin::START:	newPos = changeInBytes; break;
		case SeekOrigin::END:	newPos = Int64(m_size) + changeInBytes; break;
		case SeekOrigin::CURRENT: newPos = Int64(m_position) + changeInBytes; break;
	}
	// Clamp new pos
	newPos = (newPos < 0) ? 0 : newPos;
	newPos = (newPos > Int64(m_size)) ? Int64(m_size) : newPos;
	// Set the new position
	m_position = SizeT(newPos);
	// Return absolute position
	return newPos;
}

Int64 MemoryReadStream::tell()
{	
	return Int64(m_position);
}

Void MemoryReadStream::close()
{
	m_data = NV_NULL;
	m_size = 0;
	m_position = 0;
}

Bool MemoryReadStream::isClosed()
{
	return m_data == NV_NULL;
}


} // namespace Nv

#endif // NV_MEMORY_READ_STREAM_H