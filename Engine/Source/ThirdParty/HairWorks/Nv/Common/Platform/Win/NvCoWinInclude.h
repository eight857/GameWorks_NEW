/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#ifndef NV_CO_WIN_INCLUDE_H
#define NV_CO_WIN_INCLUDE_H

#include <Nv/Common/NvCoCommon.h>

#ifndef _WIN32
	#error "This file should only be included by Windows builds!!"
#endif

// We only support >= Windows XP, and we need this for critical section and 
#ifndef _WIN32_WINNT
#	ifndef NV_WINMODERN
#		define _WIN32_WINNT 0x0500 
#	else
#		define _WIN32_WINNT 0x0602 
#	endif
#endif

#pragma warning (push)
#pragma warning (disable : 4668) //'symbol' is not defined as a preprocessor macro, replacing with '0' for 'directives'
#include <windows.h>
#pragma warning (pop)

#endif
