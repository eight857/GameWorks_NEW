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

#ifndef NV_DX12_HANDLE_H
#define NV_DX12_HANDLE_H

#include <Nv/Foundation/NvApiHandle.h>

// Dx12 types
#include <d3d12.h>
//#include <wrl.h>

namespace Nv {

class Dx12SubType { Dx12SubType(); public:
/*! SubTypes for Dx12 */
enum Enum
{
	UNKNOWN,	///< Unknown
	CONTEXT,	
	DEVICE,
	BUFFER,
	FLOAT32,
	SHADER_RESOURCE_VIEW,
	DEPTH_STENCIL_VIEW,
	COUNT_OF,
};
}; 

// Handle types 
#define NV_DX12_HANDLE_TYPES(x)	\
	x(ID3D12Device, DEVICE, DEVICE) \
	x(ID3D12GraphicsCommandList, CONTEXT, CONTEXT) \
	x(ID3D12Resource, BUFFER, BUFFER) 

// The 'value' types - ie ones that can be passed as pointers, in addition to the handle types
#define NV_DX12_VALUE_TYPES(x) \
	x(Float32, FLOAT32, UNKNOWN) 

struct Dx12Type
{
	// Used by the macros. NOTE! Should be wrapping type (ie not the actual type - typically with E prefix)
	typedef Dx12SubType ScopeSubType;

		/// Get the full type for the subtype
	NV_FORCE_INLINE static Int getType(Dx12SubType::Enum subType) { return (Int(ApiType::DX12) << 8) | Int(subType); }
		/// Get the type via template, needed for arrays
	template <typename T>
	NV_FORCE_INLINE static Int getType() { return getType((T*)NV_NULL); }

		/// Implement getType	
	NV_DX12_HANDLE_TYPES(NV_HW_GET_TYPE)
		/// Implement getHandle, which will return a TypedApiHandle 
	NV_DX12_HANDLE_TYPES(NV_HW_GET_HANDLE)
		/// Implement getType for 'value types' (ie structs and others that shouldn't be in a handle)
	NV_DX12_VALUE_TYPES(NV_HW_GET_VALUE_TYPE)

		/// A template to work around warnings from dereferencing NV_NULL
	template <typename T>
	NV_FORCE_INLINE static Int getPtrType() { Void* data = NV_NULL; return getType(*(const T*)&data); }

		/// Get a pointer
	template <typename T>
	NV_FORCE_INLINE static ConstApiPtr getPtr(const T* in) { return ConstApiPtr(getPtrType<T>(), in); }
	template <typename T>
	NV_FORCE_INLINE static ApiPtr getPtr(T* in) { return ApiPtr(getPtrType<T>(), in); }

		/// Get from a handle
	template <typename T>
	NV_FORCE_INLINE static T* get(const ApiHandle& in) { const Int type = getType((T*)NV_NULL); return reinterpret_cast<T*>((type == in.m_type) ? in.m_handle : handleCast(in.m_type, type)); }

		/// Get from 
	template <typename T>
	NV_FORCE_INLINE static const T* get(const ConstApiPtr& ptr) { const Int type = getPtrType<T>(); return reinterpret_cast<const T*>((ptr.m_type == type) ? ptr.getData() : handlePtrCast(ptr.m_type, type)); }
		// Get from 
	template <typename T>
	NV_FORCE_INLINE static T* get(const ApiPtr& ptr) { const Int type = getPtrType<T>(); return reinterpret_cast<T*>((ptr.m_type == type) ? ptr.getData() : handlePtrCast(ptr.m_type, type)); }

		/// Get the sub type as text
	static const Char* getSubTypeText(Dx12SubType::Enum subType);

		/// If the match fails - implement casting. Writes impossible casts to Logger and returns NV_NULL.
	static Void* handlePtrCast(Int fromType, Int toType);
	static Void* handleCast(Int fromType, Int toType);
		/// Called when a cast isn't possible. Will output warnings to log and return NV_NULL
	static Void castFailure(Int fromType, Int toType);
		/// Log failure
	static Void logCastFailure(Int fromType, Int toType);
};

/* For generic handles you can use Dx11Handle. If you want the typed handle type use Dx11Type::getHandle(texture) */
typedef WrapApiHandle<Dx12Type> Dx12Handle;

} // namespace Nv

#endif // NV_DX12_HANDLE_H
