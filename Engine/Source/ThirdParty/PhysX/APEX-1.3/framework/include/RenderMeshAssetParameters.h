/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


// This file was generated by NxParameterized/scripts/GenParameterized.pl
// Created: 2015.06.02 04:11:41

#ifndef HEADER_RenderMeshAssetParameters_h
#define HEADER_RenderMeshAssetParameters_h

#include "NxParametersTypes.h"

#ifndef NX_PARAMETERIZED_ONLY_LAYOUTS
#include "NxParameterized.h"
#include "NxParameters.h"
#include "NxParameterizedTraits.h"
#include "NxTraitsInternal.h"
#endif

namespace physx
{
namespace apex
{

#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to __declspec(align())

namespace RenderMeshAssetParametersNS
{


struct REF_DynamicArray1D_Type
{
	NxParameterized::Interface** buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct STRING_DynamicArray1D_Type
{
	NxParameterized::DummyStringStruct* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct BOUNDS3_DynamicArray1D_Type
{
	physx::PxBounds3* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};


struct ParametersStruct
{

	REF_DynamicArray1D_Type submeshes;
	STRING_DynamicArray1D_Type materialNames;
	BOUNDS3_DynamicArray1D_Type partBounds;
	physx::PxU32 textureUVOrigin;
	physx::PxU32 boneCount;
	bool deleteStaticBuffersAfterUse;
	bool isReferenced;

};

static const physx::PxU32 checksum[] = { 0x119d6f62, 0x8d1ff03d, 0x19864d20, 0x93421fd0, };

} // namespace RenderMeshAssetParametersNS

#ifndef NX_PARAMETERIZED_ONLY_LAYOUTS
class RenderMeshAssetParameters : public NxParameterized::NxParameters, public RenderMeshAssetParametersNS::ParametersStruct
{
public:
	RenderMeshAssetParameters(NxParameterized::Traits* traits, void* buf = 0, PxI32* refCount = 0);

	virtual ~RenderMeshAssetParameters();

	virtual void destroy();

	static const char* staticClassName(void)
	{
		return("RenderMeshAssetParameters");
	}

	const char* className(void) const
	{
		return(staticClassName());
	}

	static const physx::PxU32 ClassVersion = ((physx::PxU32)0 << 16) + (physx::PxU32)0;

	static physx::PxU32 staticVersion(void)
	{
		return ClassVersion;
	}

	physx::PxU32 version(void) const
	{
		return(staticVersion());
	}

	static const physx::PxU32 ClassAlignment = 8;

	static const physx::PxU32* staticChecksum(physx::PxU32& bits)
	{
		bits = 8 * sizeof(RenderMeshAssetParametersNS::checksum);
		return RenderMeshAssetParametersNS::checksum;
	}

	static void freeParameterDefinitionTable(NxParameterized::Traits* traits);

	const physx::PxU32* checksum(physx::PxU32& bits) const
	{
		return staticChecksum(bits);
	}

	const RenderMeshAssetParametersNS::ParametersStruct& parameters(void) const
	{
		RenderMeshAssetParameters* tmpThis = const_cast<RenderMeshAssetParameters*>(this);
		return *(static_cast<RenderMeshAssetParametersNS::ParametersStruct*>(tmpThis));
	}

	RenderMeshAssetParametersNS::ParametersStruct& parameters(void)
	{
		return *(static_cast<RenderMeshAssetParametersNS::ParametersStruct*>(this));
	}

	virtual NxParameterized::ErrorType getParameterHandle(const char* long_name, NxParameterized::Handle& handle) const;
	virtual NxParameterized::ErrorType getParameterHandle(const char* long_name, NxParameterized::Handle& handle);

	void initDefaults(void);

protected:

	virtual const NxParameterized::DefinitionImpl* getParameterDefinitionTree(void);
	virtual const NxParameterized::DefinitionImpl* getParameterDefinitionTree(void) const;


	virtual void getVarPtr(const NxParameterized::Handle& handle, void*& ptr, size_t& offset) const;

private:

	void buildTree(void);
	void initDynamicArrays(void);
	void initStrings(void);
	void initReferences(void);
	void freeDynamicArrays(void);
	void freeStrings(void);
	void freeReferences(void);

	static bool mBuiltFlag;
	static NxParameterized::MutexType mBuiltFlagMutex;
};

class RenderMeshAssetParametersFactory : public NxParameterized::Factory
{
	static const char* const vptr;

public:
	virtual NxParameterized::Interface* create(NxParameterized::Traits* paramTraits)
	{
		// placement new on this class using mParameterizedTraits

		void* newPtr = paramTraits->alloc(sizeof(RenderMeshAssetParameters), RenderMeshAssetParameters::ClassAlignment);
		if (!NxParameterized::IsAligned(newPtr, RenderMeshAssetParameters::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class RenderMeshAssetParameters");
			paramTraits->free(newPtr);
			return 0;
		}

		memset(newPtr, 0, sizeof(RenderMeshAssetParameters)); // always initialize memory allocated to zero for default values
		return NX_PARAM_PLACEMENT_NEW(newPtr, RenderMeshAssetParameters)(paramTraits);
	}

	virtual NxParameterized::Interface* finish(NxParameterized::Traits* paramTraits, void* bufObj, void* bufStart, physx::PxI32* refCount)
	{
		if (!NxParameterized::IsAligned(bufObj, RenderMeshAssetParameters::ClassAlignment)
		        || !NxParameterized::IsAligned(bufStart, RenderMeshAssetParameters::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class RenderMeshAssetParameters");
			return 0;
		}

		// Init NxParameters-part
		// We used to call empty constructor of RenderMeshAssetParameters here
		// but it may call default constructors of members and spoil the data
		NX_PARAM_PLACEMENT_NEW(bufObj, NxParameterized::NxParameters)(paramTraits, bufStart, refCount);

		// Init vtable (everything else is already initialized)
		*(const char**)bufObj = vptr;

		return (RenderMeshAssetParameters*)bufObj;
	}

	virtual const char* getClassName()
	{
		return (RenderMeshAssetParameters::staticClassName());
	}

	virtual physx::PxU32 getVersion()
	{
		return (RenderMeshAssetParameters::staticVersion());
	}

	virtual physx::PxU32 getAlignment()
	{
		return (RenderMeshAssetParameters::ClassAlignment);
	}

	virtual const physx::PxU32* getChecksum(physx::PxU32& bits)
	{
		return (RenderMeshAssetParameters::staticChecksum(bits));
	}
};
#endif // NX_PARAMETERIZED_ONLY_LAYOUTS

} // namespace apex
} // namespace physx

#pragma warning(pop)

#endif
