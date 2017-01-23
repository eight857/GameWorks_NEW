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

	void AddColorMapPoint(UCurveLinearColor* ColorMap, float Time, FLinearColor Color)
	{
		ColorMap->FloatCurves[0].AddKey(Time, Color.R);
		ColorMap->FloatCurves[1].AddKey(Time, Color.G);
		ColorMap->FloatCurves[2].AddKey(Time, Color.B);
		ColorMap->FloatCurves[3].AddKey(Time, Color.A);
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

	NvFlowRenderMaterialParams FlowRenderMaterialParams;
	NvFlowRenderMaterialParamsDefaults(&FlowRenderMaterialParams);

	AlphaScale = FlowRenderMaterialParams.alphaScale;

	ColorMap = CreateDefaultSubobject<UCurveLinearColor>(TEXT("DefaultColorMap0"));
	AddColorMapPoint(ColorMap, 0.f, FLinearColor(0.0f, 0.f, 0.f, 0.f));
	AddColorMapPoint(ColorMap, 0.1f, FLinearColor(0.f, 0.f, 0.f, 0.25f));
	AddColorMapPoint(ColorMap, 0.6f, FLinearColor(1.f * 213.f / 255.f, 1.f * 100.f / 255.f, 1.f * 30.f / 255.f, 0.8f));
	AddColorMapPoint(ColorMap, 0.75f, FLinearColor(2.f * 1.27f, 2.f * 1.20f, 1.f * 0.39f, 0.8f));
	AddColorMapPoint(ColorMap, 0.85f, FLinearColor(4.f * 1.27f, 4.f * 1.20f, 1.f * 0.39f, 0.8f));
	AddColorMapPoint(ColorMap, 1.f, FLinearColor(8.0f, 8.0f, 8.0f, 0.7f));

#if WITH_EDITORONLY_DATA
	// invalidate
	ColorMap->AssetImportData = nullptr;
#endif

	ColorMapMinX = 0.f;
	ColorMapMaxX = 1.f;

}

// NvFlow end
