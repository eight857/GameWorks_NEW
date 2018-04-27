//! @file output.h
//! @brief Substance Air output informations definition
//! @author Antoine Gonzalez - Allegorithmic
//! @copyright Allegorithmic. All rights reserved.
//! @version 2.0 OutputDesc::mGuiOrder pruned. Outputs are sorted in GUI order
//!		by default.

#ifndef _SUBSTANCE_AIR_FRAMEWORK_OUTPUT_H
#define _SUBSTANCE_AIR_FRAMEWORK_OUTPUT_H

#include "renderresult.h"
#include "channel.h"
#include "typedefs.h"
#include "outputfmt.h"



#include <memory>
#include <algorithm>
#include <utility>


namespace SubstanceAir
{

class InputInstanceBase;
class GraphInstance;
class Renderer;

namespace Details
{
	class RenderToken;
	struct InstantiateOutput;
	class States;
}


//! @brief Substance output description
//! Output descriptions are members of GraphDesc.
struct OutputDesc
{
	//! @brief User-readable identifier of the output
	string mIdentifier;

	//! @brief 32 bits unsigned Unique Identifier
	UInt mUid;

	//! @brief Output format
	//! @see SubstancePixelFormat enum in substance/pixelformat.h
	int mFormat;

	//! @brief Usages of the output as ChannelUse enumerators, or Channel_UNKNOWN if user specific
	vector<ChannelUse> mChannels;

	//! @brief Channel usages, as strings
	//! Useful for elements of mChannels that are Channel_UNKNOWN: user specific syntax.
	vector<string> mChannelsStr;

	//! @brief Number of mipmaps level generated (0: FULL mipmaps pyramid)
	unsigned int mMipmaps;

	//! @brief Label of this input (UI) UTF8 format
	string mLabel;

	//! @brief Group of this output (UI) UTF8 format
	string mGroup;

	//! @brief Expression that allows to control output visibility status (UI).
	//! Empty if no conditional visibility.
	//! @see See visibleif.h header to evaluate this expression. Depend on
	//!		current input values of the parent graph.
	string mGuiVisibleIf;

	//! @brief User tag of this output (UI/integration purpose only) UTF8 format
	string mUserTag;

	//! @brief The UIDs of inputs modifying this output
	//! UIDs are sorted (allows binary search).
	//!
	//! If one of the inputs listed here is modified, this output is flagged as
	//! dirty (outdated) and is recomputed at next rendering.
	//! @warning Dynamic output components composition
	//! 	(OutputFormat::Component::outputUid) is not taken into account.
	Uids mAlteringInputUids;

	//! @brief Internal use only
	bool mFmtOverridden;

	//! @brief Accessor for the default channel usage for this output
	//! Returns the default channel type
	ChannelUse defaultChannelUse() const;

	//! @brief Accessor for the default channel usage for this output in string format
	//! Returns the default channel type as a string
	string defaultChannelUseStr() const;

	//! @brief Checks if the channel use was specified by the user
	//! Returns if the user set at least one channel type this output is intended
	//!	to be used for.
	bool channelUseSpecified() const { return mChannels.size() != 0; }
};


//! @brief Substance output instance
//! Output instances are members of GraphInstance. Each OutputInstance
//! correspond to a OutputDesc. Output instances contain results of the
//! rendering process.
class OutputInstance
{
public:
	//! @brief Auto ptr on render result (returned by grabResult)
	typedef unique_ptr<RenderResult> Result;

	//! @brief Reference on corresponding output description
	const OutputDesc &mDesc;

	//! @brief Reference on parent graph instance
	GraphInstance &mParentGraph;

	//! @brief Flag that allows to disable output rendering
	//! Default: enabled.
	bool mEnabled;

	//! @brief User data, can be used to store pointer or other stuff
	//! Useful in callback functions.
	size_t mUserData;

	//! @brief Override current output format
	//! Allows to override pixel format, mipmap pyramid depth, horizontal and
	//! vertical flipping, resize, per component value remapping (e.g. invert),
	//! components shuffling (e.g. normal map re-layout) and composition from
	//! other graph outputs (e.g. channels composition to match shader layout).
	//! @pre Output UIDs used in component output composition must references
	//! 	outputs of the parent graph.
	//! @post Force output rendering (flag as dirty)
	void overrideFormat(const OutputFormat& newFormat);

	//! @brief Grab render result corresponding to this output
	//! @note If this output was computed several times, this function
	//!     returns the least recent result.
	//! @post Ownership of the rendering result is transferred to the caller.
	//! @warning Results must be deleted before corresponding renderer
	//!		or when renderer engine implementation is switched.
	//! @return Return the rendering result or NULL auto-pointer if no results
	//!		available.
	Result grabResult();

	//! @brief Force output rendering, even if it is not modified by an input
	//! @note The output must be also enabled (mEnabled == true) to be rendered.
	//!
	//! The dirty flag is reset at next render push.
	void flagAsDirty() { mDirty = true; }

	//! @brief Accessor on 'dirty' flag: the output need to be renderer
	bool isDirty() const { return mDirty; }

	//! @brief Returns if format is really overridden
	bool isFormatOverridden() const { return mFormatOverridden; }

	//! @brief Accessor on current format override information
	//! Returns identity override structure if isFormatOverridden() is false
	const OutputFormat& getFormatOverride() const { return mFormatOverride; }

	//! @brief Determine if output was created dynamically
	//! Returns true if the output was created dynamically instead of from a GraphDesc
	bool isDynamicOutput() const { return mDynamicOutput; }

	typedef shared_ptr<Details::RenderToken> Token;		   //!< Internal use only
	void push(const Token&);                               //!< Internal use only
	bool queueRender();                                    //!< Internal use only
	void invalidate();                                     //!< Internal use only
protected:
	typedef deque<Token> RenderTokens;
	typedef vector<OutputInstance*> Ptrs;

	OutputInstance(const OutputDesc&,GraphInstance&,bool);
	void releaseTokensOwnedByEngine(UInt engineUid);

	bool mDirty;
	RenderTokens mRenderTokens;
	OutputFormat mFormatOverride;
	bool mFormatOverridden;
	bool mDynamicOutput;
	Ptrs mCompositeOutput;

	friend class GraphInstance;
	friend struct Details::InstantiateOutput;
	friend class Details::States;
private:
	OutputInstance(const OutputInstance&);
	const OutputInstance& operator=(const OutputInstance&);
};

} // namespace SubstanceAir


#endif //_SUBSTANCE_AIR_FRAMEWORK_OUTPUT_H
