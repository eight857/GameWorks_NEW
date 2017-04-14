
/**
 *	NvFlow - Emitter Component
 */
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/ActorComponent.h"
#include "Containers/List.h"

#include "FlowEmitterComponent.generated.h"



template <typename T>
class FStateInterpolator
{
	struct FNode : public TIntrusiveLinkedList<FNode>
	{
		float Time;
		T State;

		FNode(float InTime, const T& InState) : Time(InTime), State(InState) {}
	};

	FNode* Head;
	FNode* Tail;

	bool bIsStationary;

public:
	FStateInterpolator()
	{
		Head = Tail = nullptr;
		bIsStationary = true;
	}

	void Add(float InTime, const T& InState)
	{
		if (Tail == nullptr)
		{
			check(Head == nullptr);
			Head = Tail = new FNode(0.0f, InState);
		}
		check(InTime > Tail->Time);
		bIsStationary &= InState.Equals(Tail->State);

		FNode* NewNode = new FNode(InTime, InState);
		NewNode->LinkAfter(Tail);
		Tail = NewNode;
	}

	void DiscardBefore(float InTime)
	{
		if (InTime > 0.0f)
		{
			check(InTime <= Tail->Time);

			check(Head != nullptr);
			for (FNode* Node = Head; Node->Next() != nullptr && Node->Next()->Time <= InTime; )
			{
				Head = Node->Next();
				Node->Unlink();
				delete Node;
				Node = Head;
			}

			check(Head != nullptr);
			Head->Time -= InTime;
			bIsStationary = true;
			for (FNode* Node = Head->Next(); Node != nullptr; Node = Node->Next())
			{
				Node->Time -= InTime;
				bIsStationary &= Node->State.Equals(Head->State);
			}
		}
	}

	bool IsStationary() const { return bIsStationary; }

	class FSampler
	{
		float Time;
		FNode* Node;
	public:
		FSampler(const FStateInterpolator& InOwner, float InTime = 0.0f)
		{
			Time = InTime;
			Node = InOwner.Head;
			check(Node->Time <= InTime);
		}

		void AdvanceAndSample(float InTimeStep, T& OutState)
		{
			Time += InTimeStep;

			while (Node->Next() != nullptr && Node->Next()->Time < Time)
			{
				Node = Node->Next();
			}
			check(Node->Time <= Time);
			check(Node->Next() != nullptr && Node->Next()->Time >= Time);

			if (Node->Next() != nullptr)
			{
				const float Alpha = (Time - Node->Time) / (Node->Next()->Time - Node->Time);
				OutState.Blend(Node->State, Node->Next()->State, Alpha);
			}
			else
			{
				OutState = Node->State;
			}
		}
	};

	friend class FSampler;
};

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
};

typedef FStateInterpolator<FBodyState> FBodyStateInterpolator;

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

	FBodyStateInterpolator BodyStateInterpolator;

	/** Flow material, if null then taken default material from grid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grid)
	class UFlowMaterial* FlowMaterial;

	/** If true, emitter will use DistanceField from a StaticMeshComponent in the same Actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Emitter)
	uint32		bUseDistanceField : 1;

	virtual void PostLoad() override;
};

