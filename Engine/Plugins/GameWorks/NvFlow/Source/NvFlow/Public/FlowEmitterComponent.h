
/**
 *	NvFlow - Emitter Component
 */
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/ActorComponent.h"
#include "Containers/List.h"

#include "FlowEmitterComponent.generated.h"


#define FLOW_EMITTER_ACCUM_EXACT_FRAME_STATE 0
#define FLOW_EMITTER_LOG_ACCUM_STATE 0
#if FLOW_EMITTER_LOG_ACCUM_STATE
DECLARE_LOG_CATEGORY_EXTERN(LogFlowEmitterState, Log, All);
#endif

struct FBodyState
{
	FTransform Transform;
	FVector LinearVelocity;
	FVector AngularVelocity;

	bool Equals(const FBodyState& Other) const
	{
		return Transform.Equals(Other.Transform) && LinearVelocity.Equals(Other.LinearVelocity) && AngularVelocity.Equals(Other.AngularVelocity);
	}

	void Blend(const FBodyState& State0, const FBodyState& State1, float Alpha)
	{
		Transform.Blend(State0.Transform, State1.Transform, Alpha);
		LinearVelocity = FMath::Lerp(State0.LinearVelocity, State1.LinearVelocity, Alpha);
		AngularVelocity = FMath::Lerp(State0.AngularVelocity, State1.AngularVelocity, Alpha);
	}

#if FLOW_EMITTER_LOG_ACCUM_STATE
	FString ToString() const
	{
		const FVector Location = Transform.GetLocation();
		return FString::Printf(TEXT("loc=(%f %f %f), lvel=(%f %f %f)"), Location.X, Location.Y, Location.Z, LinearVelocity.X, LinearVelocity.Y, LinearVelocity.Z);
	}
#endif
};

template <typename T>
class TResizeableCircularBuffer
{
public:
	TResizeableCircularBuffer(uint32 InReserveCount)
	{
		Capacity = FMath::RoundUpToPowerOfTwo(InReserveCount + 1);
		BeginIdx = EndIdx = 0;
		Elements = new T[Capacity];
	}

	~TResizeableCircularBuffer()
	{
		delete[] Elements;
	}

	void PushBack(const T& Value)
	{
		uint32 NewEndIdx = (EndIdx + 1) & (Capacity - 1);
		if (NewEndIdx == BeginIdx)
		{
			// need to resize
			const uint32 NewCapacity = Capacity * 2;
			T* NewElements = new T[NewCapacity];

			uint32 DstIdx = 0;
			for (uint32 SrcIdx = BeginIdx; SrcIdx < Capacity; ++SrcIdx) NewElements[DstIdx++] = Elements[SrcIdx];
			for (uint32 SrcIdx = 0; SrcIdx < BeginIdx; ++SrcIdx) NewElements[DstIdx++] = Elements[SrcIdx];

			delete[] Elements;

			BeginIdx = 0;
			EndIdx = Capacity - 1;
			NewEndIdx = Capacity;

			Capacity = NewCapacity;
			Elements = NewElements;
		}

		Elements[EndIdx] = Value;
		EndIdx = NewEndIdx;
	}

	bool IsEmpty() const { return (BeginIdx == EndIdx); }

	bool PopFront()
	{
		if (!IsEmpty())
		{
			BeginIdx = (BeginIdx + 1) & (Capacity - 1);
			return true;
		}
		return false;
	}

	T& GetFront() {	check(!IsEmpty()); return Elements[BeginIdx]; }
	const T& GetFront() const { check(!IsEmpty()); return Elements[BeginIdx]; }

	T& GetBack() { check(!IsEmpty()); return Elements[(EndIdx - 1) & (Capacity - 1)]; }
	const T& GetBack() const { check(!IsEmpty()); return Elements[(EndIdx - 1) & (Capacity - 1)]; }

	const uint32 Count() const { return (EndIdx - BeginIdx) & (Capacity - 1); }

	T& operator[] (uint32 Idx) { return Elements[(BeginIdx + Idx) & (Capacity - 1)]; }
	const T& operator[] (uint32 Idx) const { return Elements[(BeginIdx + Idx) & (Capacity - 1)]; }

protected:
	T* Elements;
	uint32 Capacity;
	uint32 BeginIdx;
	uint32 EndIdx;
};


template <typename T>
class FStateAccumulator
{
	struct FTimeState
	{
		float Time;
		T State;
	};

	TResizeableCircularBuffer<FTimeState> CircularBuffer;
	bool bIsStationary;

public:
	FStateAccumulator()
		: CircularBuffer(7), bIsStationary(true)
	{
	}

	void Add(float InTime, const T& InState)
	{
		if (CircularBuffer.IsEmpty())
		{
			CircularBuffer.PushBack( {0.0f, InState} );

#if FLOW_EMITTER_LOG_ACCUM_STATE
			UE_LOG(LogFlowEmitterState, Display, TEXT("FStateAccumulator: Add at 0 node: %s"), *InState.ToString())
#endif
		}
		const FTimeState& Back = CircularBuffer.GetBack();
		check(InTime > Back.Time);
		bIsStationary &= InState.Equals(Back.State);

		CircularBuffer.PushBack( {InTime, InState} );

#if FLOW_EMITTER_LOG_ACCUM_STATE
		UE_LOG(LogFlowEmitterState, Display, TEXT("FStateAccumulator: Add at %f node: %s"), InTime, *InState.ToString())
#endif
	}

	void DiscardBefore(float InTime)
	{
#if FLOW_EMITTER_LOG_ACCUM_STATE
		UE_LOG(LogFlowEmitterState, Display, TEXT("FStateAccumulator: DiscardBefore %f"), InTime)
#endif
		if (InTime > 0.0f && !CircularBuffer.IsEmpty())
		{
			check(InTime <= CircularBuffer.GetBack().Time);

			while (CircularBuffer.Count() > 1 && CircularBuffer[1].Time <= InTime)
			{
				CircularBuffer.PopFront();
			}

			FTimeState& Front = CircularBuffer.GetFront();
#if FLOW_EMITTER_LOG_ACCUM_STATE
			UE_LOG(LogFlowEmitterState, Display, TEXT("FStateAccumulator: DiscardBefore node %f -> %f"), Front.Time, Front.Time - InTime);
#endif
			Front.Time -= InTime;
			bIsStationary = true;
			for (uint32 Idx = 1, Count = CircularBuffer.Count(); Idx < Count; ++Idx)
			{
				FTimeState& Curr = CircularBuffer[Idx];
#if FLOW_EMITTER_LOG_ACCUM_STATE
				UE_LOG(LogFlowEmitterState, Display, TEXT("FStateAccumulator: DiscardBefore node %f -> %f"), Curr.Time, Curr.Time - InTime);
#endif
				Curr.Time -= InTime;
				bIsStationary &= Curr.State.Equals(Front.State);
			}
		}
	}

	bool IsStationary() const { return bIsStationary; }

	class FInterpolator
	{
		const TResizeableCircularBuffer<FTimeState>& CircularBuffer;
		float Time;
		uint32 Idx;
	public:
		FInterpolator(const FStateAccumulator& InAccumulator, float InTime = 0.0f)
			: CircularBuffer(InAccumulator.CircularBuffer), Time(InTime), Idx(0)
		{
			check(CircularBuffer.GetFront().Time <= InTime);
		}

		void AdvanceAndSample(float InTimeStep, T& OutState)
		{
			Time += InTimeStep;

			uint32 Count = CircularBuffer.Count();
			for (; Idx + 1 < Count && CircularBuffer[Idx + 1].Time < Time; ++Idx) {}

			check(CircularBuffer[Idx].Time <= Time);
			check(Idx + 1 < Count && CircularBuffer[Idx + 1].Time >= Time);

			if (Idx + 1 < Count)
			{
				const FTimeState& Curr = CircularBuffer[Idx];
				const FTimeState& Next = CircularBuffer[Idx + 1];
				const float Alpha = (Time - Curr.Time) / (Next.Time - Curr.Time);
				OutState.Blend(Curr.State, Next.State, Alpha);
			}
			else
			{
				OutState = CircularBuffer[Idx].State;
			}
#if FLOW_EMITTER_LOG_ACCUM_STATE
			UE_LOG(LogFlowEmitterState, Display, TEXT("FStateAccumulator: Sample at %f (%f) result: %s"), Time, InTimeStep, *OutState.ToString());
#endif
		}
	};
	friend class FInterpolator;

	float GetLastTime() const
	{
		return CircularBuffer.IsEmpty() ? 0.0f : CircularBuffer.GetBack().Time;
	}

	class FIterator
	{
		const TResizeableCircularBuffer<FTimeState>& CircularBuffer;
		float LastTime;
		float EndTime;
		uint32 Idx;
	public:
		FIterator(const FStateAccumulator& InAccumulator, float InBeginTime, float InEndTime)
			: CircularBuffer(InAccumulator.CircularBuffer), LastTime(InBeginTime), EndTime(InEndTime)
		{
			const uint32 Count = CircularBuffer.Count();
			//skip to BeginTime
			for (Idx = 0; Idx < Count && CircularBuffer[Idx].Time <= InBeginTime; ++Idx) {}
		}

		bool Next(float& DeltaTime, T& OutState)
		{
			const uint32 Count = CircularBuffer.Count();
			if (Idx < Count)
			{
				const FTimeState& Curr = CircularBuffer[Idx];
				if (Curr.Time <= EndTime)
				{
					DeltaTime = Curr.Time - LastTime;
					OutState = Curr.State;
					++Idx;
					LastTime = Curr.Time;
#if FLOW_EMITTER_LOG_ACCUM_STATE
					UE_LOG(LogFlowEmitterState, Display, TEXT("FStateAccumulator: Iterate at %f (%f) node: %s"), Curr.Time, DeltaTime, *OutState.ToString())
#endif
					return true;
				}
			}
			return false;
		}
	};
	friend class FIterator;
};

typedef FStateAccumulator<FBodyState> FBodyStateAccumulator;




UCLASS(ClassGroup = Physics, config = Engine, editinlinenew, HideCategories = (Activation, PhysX), meta = (BlueprintSpawnableComponent), MinimalAPI)
class UFlowEmitterComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	/** Linear emission velocity*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter)
	FVector		LinearVelocity;

	/** Angular emission velocity*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter)
	FVector		AngularVelocity;

	/** Factor between 0 and 1 to blend in physical velocity of actor*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter, meta = (ClampMin = 0.0f, UIMax = 1.0f))
	float 		BlendInPhysicalVelocity;

	UPROPERTY()
	float		Density_DEPRECATED;

	/**  Target smoke*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter, meta = (UIMin = 0.0f, UIMax = 10.0f))
	float		Smoke;

	/** Target temperature*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter, meta = (UIMin = 0.0f, UIMax = 10.0f))
	float		Temperature;

	/** Target fuel*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter, meta = (UIMin = -2.0f, UIMax = 2.0f))
	float		Fuel;

	/** Minimum temperature to release FuelRelease additional fuel*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter, meta = (UIMin = -2.0f, UIMax = 2.0f))
	float		FuelReleaseTemp;

	/** Fuel released when temperature exceeds release temperature*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter, meta = (UIMin = -2.0f, UIMax = 2.0f))
	float		FuelRelease;

	/** Time factor used for pre-allocation of grid cells for fast emitters*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter, meta = (ClampMin = 0.0f, UIMax = 1.0f))
	float		AllocationPredict;

	/** Controls emitter allocation behavior. 0.0 turns emitter allocation off. 1.0 is default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter, meta = (ClampMin = 0.0f, UIMax = 2.0f))
	float		AllocationScale;

	/** 0.0 is pure emitter. 1.0 makes entire shape interior a collider. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter, meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float		CollisionFactor;

	/** Allows inflation of emitter outside of shape surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter, meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float		EmitterInflate;

	/** Rate at which grid cells move to emitter target values. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter, meta = (ClampMin = 0.0f, UIMax = 10.0f))
	float		CoupleRate;

	/** 1.0 makes velocity change based on CoupleRate. 0.0 makes emitter have no effect on velocity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter, meta = (ClampMin = 0.0f, UIMax = 1.0f))
	float VelocityMask;

	UPROPERTY()
	float DensityMask_DEPRECATED;

	/** 1.0 makes smoke change based on CoupleRate. 0.0 makes emitter have no effect on smoke. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter, meta = (ClampMin = 0.0f, UIMax = 1.0f))
	float SmokeMask;

	/** 1.0 makes temperature change based on CoupleRate. 0.0 makes emitter have no effect on temperature. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter, meta = (ClampMin = 0.0f, UIMax = 1.0f))
	float TemperatureMask;

	/** 1.0 makes fuel change based on CoupleRate. 0.0 makes emitter have no effect on fuel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter, meta = (ClampMin = 0.0f, UIMax = 1.0f))
	float FuelMask;

	/** Super sampling of emitter shape along it's path onto flow grid*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter, meta = (ClampMin = 1, UIMax = 10))
	int32		NumSubsteps;

	/** If true, emitter will disable velocity/density emission and will allocate based on the shape instead of the bounding box. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter)
	uint32		bAllocShapeOnly : 1;

#if !FLOW_EMITTER_ACCUM_EXACT_FRAME_STATE
	float LastFrameTime;
	FBodyState LastBodyState;
	bool bIsLastStateTheSame;
#endif
	FBodyStateAccumulator BodyStateAccumulator;

	/** Flow material, if null then taken default material from grid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	class UFlowMaterial* FlowMaterial;

	/** If true, emitter will use DistanceField from a StaticMeshComponent in the same Actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter)
	uint32		bUseDistanceField : 1;

	virtual void PostLoad() override;
};

