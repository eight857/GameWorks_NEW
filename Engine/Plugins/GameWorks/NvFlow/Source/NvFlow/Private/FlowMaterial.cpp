#include "NvFlowPCH.h"

// NvFlow begin

#include "EnginePrivate.h"

#include "NvFlow.h"

namespace
{
	void CopyMaterialPerComponent(const NvFlowGridMaterialPerComponent& In, FFlowMaterialPerComponent& Out)
	{
		Out.Damping = In.damping;
		Out.Fade = In.fade;
		Out.MacCormackBlendFactor = In.macCormackBlendFactor;
		Out.MacCormackBlendThreshold = In.macCormackBlendThreshold;
		Out.AllocWeight = In.allocWeight;
		Out.AllocThreshold = In.allocThreshold;
	}
}

UFlowMaterial::UFlowMaterial(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NvFlowGridMaterialParams FlowGridMaterialParams;
	NvFlowGridMaterialParamsDefaults(&FlowGridMaterialParams);

	CopyMaterialPerComponent(FlowGridMaterialParams.velocity, Velocity);
	CopyMaterialPerComponent(FlowGridMaterialParams.density, Density);
	CopyMaterialPerComponent(FlowGridMaterialParams.temperature, Temperature);
	CopyMaterialPerComponent(FlowGridMaterialParams.fuel, Fuel);

	VorticityStrength = FlowGridMaterialParams.vorticityStrength;
	VorticityVelocityMask = FlowGridMaterialParams.vorticityVelocityMask;
	IgnitionTemp = FlowGridMaterialParams.ignitionTemp;
	BurnPerTemp = FlowGridMaterialParams.burnPerTemp;
	FuelPerBurn = FlowGridMaterialParams.fuelPerBurn;
	TempPerBurn = FlowGridMaterialParams.tempPerBurn;
	DensityPerBurn = FlowGridMaterialParams.densityPerBurn;
	DivergencePerBurn = FlowGridMaterialParams.divergencePerBurn;
	BuoyancyPerTemp = FlowGridMaterialParams.buoyancyPerTemp;
	CoolingRate = FlowGridMaterialParams.coolingRate;

	UFlowRenderMaterial* DefaultRenderMaterial = CreateDefaultSubobject<UFlowRenderMaterial>(TEXT("DefaultFlowRenderMaterial0"));
	RenderMaterials.Add(DefaultRenderMaterial);
}

// NvFlow end
