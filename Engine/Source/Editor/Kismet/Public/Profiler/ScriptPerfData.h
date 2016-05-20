// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// EScriptPerfDataType

enum EScriptPerfDataType
{
	Event = 0,
	Node
};

//////////////////////////////////////////////////////////////////////////
// FScriptPerfData

class KISMET_API FScriptPerfData : public TSharedFromThis<FScriptPerfData>
{
public:

	FScriptPerfData(const EScriptPerfDataType StatTypeIn)
		: StatType(StatTypeIn)
		, InclusiveTiming(0.0)
		, NodeTiming(0.0)
		, MaxTiming(-MAX_dbl)
		, MinTiming(MAX_dbl)
		, TotalTiming(0.0)
		, NumSamples(0)
	{
	}

	/** Add a single event node timing into the dataset */
	void AddEventTiming(const double NodeTimingIn);

	/** Add a single event pure node timing into the dataset */
	void AddPureChainTiming(const double PureTimingIn);

	/** Add a inclusive timing into the dataset */
	void AddInclusiveTiming(const double InclusiveNodeTimingIn);

	/** Add data */
	void AddData(const FScriptPerfData& DataIn);

	/** Add branch data */
	void AddBranchData(const FScriptPerfData& DataIn);

	/** Increments sample count without affecting data */
	void TickSamples() { NumSamples++; }

	/** Reset the current data buffer and all derived data stats */
	void Reset();

	// Returns if this data structure has any valid data
	bool IsDataValid() const { return NumSamples > 0; }

	// Updates the various thresholds that control stats calcs and display.
	static void SetNumberFormattingForStats(const FNumberFormattingOptions& FormatIn) { StatNumberFormat = FormatIn; }
	static void SetRecentSampleBias(const float RecentSampleBiasIn);
	static void SetEventPerformanceThreshold(const float EventPerformanceThresholdIn);
	static void SetNodePerformanceThreshold(const float NodePerformanceThresholdIn);
	static void SetInclusivePerformanceThreshold(const float InclusivePerformanceThresholdIn);
	static void SetMaxPerformanceThreshold(const float MaxPerformanceThresholdIn);

	// Returns the various stats this container holds
	double GetNodeTiming() const { return NodeTiming; }
	double GetInclusiveTiming() const { return InclusiveTiming; }
	double GetMaxTiming() const { return MaxTiming; }
	double GetMinTiming() const { return MinTiming; }
	double GetTotalTiming() const { return TotalTiming; }
	int32 GetSampleCount() const { return NumSamples; }

	// Returns various performance heat levels for visual display
	float GetNodeHeatLevel() const;
	float GetInclusiveHeatLevel() const;
	float GetMaxTimeHeatLevel() const;

	// Returns the various stats this container holds in FText format
	FText GetTotalTimingText() const;
	FText GetInclusiveTimingText() const;
	FText GetNodeTimingText() const;
	FText GetMaxTimingText() const;
	FText GetMinTimingText() const;
	FText GetSamplesText() const;

private:

	/** The stat type for performance threshold comparisons */
	EScriptPerfDataType StatType;
	/** Inclusive timing, pure and node timings */
	double InclusiveTiming;
	/** Node timing */
	double NodeTiming;
	/** Max exclusive timing */
	double MaxTiming;
	/** Min exclusive timing */
	double MinTiming;
	/** Total time accrued */
	double TotalTiming;
	/** The current inclusive sample count */
	int32 NumSamples;

	/** Number formatting options */
	static FNumberFormattingOptions StatNumberFormat;

	/** Controls the bias between new and older samples */
	static float RecentSampleBias;
	/** Historical Sample Bias */
	static float HistoricalSampleBias;
	/** Event performance threshold */
	static float EventPerformanceThreshold;
	/** Node performance threshold */
	static float NodePerformanceThreshold;
	/** Node inclusive performance threshold */
	static float InclusivePerformanceThreshold;
	/** Node max performance threshold */
	static float MaxPerformanceThreshold;

};
