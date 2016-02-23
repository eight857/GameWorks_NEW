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

#include <Nv/Foundation/NvCommon.h>

#include "NvWinMemoryMappedFile.h"

namespace Nv
{

Result WinMemoryMappedFile::init(const char* name, SizeT size)
{
#ifndef NV_WINMODERN
   	m_mapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name);
	if (m_mapFile == NV_NULL )
	{
		m_mapFile = CreateFileMappingA(
      				INVALID_HANDLE_VALUE,    // use paging file
       				NV_NULL,                    // default security
       				PAGE_READWRITE,          // read/write access
       				0,                       // maximum object size (high-order DWORD)
       				DWORD(size),                // maximum object size (low-order DWORD)
       				name);
	}
#else
	// convert name to unicode
	const static int BUFFER_SIZE = 256;
	WCHAR buffer[BUFFER_SIZE];
	int succ = MultiByteToWideChar(CP_ACP, 0, name, -1, buffer, BUFFER_SIZE);
	// validate
	if (succ < 0)
		succ = 0;
	if (succ < BUFFER_SIZE)
		buffer[succ] = 0;
	else if (buffer[BUFFER_SIZE - 1])
		buffer[0] = 0;

	m_mapFile = (succ > 0) ? CreateFileMappingFromApp(
		INVALID_HANDLE_VALUE,    // use paging file
       	NULL,                    // default security
       	PAGE_READWRITE,          // read/write access
       	ULONG64(mapSize),        // maximum object size (low-order DWORD)
       	buffer) 
		: NV_NULL;
#endif
	if (!m_mapFile)
	{
		return NV_FAIL;
	}
	m_baseAddress = MapViewOfFile(m_mapFile, FILE_MAP_ALL_ACCESS, 0, 0, size);
	if (!m_baseAddress)
	{
		CloseHandle(m_mapFile);
		m_mapFile = NV_NULL;
		return NV_FAIL;
	}
	return NV_OK;
}

WinMemoryMappedFile::~WinMemoryMappedFile()
{
	if (m_baseAddress )
   	{
   		UnmapViewOfFile(m_baseAddress);
		CloseHandle(m_mapFile);
	}
}

} // namespace Nv
