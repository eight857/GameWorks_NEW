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

#ifndef NV_STREAM_H
#define NV_STREAM_H

#include <Nv/Foundation/NvCommon.h>
#include <Nv/Foundation/NvTypeMacros.h>

namespace Nv {

/*! The seek type options */
class SeekOrigin { SeekOrigin(); public: enum Enum
{
	CURRENT,		///< Relative to the current position
	START,			///< Relative to the start 
	END,			///< Relative to the end
}; };
typedef SeekOrigin::Enum ESeekOrigin;

/// Read stream interface
class ReadStream
{
	NV_DECLARE_INTERFACE_BASE(ReadStream)

		/*! Reads the number of bytes specified into buffer. Returns the amount of bytes read */
	virtual Int64 read(Void* buffer, Int64 numBytes) = 0;
		/*! Seeks the read position relative to the origin by changeInBytes. Returns the current read position (ie same as tell). */
	virtual Int64 seek(ESeekOrigin origin, Int64 changeInBytes) = 0;
		/*! Returns the current read position */
	virtual Int64 tell() = 0;
		/*! Closes the stream if open */
	virtual Void close() = 0; 
		/*! Returns true if the stream is closed */
	virtual Bool isClosed() = 0;

	virtual ~ReadStream() {}
};

/// Write stream interface
class WriteStream
{
	NV_DECLARE_INTERFACE_BASE(WriteStream)

		/*! Writes data to the stream. Returns the number of bytes actually written */
	virtual Int64 write(const Void* data, Int64 numBytes) = 0;
		/*! Flushes any non written data */
	virtual Void flush() = 0;
		/*! Closes the stream if open */
	virtual Void close() = 0;
		/*! Returns true if the stream is closed */
	virtual Bool isClosed() = 0;

	virtual ~WriteStream() {}
};

} // namespace Nv

#endif // NV_LOGGER_H