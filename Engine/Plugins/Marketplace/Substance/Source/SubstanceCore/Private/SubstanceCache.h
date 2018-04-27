// Copyright 2018 Allegorithmic Inc. All rights reserved.
// File: SubstanceCache.h

#pragma once

/** Forward Declare */
class FArchive;
class USubstanceTexture2D;

namespace SubstanceAir
{
class OutputInstance;
class GraphInstance;
}

namespace Substance
{
class SubstanceCache
{
public:
	/** Singleton implementation to create - return class */
	static TSharedRef<SubstanceCache> Get()
	{
		if (!SbsCache.IsValid())
			SbsCache = MakeShared<SubstanceCache>();

		return SbsCache.ToSharedRef();
	}

	/** Cleans up class instance */
	static void Shutdown();

	//#TODO:: Switch to SubstanceAir::weak_ptr when available
	/** Checks to see if graph is stored in cache and can be read */
	bool CanReadFromCache(std::weak_ptr<SubstanceAir::GraphInstance> graph);

	/** Load a graph instance from cache */
	bool ReadFromCache(std::weak_ptr<SubstanceAir::GraphInstance> graph);

	/** Save an output instance to cache */
	void CacheOutput(SubstanceAir::OutputInstance* Output, const SubstanceTexture& result);

private:
	/** Get the path to the cache working directory path */
	FString GetPathForGuid(const FGuid& guid) const;

	/** Serializes frameworks SubstanceTexture objects */
	bool SerializeTexture(FArchive& Ar, SubstanceTexture& result) const;

	/** Singleton class instance */
	static TSharedPtr<SubstanceCache> SbsCache;
};
}
