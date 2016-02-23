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

#ifndef NV_TRAITS_H
#define NV_TRAITS_H


namespace Nv {
// Predeclare the ComPtr type
template <class T>
class ComPtr;
template <class T>
class WeakComPtr;

namespace Traits {

// Helper to strip const from a type
template <typename T>
struct StripConst
{
	typedef T Value;
};
template <typename T>
struct StripConst<const T>
{
	typedef T Value;
};

/* Find the underlying type of a pointer taking into account smart pointers*/
namespace GetPointerTypeImpl
{
template <typename T>
struct Strip;

template <typename T>
struct Strip<T*>
{
	typedef T Value;
};
template <typename T>
struct Strip<const T*>
{
	typedef T Value;
};
template <typename T>
struct Strip<ComPtr<T> >
{
	typedef typename Strip<T*>::Value Value;
};
template <typename T>
struct Strip<WeakComPtr<T> >
{
	typedef typename Strip<T*>::Value Value;
};
} // namespace PointerTypeImpl

// To get the underlying thing a pointer points to 
template <typename T>
struct GetPointerType
{
	typedef typename GetPointerTypeImpl::Strip<T>::Value Value;
};

template <typename T>
struct GetPointerType<const T>
{
	typedef typename GetPointerTypeImpl::Strip<T>::Value Value;
};

} // namespace Traits
} // namespace Nv


#endif // NV_TRAITS_H
