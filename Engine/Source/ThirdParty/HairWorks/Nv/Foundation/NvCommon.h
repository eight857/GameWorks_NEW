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

#ifndef NV_COMMON_H
#define NV_COMMON_H

// Disable exception signatures (happens on placement new)  
#ifdef NV_DISABLE_EXCEPTIONS
#	if !defined(_HAS_EXCEPTIONS) && defined(_MSC_VER)
#		define _HAS_EXCEPTIONS 0
#	endif
#endif

#include <cstddef>
#include <stdint.h> // cstdint is in tr1 folder on current XCode
#include <string.h>	// for memory methods like memcpy 
#include <assert.h>
#include <new>		// For placement new


/* macro helpers */
#define NV_CONCATENATE_HELPER(X, Y) X ## Y
#define NV_CONCATENATE(X, Y) NV_CONCATENATE_HELPER(X, Y)

#define NV_STRINGIFY_HELPER(X) #X
#define NV_STRINGIFY(X) NV_STRINGIGY_HELPER(X)

/**
Operating system defines, see http://sourceforge.net/p/predef/wiki/OperatingSystems/
*/

#ifndef NV_PLATFORM
#define NV_PLATFORM

#if defined(WINAPI_FAMILY) && WINAPI_FAMILY == WINAPI_PARTITION_APP
#	define NV_WINRT 1 // Windows Runtime, either on Windows RT or Windows 8
#elif defined(XBOXONE)
#	define NV_XBOXONE 1
#elif defined(_WIN64) // note: XBOXONE implies _WIN64
#	define NV_WIN64 1
#elif defined(_M_PPC)
#	define NV_X360 1
#elif defined(_WIN32) // note: _M_PPC implies _WIN32
#	define NV_WIN32 1
#elif defined(__ANDROID__)
#	define NV_ANDROID 1
#elif defined(__linux__) // note: __ANDROID__ implies __linux__
#	define NV_LINUX 1
#elif defined(__APPLE__) && (defined(__arm__) || defined(__arm64__))
#	define NV_IOS 1
#elif defined(__APPLE__)
#	define NV_OSX 1
#elif defined(__CELLOS_LV2__)
#	define NV_PS3 1
#elif defined(__ORBIS__)
#	define NV_PS4 1
#elif defined(__SNC__) && defined(__arm__)
#	define NV_PSP2 1
#elif defined(__ghs__)
#	define NV_WIIU 1
#else
#	error "Unknown operating system"
#endif

/**
family shortcuts - may be more than one set. Test with #ifdef or #if defined(...)
*/
#if defined(NV_WINRT) || defined(NV_WIN32) || defined(NV_WIN64)
#	define NV_WINDOWS_FAMILY 1
// To be compatible with depreciated NvFoundation.h
#	define NV_WINDOWS
#endif
#if defined(NV_XBOXONE) || defined(NV_X360) || defined(NV_WINDOWS_FAMILY)
#	define NV_MICROSOFT_FAMILY 1
#endif
#if defined(NV_LINUX) || defined(NV_ANDROID)
#	define NV_LINUX_FAMILY 1
#endif
#if defined(NV_IOS) || defined(NV_OSX)
#	define NV_APPLE_FAMILY 1
#endif
#if defined(NV_LINUX_FAMILY) || defined(NV_APPLE_FAMILY)
#	define NV_UNIX_FAMILY 1
#endif

#endif // NV_PLATFORM

/* compiler */
#ifndef NV_COMPILER
#define NV_COMPILER

#if defined(__clang__)
#	define NV_CLANG 1
#elif defined(__GNUC__) // note: __clang__ implies __GNUC__
#	define NV_GCC 1
#elif defined(_MSC_VER)
#	define NV_VC (_MSC_VER / 100 - 6)
#else
#	error "Unknown compiler"
#endif

/**
family shortcuts
*/
#if defined(NV_CLANG) || defined(NV_GCC)
#	define NV_GCC_FAMILY 1
#endif

#endif // NV_COMPILER

/* processor */
#ifndef NV_PROCESSOR

// Known processor types
#define NV_PROCESSOR_UNKNOWN	0
#define NV_PROCESSOR_X64		1
#define NV_PROCESSOR_X86		2
#define NV_PROCESSOR_ARM		3
#define NV_PROCESSOR_ARM64		4
#define NV_PROCESSOR_PPC		5
#define NV_PROCESSOR_SPU		6

/// The variable NV_PROCESSOR is set to one of the processor options listed above
/// To test for a processor #if NV_PROCESSOR == NV_PROCESSOR_X86 

/* Work out the processor type */
#if defined(_M_ARM) || defined(__ARM_EABI__)
#	define NV_PROCESSOR NV_PROCESSOR_ARM
// Assume its arm 7 form now
#	define NV_PROCESSOR_ARCH_ARM_V7 1
#elif defined(__SPU__)
#	define NV_PROCESSOR NV_PROCESSOR_SPU
#elif defined(__i386__) || defined(_M_IX86)
#	define NV_PROCESSOR NV_PROCESSOR_X86
#elif defined(_M_AMD64) || defined(_M_X64) || defined(__amd64) || defined(__x86_64)
#	define NV_PROCESSOR NV_PROCESSOR_X64 
#elif defined(_PPC_) || defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC)
#	define NV_PROCESSOR NV_PROCESSOR_PPC
#elif defined(__arm__)
// https://wiki.edubuntu.org/ARM/Thumb2PortingHowto
// __ARM_ARCH_4T__ - 
#	define NV_PROCESSOR NV_PROCESSOR_ARM

// http://www.shervinemami.info/armAssembly.html#template

#   if defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7R__) || defined (__ARM_ARCH_7M__)
#       define NV_PROCESSOR_ARCH_ARM_V7 1
#   elif defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_6T2__) || defined(NN_PROCESSOR_ARM) || defined(NN_PROCESSOR_ARM_V6) 
#       define NV_PROCESSOR_ARCH_ARM_V6 1
#	else 
//#		error Unknown ARM
#       define NV_PROCESSOR_ARCH_ARM_UNKNOWN 1
#   endif
#elif defined(__aarch64__)
#   define NV_PROCESSOR NV_PROCESSOR_ARM64
#endif

#ifndef NV_PROCESSOR
#	error "Unable to detect processor type"
#endif

// Processor families
#define NV_PROCESSOR_FAMILY_UNKNOWN 0
#define NV_PROCESSOR_FAMILY_X86		1			
#define NV_PROCESSOR_FAMILY_ARM		2
#define NV_PROCESSOR_FAMILY_PPC		3

#if (NV_PROCESSOR == NV_PROCESSOR_X64) || (NV_PROCESSOR == NV_PROCESSOR_X86)
#	define NV_PROCESSOR_FAMILY NV_PROCESSOR_FAMILY_X86
#elif NV_PROCESSOR == NV_PROCESSOR_ARM || NV_PROCESSOR == NV_PROCESSOR_ARM64
#	define NV_PROCESSOR_FAMILY NV_PROCESSOR_FAMILY_ARM
#elif NV_PROCESSOR == NV_PROCESSOR_PPC
#	define NV_PROCESSOR_FAMILY NV_PROCESSOR_FAMILY_PPC
#else
#	define NV_PROCESSOR_FAMILY NV_PROCESSOR_FAMILY_UNKNOWN
#endif

#if NV_PROCESSOR == NV_PROCESSOR_X86
#	define NV_INT_IS_INT32
#	define NV_FLOAT_IS_FLOAT32
#	define NV_PTR_IS_32
#	define NV_LITTLE_ENDIAN
#	define NV_HAS_UNALIGNED_ACCESS
#elif NV_PROCESSOR == NV_PROCESSOR_X64
#	define NV_INT_IS_INT32
#	define NV_FLOAT_IS_FLOAT32
#	define NV_PTR_IS_64
#	define NV_LITTLE_ENDIAN
#	define NV_HAS_UNALIGNED_ACCESS
#elif NV_PROCESSOR == NV_PROCESSOR_PPC
#	define NV_INT_IS_INT32
#	define NV_FLOAT_IS_FLOAT32
#	define NV_PTR_IS_32
#	define NV_BIG_ENDIAN
#elif NV_PROCESSOR == NV_PROCESSOR_ARM
#	define NV_INT_IS_INT32
#	define NV_FLOAT_IS_FLOAT32
#	define NV_PTR_IS_32
#   if defined(__ARMEB__)
#		define NV_BIG_ENDIAN
#   else 
#		define NV_LITTLE_ENDIAN
#   endif
#elif NV_PROCESSOR == NV_PROCESSOR_ARM64
#	define NV_INT_IS_INT32
#	define NV_FLOAT_IS_FLOAT32
#	define NV_PTR_IS_64
#   if defined(__ARMEB__)
#		define NV_BIG_ENDIAN
#   else
#		define NV_LITTLE_ENDIAN
#   endif
#else
#	error "Unknown processor type"
#endif

#endif // NV_PROCESSOR

/* align, inline, linkage macros */

#ifndef NV_MACROS
#define NV_MACROS

// Gcc
#ifdef NV_GCC_FAMILY
#	define NV_ALIGN_PREFIX(bytes)
#	define NV_ALIGN_SUFFIX(bytes) __attribute__((aligned (bytes)))

#	define NV_PUSH_PACK_DEFAULT _Pragma("pack(push, 8)")
#	define NV_POP_PACK _Pragma("pack(pop)")

#	define NV_FORCE_INLINE __attribute__((always_inline))
#	define NV_NO_INLINE __attribute__((noinline))

#	define NV_CDECL
#	define NV_STDCALL

#   define NV_BREAKPOINT(id) __builtin_trap();

#	if (__cplusplus >= 201103L) && ((__GNUC__ > 4) || (__GNUC__ ==4 && __GNUC_MINOR__ >= 6))
#		define NV_NULL	nullptr
#		define NV_HAS_ENUM_CLASS
#	else
#		define NV_NULL	__null
#	endif

#	define NV_ALIGN_OF(T)	__alignof__(T)

// Check for C++11
#	if (__cplusplus >= 201103L) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 405
#		define NV_HAS_MOVE_SEMANTICS 1
#		if (__GNUC__ * 100 + __GNUC_MINOR__) >= 407
#			define NV_OVERRIDE override 
#		endif
#	endif

#	define NV_RESTRICT	__restrict	
#endif


// Visual Studio
#ifdef NV_VC
#	define NV_ALIGN_PREFIX(bytes) __declspec(align(bytes))
#	define NV_ALIGN_SUFFIX(bytes) 

#	define NV_PUSH_PACK_DEFAULT __pragma(pack(push, 8))
#	define NV_POP_PACK __pragma(pack(pop))

#	define NV_FORCE_INLINE __forceinline
#	define NV_NO_INLINE __declspec(noinline)

#	define NV_CDECL __cdecl
#	define NV_STDCALL __stdcall
#	define NV_BREAKPOINT(id) __debugbreak();
#	define NV_NULL	nullptr
#	define NV_ALIGN_OF(T) __alignof(T)

#	define NV_OVERRIDE	override

// C4481: nonstandard extension used: override specifier 'override'
#	if _MSC_VER < 1700
#		pragma warning(disable : 4481)
#	endif

#	if (_MSC_VER >= 1400)
#		define NV_RESTRICT					__restrict
#	endif
#	if _MSC_VER >= 1600
#		define NV_HAS_MOVE_SEMANTICS 1
#	endif
#	if _MSC_VER >= 1700
#		define NV_HAS_ENUM_CLASS 1
#   endif

// If set the preprocessor is 'ms style' which is not standards compliant
// https://stackoverflow.com/questions/11469462/difference-between-gcc-and-microsoft-preprocessor
#	define NV_HAS_MS_PRE_PROC
#endif

#ifdef NV_GCC_FAMILY
#	define NV_OFFSET_OF(X, Y) __builtin_offsetof(X, Y)
#else
#	define NV_OFFSET_OF(X, Y) offsetof(X, Y)
#endif

#ifndef NV_RESTRICT
#	define NV_RESTRICT 
#endif

#ifndef NV_BREAKPOINT
// Make it crash with a write to 0!
#   define NV_BREAKPOINT(id) (*((int*)0) = int(id));
#endif

#ifndef NV_OVERRIDE
#	define NV_OVERRIDE
#endif

#ifndef NV_NULL
#	if defined(NULL)
#		define NV_NULL NULL
#	else
#		define NV_NULL 0
#	endif
#endif

#if defined(NV_HAS_ENUM_CLASS)
#	define NV_ENUM_START(name) enum struct name  
#	define NV_ENUM_END(name) typedef name E##name;
#else
#	define NV_ENUM_START(name) class name { name(); public: enum Enum  
#	define NV_ENUM_END(name) }; typedef name::Enum E##name;
#endif

#ifdef __ANDROID__
#	define NV_IMPORT 
#	define NV_EXPORT __attribute__ ((visibility ("default")))
#else
// also supported by GCC, dllexport implies "default" visibility
#	define NV_IMPORT __declspec(dllimport)
#	define NV_EXPORT __declspec(dllexport)
#endif

#ifdef __cplusplus
#	define NV_C_EXPORT extern "C"
#else
#	define NV_C_EXPORT
#endif

#endif // NV_MACROS

#ifndef NV_ALIGN
#	define NV_ALIGN(bytes, decl) NV_ALIGN_PREFIX(bytes) decl NV_ALIGN_SUFFIX(bytes)
#endif

#ifndef NV_DEBUG
#	ifdef NDEBUG
#		define NV_DEBUG 0
#	else
#		define NV_DEBUG 1
#	endif
#endif // NV_DEBUG

#if defined(NV_WINDOWS_FAMILY) 
#	define NV_NO_ALIAS __declspec(noalias)
#else
#	define NV_NO_ALIAS
#endif

/**
Deprecated macro

To deprecate a function: Place NV_DEPRECATED at the start of the function header (leftmost word).
To deprecate a 'typedef', a 'struct' or a 'class': Place NV_DEPRECATED directly after the keywords ('typedef', 'struct', 'class').
Use these macro definitions to create warnings for deprecated functions
Define NV_ENABLE_DEPRECIATION_WARNINGS to enable warnings
*/
#ifdef NV_ENABLE_DEPRECIATION_WARNINGS
#	ifdef NV_GCC
#		define NV_DEPRECATED __attribute__((deprecated()))
#	elif defined(NV_VC)
#		define NV_DEPRECATED __declspec(deprecated)
#	else
#		define NV_DEPRECATED
#	endif
#endif

#ifndef NV_DEPRECATED
#	define NV_DEPRECATED
#endif


// For getting sizes of constant arrays
#define NV_COUNT_OF(x) (sizeof(x)/sizeof(x[0]))

// Assert

#define NV_ASSERT(exp) (assert(exp))
#define NV_ALWAYS_ASSERT() NV_ASSERT(0)

// static assert
#define NV_COMPILE_TIME_ASSERT(exp)	typedef char Nv_CompileTimeAssert[(exp) ? 1 : -1]

// General defines
#define NV_UNUSED(x) (void)x;
#define NV_SIZE_OF(cls, mem)			sizeof(((cls*)NV_NULL)->mem)
#define NV_INLINE inline

#define NV_FUNCTION_NAME __FUNCTION__

// Cuda macros
#ifdef __CUDACC__
#	define NV_CUDA_CALLABLE __host__ __device__
#else
#	define NV_CUDA_CALLABLE
#endif

// Define basic types available in Nv namespace
namespace Nv
{

/// These types exist such that in the Nv namespace we can follow the convention 
/// that type names are upper camel, even with built in types. 
/// This also gives flexibility - to change representations as necessary

typedef bool Bool;
typedef char Char;			///< Char is 8 bit 
typedef	uint8_t UInt8;
typedef int8_t Int8;
typedef	uint16_t UInt16;
typedef int16_t Int16;
typedef	uint32_t UInt32;
typedef int32_t Int32;
typedef	uint64_t UInt64;
typedef int64_t Int64;
typedef	float Float32;
typedef double Float64;

// Non sized types - default for the compiler/platform
// Must at a minimum be 32 bits. Never larger than sizeof memory types
typedef int Int;
typedef unsigned int UInt;
typedef float Float;

// Misc
typedef void Void;

// Memory/pointer types
typedef size_t SizeT;
typedef ptrdiff_t PtrDiffT;

// The type used for indexing. 
// This uses a signed type to avoid problems with numerical under/overflow
// It is not necessarily the same size as a pointer. This means it might _not_ 
// be able to address the full address space of bytes. 
typedef PtrDiffT IndexT;	
	
// Type used for hashing 
typedef UInt32 Hash32;
typedef UInt64 Hash64;

#define NV_CHECK_SIGNED(type, size) NV_COMPILE_TIME_ASSERT(sizeof(type) == size && type(~type(0)) < type(0))
#define NV_CHECK_UNSIGNED(type, size) NV_COMPILE_TIME_ASSERT(sizeof(type) == size && type(~type(0)) > type(0))

NV_CHECK_SIGNED(Int8, 1);
NV_CHECK_SIGNED(Int16, 2);
NV_CHECK_SIGNED(Int32, 4);
NV_CHECK_SIGNED(Int64, 8);
NV_CHECK_UNSIGNED(UInt8, 1);
NV_CHECK_UNSIGNED(UInt16, 2);
NV_CHECK_UNSIGNED(UInt32, 4);
NV_CHECK_UNSIGNED(UInt64, 8);

NV_COMPILE_TIME_ASSERT(sizeof(Float32) == 4);
NV_COMPILE_TIME_ASSERT(sizeof(Float64) == 8);

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

} // namespace Nv

// Type ranges
#define	NV_MAX_I8			127					//maximum possible sbyte value, 0x7f
#define	NV_MIN_I8			(-128)				//minimum possible sbyte value, 0x80
#define	NV_MAX_U8			255U				//maximum possible ubyte value, 0xff
#define	NV_MIN_U8			0					//minimum possible ubyte value, 0x00
#define	NV_MAX_I16			32767				//maximum possible sword value, 0x7fff
#define	NV_MIN_I16			(-32768)			//minimum possible sword value, 0x8000
#define	NV_MAX_U16			65535U				//maximum possible uword value, 0xffff
#define	NV_MIN_U16			0					//minimum possible uword value, 0x0000
#define	NV_MAX_I32			2147483647			//maximum possible sdword value, 0x7fffffff
#define	NV_MIN_I32			(-2147483647 - 1)	//minimum possible sdword value, 0x80000000
#define	NV_MAX_U32			4294967295U			//maximum possible udword value, 0xffffffff
#define	NV_MIN_U32			0					//minimum possible udword value, 0x00000000
#define	NV_MAX_F32			3.4028234663852885981170418348452e+38F	
												//maximum possible float value
#define	NV_MAX_F64			DBL_MAX				//maximum possible double value

#define NV_EPS_F32			FLT_EPSILON			//maximum relative error of float rounding
#define NV_EPS_F64			DBL_EPSILON			//maximum relative error of double rounding

// Set up the previous legacy aliases

// Short hand/global namespace versions of basic types
typedef Nv::UInt8 NvUInt8;
typedef Nv::Int8 NvInt8 ;
typedef Nv::UInt16 NvUInt16;
typedef Nv::Int16 NvInt16 ;
typedef Nv::UInt32 NvUInt32;
typedef Nv::Int32 NvInt32;
typedef Nv::UInt64 NvUInt64;
typedef Nv::Int64 NvInt64;

typedef Nv::Float32 NvFloat32;
typedef Nv::Float64 NvFloat64;

typedef Nv::Bool NvBool;
typedef Nv::Char NvChar;

// Use when just want most suitable size, also helper so can use upper camel naming convention
typedef Nv::Int NvInt;
typedef Nv::UInt NvUInt;
typedef Nv::Float NvFloat;
typedef Nv::Void NvVoid;

#endif // NV_COMMON_H
