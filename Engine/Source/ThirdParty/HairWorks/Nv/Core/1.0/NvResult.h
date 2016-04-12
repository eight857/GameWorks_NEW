/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#ifndef NV_RESULT_H
#define NV_RESULT_H

#include "NvAssert.h"
#include "NvTypes.h"

/** \addtogroup core
@{
*/

typedef NvInt32 NvResult;

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
#define NV_MAKE_RESULT(sev, fac, code) ((((NvInt32)(sev))<<31) | (((NvInt32)(fac))<<16) | ((NvInt32)(code)))

/// Will be 0 - for ok, 1 for failure
#define NV_GET_RESULT_SEVERITY(r)	((NvInt)(((NvUInt32)(r)) >> 31))
/// Will be 0 - for ok, 1 for failure
#define NV_GET_RESULT_FACILITY(r)	((NvInt)(((r) >> 16) & 0x7fff))
/// Get the result code
#define NV_GET_RESULT_CODE(r)		((NvInt)((r) & 0xffff))

#define NV_SEVERITY_ERROR         1
#define NV_SEVERITY_SUCCESS       0

#define NV_MAKE_ERROR(fac, code)	NV_MAKE_RESULT(NV_SEVERITY_ERROR, NV_FACILITY_##fac, code)
#define NV_MAKE_SUCCESS(fac, code)	NV_MAKE_RESULT(NV_SEVERITY_SUCCESS, NV_FACILITY_##fac, code)

/*************************** Facilities ************************************/

// General - careful to make compatible with HRESULT
#define NV_FACILITY_GENERAL      0

// Base facility -> so as to not clash with HRESULT values
#define NV_FACILITY_BASE		 0x100				

// Facilities. Facilities numbers must be unique across a project to make the resulting result a unique number!
// It can be useful to have a consistent short name for a facility, as used in the name prefix
// By convention INTERFACE <-> INTF, UNKNOWN <-> UNK, MEMORY <-> MEM, others use their full name 
#define NV_FACILITY_DISK         (NV_FACILITY_BASE + 1)
#define NV_FACILITY_INTERFACE    (NV_FACILITY_BASE + 2)
#define NV_FACILITY_UNKNOWN      (NV_FACILITY_BASE + 3)
#define NV_FACILITY_MEMORY		 (NV_FACILITY_BASE + 4)
#define NV_FACILITY_MISC	     (NV_FACILITY_BASE + 5)

// For external modules
#define NV_FACILITY_EXTERNAL_BASE 0x200
#define NV_FACILITY_HAIR		(NV_FACILITY_EXTERNAL_BASE + 1)

/* *************************** Codes **************************************/
/// Result codes have the following style
//
// 1) NV_name
// 2) NV_s_f_name
// 3) NV_s_name

// where s is severity as a single letter S - success, and E for error
// Style 1 is reserved for NV_OK and NV_FAIL as they are so common and not tied to a facility

// s is E for error S for success. Most are errors
// f is typically a single letter to make the result name unique 
// 
// For the common used NV_OK and NV_FAIL, the name prefix is dropped
// It is acceptable to expand 'f' to a longer name to differentiate a name 
// ie for a FACILITY 'DRIVER' it might make sense to have an error of the form
// NV_E_DRIVER_OUT_OF_MEMORY for example

// Memory
#define NV_E_MEM_OUT_OF_MEMORY            NV_MAKE_ERROR(MEMORY, 1)
#define NV_E_MEM_BUFFER_TOO_SMALL         NV_MAKE_ERROR(MEMORY, 2)

// Special cases don't need the E/S prefixes

// NV_OK is equivalent to NV_MAKE_RESULT(0, NV_FACILITY_GENERAL, 0)
#define NV_OK                          0
#define NV_FAIL                        NV_MAKE_ERROR(GENERAL, 1)

// Used to identify a Result that has yet to be initialized.  
// It defaults to failure such that if used incorrectly will fail, as similar in concept to using an uninitialized variable. 
#define NV_E_MISC_UNINITIALIZED		   NV_MAKE_ERROR(MISC, 2)			

/* Set NV_HANDLE_RESULT_ERROR(x) to code to be executed whenever an error occurs, and is detected by one of the macros */

#ifndef NV_HANDLE_RESULT_FAIL
#	define NV_HANDLE_RESULT_FAIL(x)
#endif

/* !!!!!!!!!!!!!!!!!!!!!!!!! Checking codes !!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

#define NV_FAILED(status) ((status) < 0)
#define NV_SUCCEEDED(status) ((status) >= 0)

#define NV_RETURN_ON_FAIL(x) { NvResult _res = (x); if (NV_FAILED(_res)) { NV_HANDLE_RESULT_FAIL(_res); return _res; } }
#define NV_RETURN_VOID_ON_FAIL(x) { NvResult _res = (x); if (NV_FAILED(_res)) { NV_HANDLE_RESULT_FAIL(_res); return; } }
#define NV_CORE_ASSERT_ON_FAIL(x) { NvResult _res = (x); if (NV_FAILED(_res)) { NV_CORE_ASSERT(false); return _res; } }
#define NV_CORE_ASSERT_VOID_ON_FAIL(x) { NvResult _res = (x); if (NV_FAILED(_res)) { NV_CORE_ASSERT(false); return; } }

#if __cplusplus
namespace nvidia {
typedef NvResult Result;
} // namespace nvidia
#endif

/** @} */

#endif
