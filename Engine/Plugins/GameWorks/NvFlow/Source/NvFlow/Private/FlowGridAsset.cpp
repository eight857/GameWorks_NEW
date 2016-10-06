#include "NvFlowPCH.h"

// NvFlow begin

#include "EnginePrivate.h"
#include "PhysicsEngine/PhysXSupport.h"
#include "Curves/CurveLinearColor.h"

#include "NvFlow.h"

bool UFlowGridAsset::sGlobalDebugDraw = false;
uint32 UFlowGridAsset::sGlobalRenderingMode = eNvFlowVolumeRenderMode_densityRainbow;

namespace
{
	void AddColorMapPoint(UCurveLinearColor* ColorMap, float Time, FLinearColor Color)
	{
		ColorMap->FloatCurves[0].AddKey(Time, Color.R);
		ColorMap->FloatCurves[1].AddKey(Time, Color.G);
		ColorMap->FloatCurves[2].AddKey(Time, Color.B);
		ColorMap->FloatCurves[3].AddKey(Time, Color.A);
	}
}

UFlowGridAsset::UFlowGridAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	NvFlowGridDesc FlowGridDesc;
	NvFlowGridDescDefaults(&FlowGridDesc);

	GridCellSize = FlowGridDesc.halfSize.x * 2 * GetFlowToUE4Scale() / FlowGridDesc.virtualDim.x;

	check(FlowGridDesc.virtualDim.x == GetVirtualGridDimension(EFGD_512));
	VirtualGridDimension = EFGD_512;

	MemoryLimitScale = 1.f;

	SimulationRate = 60.f;
	bMultiAdapterEnabled = false;

	NvFlowGridParams FlowGridParams;
	NvFlowGridParamsDefaults(&FlowGridParams);

	VelocityWeight = FlowGridParams.velocityWeight;
	DensityWeight = FlowGridParams.densityWeight;
	TempWeight = FlowGridParams.tempWeight;
	FuelWeight = FlowGridParams.fuelWeight;

	VelocityThreshold = FlowGridParams.velocityThreshold;
	DensityThreshold = FlowGridParams.densityThreshold;
	TempThreshold = FlowGridParams.tempThreshold;
	FuelThreshold = FlowGridParams.fuelThreshold;

	ImportanceThreshold = FlowGridParams.importanceThreshold;

	Gravity = FVector(FlowGridParams.gravity.x, FlowGridParams.gravity.z, FlowGridParams.gravity.y) * GetFlowToUE4Scale();
	VelocityDamping = FlowGridParams.velocityDamping;
	DensityDamping = FlowGridParams.densityDamping;
	VelocityFade = FlowGridParams.velocityFade;
	DensityFade = FlowGridParams.densityFade;
	VelocityMacCormackBlendFactor = FlowGridParams.velocityMacCormackBlendFactor;
	DensityMacCormackBlendFactor = FlowGridParams.densityMacCormackBlendFactor;
	VorticityStrength = FlowGridParams.vorticityStrength;
	IgnitionTemperature = FlowGridParams.combustion.ignitionTemp;
	CoolingRate = FlowGridParams.combustion.coolingRate;

	NvFlowVolumeRenderParams FlowVolumeRenderParams;
	NvFlowVolumeRenderParamsDefaults(&FlowVolumeRenderParams);
	RenderingAlphaScale = FlowVolumeRenderParams.alphaScale;
	RenderingMode = FlowVolumeRenderParams.renderMode;
	bAdaptiveScreenPercentage = false;
	AdaptiveTargetFrameTime = 10.f;
	MaxScreenPercentage = 1.f;
	MinScreenPercentage = 0.5f;
	bDebugWireframe = FlowVolumeRenderParams.debugMode;

	ColorMap = CreateDefaultSubobject<UCurveLinearColor>(TEXT("DefaultColorMap0"), false);
	AddColorMapPoint(ColorMap, 0.f, FLinearColor(0.0f, 0.f, 0.f, 0.f));
	AddColorMapPoint(ColorMap, 0.1f, FLinearColor(0.f, 0.f, 0.f, 0.25f));
	AddColorMapPoint(ColorMap, 0.6f, FLinearColor(1.f * 213.f / 255.f, 1.f * 100.f / 255.f, 1.f * 30.f / 255.f, 0.8f));
	AddColorMapPoint(ColorMap, 0.75f, FLinearColor(2.f * 1.27f, 2.f * 1.20f, 1.f * 0.39f, 0.8f));
	AddColorMapPoint(ColorMap, 0.85f, FLinearColor(4.f * 1.27f, 4.f * 1.20f, 1.f * 0.39f, 0.8f));
	AddColorMapPoint(ColorMap, 1.f, FLinearColor(8.0f, 8.0f, 8.0f, 0.7f));

	ColorMapMinX = 0.f;
	ColorMapMaxX = 1.f;

	//Collision
	ObjectType = ECC_WorldDynamic; /// ECC_Flow;
	FCollisionResponseTemplate Template;
	UCollisionProfile::Get()->GetProfileTemplate(TEXT("WorldDynamic"/*"Flow"*/), Template);
	ResponseToChannels = Template.ResponseToChannels;
}

// NvFlow end