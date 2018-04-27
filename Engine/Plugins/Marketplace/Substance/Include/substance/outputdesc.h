/** @file outputdesc.h
	@brief Substance output description and current state structure
	@author Christophe Soum - Allegorithmic (christophe.soum@allegorithmic.com)
	@note This file is part of the Substance engine headers
	@date 20080107
	@copyright Allegorithmic. All rights reserved.

	Defines the SubstanceOutputDesc structure. Filled using the
	substanceHandleGetOutputDesc function declared in handle.h.
	Contains format, size, state, etc. information.
*/

#ifndef _SUBSTANCE_OUTPUTDESC_H
#define _SUBSTANCE_OUTPUTDESC_H


/** Pixel formats definitions include */
#include "pixelformat.h"


/** @brief Substance output state enumeration

	Mutually exclusive values. 8 bits format */
typedef enum
{
	Substance_OState_NotComputed,         /**< Output not ready */
	Substance_OState_Ready,               /**< Output computed and ready */
	Substance_OState_Grabbed              /**< Output already grabbed by the user */
	
} SubstanceOutputState;


/** @brief Substance output description and current state structure definition

	Filled using the substanceHandleGetOutputDesc function declared in handle.h. */
typedef struct SubstanceOutputDesc_
{

	/** @brief Output unique identifier. */
	unsigned int outputId;
	
	/** @brief Width of the texture 
		@warning Set to 0 if the texture output width is dynamic 
			(depend on the inputs). */
	unsigned int level0Width;
	
	/** @brief Height of the texture
		@warning Set to 0 if the texture output height is dynamic 
			(depend on the inputs). */
	unsigned int level0Height;
	
	/** @brief Pixel format of the texture
		@see pixelformat.h */
	unsigned char pixelFormat;
	
	/** @brief Depth of the mipmap pyramid (0: full pyramid) */
	unsigned char mipmapCount;
	
	/** @brief Current output state */
	SubstanceOutputState state;

} SubstanceOutputDesc;

#endif /* ifndef _SUBSTANCE_OUTPUTDESC_H */
