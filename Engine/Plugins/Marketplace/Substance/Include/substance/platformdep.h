/** @file platformdep.h
	@brief Platform specific definitions
	@author Christophe Soum - Allegorithmic (christophe.soum@allegorithmic.com)
	@note This file is part of the Substance engine headers
	@date 20080108
	@copyright Allegorithmic. All rights reserved.

	Defines platform specific macros. One of these macros must already be
	defined (preprocessor #define):
	  - SUBSTANCE_PLATFORM_BLEND: generic platform, results in system memory
	  - SUBSTANCE_PLATFORM_D3D9PC: PC Direct3D9 API
	  - SUBSTANCE_PLATFORM_D3D10PC: PC Direct3D10 API
	  - SUBSTANCE_PLATFORM_D3D11PC: PC Direct3D11 API
	  - SUBSTANCE_PLATFORM_OGL2: OpenGL 2 API
	  - SUBSTANCE_PLATFORM_OGL3: OpenGL 3 API
	  - SUBSTANCE_PLATFORM_OGLES2: OpenGL ES 2 API
	  - SUBSTANCE_PLATFORM_PS4: PS4 GNM API
	  etc. */

#ifndef _SUBSTANCE_PLATFORMDEP_H
#define _SUBSTANCE_PLATFORMDEP_H

/** Base definitions */
#include "defines.h"


/* Platform specific headers */

#if defined(SUBSTANCE_PLATFORM_D3D9PC)

	/* PC Direct3D9 API */
	#ifndef D3D_DEBUG_INFO
		#define D3D_DEBUG_INFO 1
	#endif //D3D_DEBUG_INFO
	#pragma pack(push,8)
	#define D3D_OVERLOADS 1
	#include <d3d9.h>
	#pragma pack(pop)

	#define SUBSTANCE_API_PLATFORM 0x3

#elif defined(SUBSTANCE_PLATFORM_D3D10PC)

	/* PC Direct3D10 API */
	#include <d3d10.h>

	#define SUBSTANCE_API_PLATFORM 0x7

#elif defined(SUBSTANCE_PLATFORM_D3D11PC)

	/* PC Direct3D11 API */
	#include <d3d11.h>

	#define SUBSTANCE_API_PLATFORM 0x9

#elif defined(SUBSTANCE_PLATFORM_D3D11XBOXONE)

	/* Xbox One Direct3D11 API */
	#include <d3d11_x.h>

	#define SUBSTANCE_API_PLATFORM 0x9

#elif defined(SUBSTANCE_PLATFORM_OGLES2)

	/* OpenGL ES 2 platform */
	/* No header needed */

	#define SUBSTANCE_API_PLATFORM 0xD

#elif defined(SUBSTANCE_PLATFORM_OGL2)

	/* OpenGL GLSL platform */
	/* No header needed */

	#define SUBSTANCE_API_PLATFORM 0x1

#elif defined(SUBSTANCE_PLATFORM_OGL3)

	/* OpenGL 3.x GLSL platform */
	/* No header needed */

	#define SUBSTANCE_API_PLATFORM 0xC

#elif defined(SUBSTANCE_PLATFORM_PS4)

	/* PS4 GNM platform */
	#include <gnm.h>
	#include <gnmx.h>

	#define SUBSTANCE_API_PLATFORM 0xE

#elif defined(SUBSTANCE_PLATFORM_BLEND)

	/* Blend platform */
	/* No header needed */

	#define SUBSTANCE_API_PLATFORM 0x0

#else

	/* platform not supported / define not set */
	#error Platform NYI or proper SUBSTANCE_PLATFORM_xxx preprocessor macro not set

#endif


#endif /* ifndef _SUBSTANCE_PLATFORMDEP_H */
