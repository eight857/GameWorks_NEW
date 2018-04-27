//! @file renderresult.h
//! @brief Substance Air rendering result
//! @author Antoine Gonzalez - Allegorithmic
//! @copyright Allegorithmic. All rights reserved.

#ifndef _SUBSTANCE_AIR_RENDERRESULT_H
#define _SUBSTANCE_AIR_RENDERRESULT_H

#include "platform.h"
#include "typedefs.h"

#if !defined(AIR_PLATFORM_AGNOSTIC)
	#include <substance/context.h>
#endif //if !defined(AIR_PLATFORM_AGNOSTIC)


struct SubstanceContext_;


namespace SubstanceAir
{
namespace Details
{
	class Engine;
}

struct RenderCallbacks;


//! @brief Substance rendering result struct
//! @invariant Result must be deleted or content must be grabbed before
//!		corresponding renderer deletion or renderer engine implementation is
//!		switch.
struct RenderResult
{
	//! @brief Constructor, internal use only
	RenderResult(const TextureAgnostic&,SubstanceContext_*,Details::Engine*);

	//! @brief Destructor
	//! Delete texture content contained in mSubstanceTexture
	//! if not previously grabbed (releaseBuffer() or releaseTexture()).
	~RenderResult();

	//! @brief Accessor on the Substance context to use for releasing the
	//!		texture content.
	SubstanceContext_* getContext() const { return mContext; }

	#if !defined(AIR_PLATFORM_AGNOSTIC)

		//! @brief Return if hold a texture that match current platform
		bool isPlatformMatch(AIR_PLATFORM_DUMMYPARAM) const
			{ (void)_dummyParam; return SubstanceAir::isPlatformMatch(mTextureAgnostic); }

		//! @brief Accessor on the result texture
		//! Contains texture content (buffer on BLEND platform)
		//! @pre Texture created w/ matching engine platform
		//!		(isPlatformMatch()==true)
		//! @warning Read-only accessor, do not delete the texture content.
		//! @see substance/texture.h
		const SubstanceTexture& getTexture(AIR_PLATFORM_DUMMYPARAM) const
			{ (void)_dummyParam; return castToConcrete(mTextureAgnostic); }

		#ifdef SUBSTANCE_PLATFORM_BLEND
			//! @brief Grab the pixel data buffer
			//! @pre Texture created w/ BLEND platform engine
			//!		(isPlatformMatch()==true)
			//! @warning The ownership of the buffer is given to the caller.
			//! 	The buffer must be freed by alignedFree() (see memory.h).
			//! @return Return the buffer, or NULL if already released
			void* releaseBuffer()
				{ return castToConcrete(releaseTexture()).buffer; }
		#endif //ifdef SUBSTANCE_PLATFORM_BLEND

	#endif // if !defined(AIR_PLATFORM_AGNOSTIC)

	//! @brief Grab texture content ownership, platform agnostic
	//! @pre The texture content was not previously grabbed (haveOwnership()==true)
	//! @return Return the texture. The ownership of the texture content
	//!		is transferred to the caller.
	TextureAgnostic releaseTexture();

	//! @brief Accessor on the result texture, platform agnostic
	//! @warning Read-only accessor, do not delete the texture content.
	//! @return Return the texture or invalid texture if renderer is deleted
	//!		or its engine is switched.
	const TextureAgnostic& getTextureAgnostic() const
		{ return mTextureAgnostic; }

	//! @brief Return if the render result still have ownership on content
	bool haveOwnership() const { return mHaveOwnership; }

	//! @brief Internal use
	Details::Engine* getEngine() const { return mEngine; }

protected:
	TextureAgnostic mTextureAgnostic;
	bool mHaveOwnership;
	SubstanceContext_* mContext;
	Details::Engine* mEngine;

private:
	RenderResult(const RenderResult&);
	const RenderResult& operator=(const RenderResult&);
};


} // namespace SubstanceAir

#endif //_SUBSTANCE_AIR_RENDERRESULT_H
