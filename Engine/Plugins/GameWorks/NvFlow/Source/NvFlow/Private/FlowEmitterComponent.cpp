#include "NvFlowPCH.h"

/*=============================================================================
	FlowEmitterComponent.cpp: UFlowEmitterComponent methods.
=============================================================================*/

// NvFlow begin

#include "EnginePrivate.h"
#include "PhysicsEngine/PhysXSupport.h"
#include "NvFlow.h"

UFlowEmitterComponent::UFlowEmitterComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsActive = true;
	bAutoActivate = true;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	NvFlowGridEmitParams FlowGridEmitParams;
	NvFlowGridEmitParamsDefaults(&FlowGridEmitParams);

	LinearVelocity = *(FVector*)(&FlowGridEmitParams.velocityLinear.x) * UFlowGridAsset::GetFlowToUE4Scale();
	AngularVelocity = *(FVector*)(&FlowGridEmitParams.velocityAngular.x);
	Density = FlowGridEmitParams.density;
	Temperature = FlowGridEmitParams.temperature;
	Fuel = FlowGridEmitParams.fuel;
	FuelReleaseTemp = FlowGridEmitParams.fuelReleaseTemp;
	FuelRelease = FlowGridEmitParams.fuelRelease;
	AllocationPredict = FlowGridEmitParams.allocationPredict;
	AllocationScale = FlowGridEmitParams.allocationScale.x;
	CollisionFactor = 0.f;
	EmitterInflate = 0.f;
	CoupleRate = 0.5f;
	VelocityMask = 1.f;
	DensityMask = 1.f;
	TemperatureMask = 1.f;
	FuelMask = 1.f;

	BlendInPhysicalVelocity = 1.0f;
	NumSubsteps = 1;

	bHasPreviousTransform = false;
}

// NvFlow end