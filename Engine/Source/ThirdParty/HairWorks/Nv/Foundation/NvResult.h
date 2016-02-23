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

#ifndef _NV_RESULT_H_
#define _NV_RESULT_H_

#include <Nv/Foundation/NvCommon.h>

namespace Nv {

typedef Int32 Result;

extern void HandleError(Result res);

/* Result is designed to be compatible with COM HRESULT

31             30-16       15-0
Severity     Facility   Return Code

* Severity *
0 Success
1 Fail

* Facility *
Were the error originated from

* Return code *
Specific to a facility

The layout is designed such that failure is a negative number, and success is positive.
*/

/// Make a result
#define NV_MAKE_RESULT(sev, fac, code) Nv::Result( (NvUInt32(sev)<<31) | (NvUInt32(fac)<<16) | NvUInt32(code))

/// Will be 0 - for ok, 1 for failure
#define NV_GET_RESULT_SEVERITY(r)	(NvUInt32(r) >> 31)
/// Will be 0 - for ok, 1 for failure
#define NV_GET_RESULT_FACILITY(r)	((NvUInt32(r) >> 16) & 0x7fff)
/// Get the result code
#define NV_GET_RESULT_CODE(r)		(NvUInt32(r) & 0xffff)

#define NV_SEVERITY_ERROR         1
#define NV_SEVERITY_SUCCESS       0

#define NV_MAKE_ERROR(fac, code)	NV_MAKE_RESULT(NV_SEVERITY_ERROR, NV_FACILITY_##fac, code)
#define NV_MAKE_SUCCESS(fac, code)	NV_MAKE_RESULT(NV_SEVERITY_SUCCESS, NV_FACILITY_##fac, code)

/*************************** Facilities ************************************/

#define NV_FACILITY_GENERAL      0
#define NV_FACILITY_DISK         1
#define NV_FACILITY_INTERFACE    2
#define NV_FACILITY_UNKNOWN      3

// Base facility -> so as to not class with HRESULT values
#define NV_FACILITY_BASE		 100				

// Facilities. Facilities numbers must be unique across a project to make the resulting result a unique number!
#define NV_FACILITY_FOUNDATION	NV_FACILITY_BASE
#define NV_FACILITY_HAIR		(NV_FACILITY_BASE + 1)

/* *************************** Codes **************************************/
/// Result codes have the following style
//
// 1) NV_name
// 2) NV_sf_name
// 3) NV_s_f_name
// where s is severity as a single letter S - success, and E for error
// Style 1 is reserved for NV_OK and NV_FAIL as they are so common and not tied to a facility

// x is E for error S for success. Most are errors
// y is typically a single letter to make the result name unique 
// 
// For the common used NV_OK and NV_FAIL, the xy prefix is dropped
// It is acceptable to expand y to a longer name to differentiate a name 
// ie for a FACILITY 'TOAST' it might make sense to have an error of the form
// NV_E_TOAST_OUT_OF_MEMORY for example

// Memory
#define NV_EM_OUT_OF_MEMORY            NV_MAKE_ERROR(MEMORY, 1)
#define NV_EM_BUFFER_TOO_SMALL         NV_MAKE_ERROR(MEMORY, 2)

	// Special cases don't need the E/S prefixes

// NV_OK is equivalent to NV_MAKE_RESULT(0, NV_FACILITY_GENERAL, 0)
#define NV_OK                          0
#define NV_FAIL                        NV_MAKE_ERROR(FOUNDATION, 1)

/* !!!!!!!!!!!!!!!!!!!!!!!!! Checking codes !!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

#define NV_FAILED(status) ((status) < 0)
#define NV_SUCCEEDED(status) ((status) >= 0)

#if 0 && defined(NV_DEBUG)
#	define NV_RETURN_ON_FAIL(x) { ::Nv::Result _res = (x); if (NV_FAILED(_res)) { Nv::HandleError(_res); return _res; } }
#	define NV_RETURN_VOID_ON_FAIL(x) { ::Nv::Result _res = (x); if (NV_FAILED(_res)) {  Nv::HandleError(_res); return; } }
#	define NV_ASSERT_ON_FAIL(x) { ::Nv::Result _res = (x); if (NV_FAILED(_res)) { Nv::HandleError(_res); NV_ASSERT(false); return _res; } }
#	define NV_ASSERT_VOID_ON_FAIL(x) { ::Nv::Result _res = (x); if (NV_FAILED(_res)) { Nv::HandleError(_res); NV_ASSERT(false); return; } }
#else
#	define NV_RETURN_ON_FAIL(x) { ::Nv::Result _res = (x); if (NV_FAILED(_res)) return _res; }
#	define NV_RETURN_VOID_ON_FAIL(x) { ::Nv::Result _res = (x); if (NV_FAILED(_res)) return; }
#	define NV_ASSERT_ON_FAIL(x) { ::Nv::Result _res = (x); if (NV_FAILED(_res)) { Nv::HandleError(_res); NV_ASSERT(false); return _res; } }
#	define NV_ASSERT_VOID_ON_FAIL(x) { ::Nv::Result _res = (x); if (NV_FAILED(_res)) { NV_ASSERT(false); return; } }
#endif

} // namespace Nv

typedef Nv::Result NvResult;

#endif
