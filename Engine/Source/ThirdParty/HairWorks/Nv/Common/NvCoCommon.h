/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#ifndef NV_CO_COMMON_H
#define NV_CO_COMMON_H

// check that exactly one of NDEBUG and _DEBUG is defined
#if !defined(NDEBUG) ^ defined(_DEBUG)
#	error Exactly one of NDEBUG and _DEBUG needs to be defined!
#endif

#ifndef NV_DEBUG
#	ifdef NDEBUG
#		define NV_DEBUG 0
#	else
#		define NV_DEBUG 1
#	endif
#endif // NV_DEBUG

// make sure NV_CHECKED is defined in all _DEBUG configurations as well
#if !defined(NV_CHECKED) && defined _DEBUG
#	define NV_CHECKED 1
#endif

#include <Nv/Core/1.0/NvDefines.h>
#include <Nv/Core/1.0/NvTypes.h>
#include <Nv/Core/1.0/NvAssert.h>
#include <Nv/Core/1.0/NvResult.h>

/** \addtogroup common
@{
*/

namespace nvidia { 
namespace Common { 

// Op namespace is for simple/small global operations. Namespace is used to make distinct from common method names.
namespace Op {
/// Swap two items through a temporary. 
template<class T>
NV_CUDA_CALLABLE NV_FORCE_INLINE Void swap(T& x, T& y)
{
	const T tmp = x;
	x = y;
	y = tmp;
}
} // namespace Op

} // namespace Common
} // namespace nvidia

namespace NvCo = nvidia::Common;

/** @} */

#endif // NV_CO_COMMON_H
