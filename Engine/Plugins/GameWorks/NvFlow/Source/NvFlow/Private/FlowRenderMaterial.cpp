#include "NvFlowPCH.h"

// NvFlow begin

#include "EnginePrivate.h"

#include "NvFlow.h"

namespace
{
	inline void AddColorMapPoint(UCurveLinearColor* ColorMap, float Time, FLinearColor Color)
	{
		ColorMap->FloatCurves[0].AddKey(Time, Color.R);
		ColorMap->FloatCurves[1].AddKey(Time, Color.G);
		ColorMap->FloatCurves[2].AddKey(Time, Color.B);
		ColorMap->FloatCurves[3].AddKey(Time, Color.A);
	}

	inline FLinearColor ToLinearColor(const NvFlowFloat4& In)
	{
		return FLinearColor(In.x, In.y, In.z, In.w);
	}
}

UFlowRenderMaterial::UFlowRenderMaterial(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NvFlowRenderMaterialParams FlowRenderMaterialParams;
	NvFlowRenderMaterialParamsDefaults(&FlowRenderMaterialParams);

	AlphaScale = FlowRenderMaterialParams.alphaScale;
	AdditiveFactor = FlowRenderMaterialParams.additiveFactor;

	ColorMapCompMask = ToLinearColor(FlowRenderMaterialParams.colorMapCompMask);
	AlphaCompMask = ToLinearColor(FlowRenderMaterialParams.alphaCompMask);
	IntensityCompMask = ToLinearColor(FlowRenderMaterialParams.intensityCompMask);

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

	AlphaBias = FlowRenderMaterialParams.alphaBias;
	IntensityBias = FlowRenderMaterialParams.intensityBias;
}

// NvFlow end
