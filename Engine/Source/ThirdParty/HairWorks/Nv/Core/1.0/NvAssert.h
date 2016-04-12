/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#ifndef NV_CORE_ASSERT_H
#define NV_CORE_ASSERT_H

#ifndef NV_ENABLE_ASSERTS
#	define NV_ENABLE_ASSERTS (NV_DEBUG && !defined(__CUDACC__)) 
#endif

#if NV_ENABLE_ASSERTS
#	include <assert.h>
#endif

/** \addtogroup core
@{
*/

/* Assert macros */
#ifndef NV_CORE_ASSERT
#	if NV_ENABLE_ASSERTS
#		define NV_CORE_ASSERT(exp) (assert(exp))
#		define NV_CORE_ALWAYS_ASSERT(x) NV_CORE_ASSERT(0)
#	else
#		define NV_CORE_ASSERT(exp) 
#		define NV_CORE_ALWAYS_ASSERT(x) 
#	endif
#endif

/** @} */
#endif // #ifndef NV_CORE_ASSERT_H
