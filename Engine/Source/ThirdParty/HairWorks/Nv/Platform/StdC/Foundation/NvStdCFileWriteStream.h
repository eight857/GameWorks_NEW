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

#ifndef NV_STDC_FILE_WRITE_STREAM_H
#define NV_STDC_FILE_WRITE_STREAM_H

#include <stdio.h>

#include <Nv/Foundation/NvStream.h>

namespace Nv {

/*! An implementation of WriteStream that works with StdC file types. NOTE! It is designed 
to only be allocated on the stack. This is so there is no dependency on a memory allocator, 
if you want to to allocate it on the heap, derive from it, and set a suitable operator new/operator delete
*/
class StdCFileWriteStream : public WriteStream
{
	NV_DECLARE_STACK_POLYMORPHIC_CLASS(StdCFileWriteStream, WriteStream)

	NV_INLINE virtual Int64 write(const Void* data, Int64 numBytes) NV_OVERRIDE;
	NV_INLINE virtual Void flush() NV_OVERRIDE;
	NV_INLINE virtual Void close() NV_OVERRIDE;
	NV_INLINE virtual Bool isClosed() NV_OVERRIDE;

	StdCFileWriteStream(const char* filename);
	StdCFileWriteStream(FILE* file) :m_file(file) {}
	~StdCFileWriteStream() { close(); }

	FILE* m_file;		///< if it's null means it has been closed. 
};

StdCFileWriteStream::StdCFileWriteStream(const char* filename)
{
	m_file = ::fopen(filename, "wb");
}

Int64 StdCFileWriteStream::write(const Void* buffer, Int64 numBytes)
{
	if (m_file)
	{
		return ::fwrite(buffer, 1, size_t(numBytes), m_file);
	}
	return 0;
}

Void StdCFileWriteStream::flush()
{
	if (m_file)
	{
		::fflush(m_file);
	}
}

Void StdCFileWriteStream::close()
{
	if (m_file)
	{
		::fclose(m_file);
		m_file = NV_NULL;
	}
}

Bool StdCFileWriteStream::isClosed()
{
	return m_file == NV_NULL;
}

} // namespace Nv

#endif // NV_STDC_FILE_WRITE_STREAM_H