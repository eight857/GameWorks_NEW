/** @file device.h
	@brief Platform specific API device structure
	@author Christophe Soum - Allegorithmic (christophe.soum@allegorithmic.com)
	@note This file is part of the Substance engine headers
	@date 20080108
	@copyright Allegorithmic. All rights reserved.

	Defines the SubstanceDevice structure. Platform dependent definition. Used
	to initialize the SubstanceContext structure. */

#ifndef _SUBSTANCE_DEVICE_H
#define _SUBSTANCE_DEVICE_H


/** Platform dependent definitions */
#include "platformdep.h"


/* Platform specific management */
#ifdef SUBSTANCE_PLATFORM_D3D9PC
	#define SUBSTANCE_DEVICE_D3D9 1
#endif /* ifdef SUBSTANCE_PLATFORM_D3D9PC */

#ifdef SUBSTANCE_PLATFORM_D3D10PC
	#define SUBSTANCE_DEVICE_D3D10 1
#endif /* ifdef SUBSTANCE_PLATFORM_D3D10PC */

#if defined(SUBSTANCE_PLATFORM_D3D11PC) || \
    defined(SUBSTANCE_PLATFORM_D3D11XBOXONE)
	#define SUBSTANCE_DEVICE_D3D11 1
#endif /* ifdef SUBSTANCE_PLATFORM_D3D11PC */

#if defined(SUBSTANCE_PLATFORM_OGLES2) || \
    defined(SUBSTANCE_PLATFORM_OGL2)   || \
    defined(SUBSTANCE_PLATFORM_OGL3)
	#define SUBSTANCE_DEVICE_NULL 1
#endif

#ifdef SUBSTANCE_PLATFORM_PS4
	#define SUBSTANCE_DEVICE_GNM 1
#endif /* ifdef SUBSTANCE_PLATFORM_PS4 */

#ifdef SUBSTANCE_PLATFORM_BLEND
	#define SUBSTANCE_DEVICE_NULL 1
#endif /* ifdef SUBSTANCE_PLATFORM_RAWMEMORYOUTPUT */


#if defined(SUBSTANCE_DEVICE_D3D9)

	/** @brief Substance engine device structure

	    Direct3D9 PC version. Must be correctly filled when used
		to initialize the SubstanceContext structure. */
	typedef struct SubstanceDevice_
	{
		/** @brief Direct3D Device handle */
		LPDIRECT3DDEVICE9 handle;

	} SubstanceDevice;

#elif defined(SUBSTANCE_DEVICE_D3D10)

	/** @brief Substance engine device structure

	    Direct3D10 PC. Must be correctly filled when used to initialize the
		SubstanceContext structure. */
	typedef struct SubstanceDevice_
	{
		/** @brief Direct3D Device handle */
		ID3D10Device *handle;

	} SubstanceDevice;

#elif defined(SUBSTANCE_DEVICE_D3D11)

	/** @brief Substance engine device structure

	    Direct3D11 PC. Must be correctly filled when used to initialize the
		SubstanceContext structure. */
	typedef struct SubstanceDevice_
	{
		/** @brief Direct3D Device handle */
		#ifdef SUBSTANCE_PLATFORM_D3D11XBOXONE
		ID3D11DeviceX *handle;
		#else
		ID3D11Device *handle;
		#endif

	} SubstanceDevice;

#elif defined(SUBSTANCE_DEVICE_GNM)

	/** @brief Substance engine device structure

	    PS4 GNM version. Must be correctly filled when used to initialize the
	    SubstanceContext structure. */
	typedef struct SubstanceDevice_
	{
		/** @brief GNM Context data structure pointer

		    This structure is used for managing and controlling
	        the command buffer. */
		sce::Gnmx::LightweightGfxContext* gnmContext;

	} SubstanceDevice;

#elif defined(SUBSTANCE_DEVICE_NULL)

	/** @brief Substance engine device structure

	    Generic version. Dummy structure. No device-like objects are necessary.
		The Substance context can be built without any device pointer. */
	typedef struct SubstanceDevice_
	{
		/** @brief Dummy value (avoid 0 sized structure). */
		int dummy_;

	} SubstanceDevice;

#else

	/** @todo Specify device structures for other APIs */
	#error NYI

#endif



#endif /* ifndef _SUBSTANCE_DEVICE_H */
