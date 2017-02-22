#include "NvFlowPCH.h"

// NvFlow begin

#include "EnginePrivate.h"
#include "PhysicsEngine/PhysXSupport.h"
#include "Curves/CurveLinearColor.h"

#include "NvFlow.h"

bool UFlowGridAsset::sGlobalDebugDraw = false;
uint32 UFlowGridAsset::sGlobalRenderChannel = eNvFlowGridTextureChannelDensity;
uint32 UFlowGridAsset::sGlobalRenderMode = eNvFlowVolumeRenderMode_rainbow;
uint32 UFlowGridAsset::sGlobalMode = eNvFlowGridDebugVisBlocks;
bool UFlowGridAsset::sGlobalDebugDrawShadow = false;
uint32 UFlowGridAsset::sGlobalMultiGPU = 0;
bool UFlowGridAsset::sGlobalMultiGPUResetRequest = false;

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
	bLowLatencyMapping = true;
	bMultiAdapterEnabled = false;

	NvFlowGridParams FlowGridParams;
	NvFlowGridParamsDefaults(&FlowGridParams);

	Gravity = FVector(FlowGridParams.gravity.x, FlowGridParams.gravity.z, FlowGridParams.gravity.y) * GetFlowToUE4Scale();
	bPressureLegacyMode = FlowGridParams.pressureLegacyMode;
	bBigEffectMode = FlowGridParams.bigEffectMode;

	NvFlowVolumeRenderParams FlowVolumeRenderParams;
	NvFlowVolumeRenderParamsDefaults(&FlowVolumeRenderParams);
	RenderMode = (EFlowRenderMode)FlowVolumeRenderParams.renderMode;
	RenderChannel = (EFlowRenderChannel)FlowVolumeRenderParams.renderChannel;
	ColorMapResolution = 64;
	bAdaptiveScreenPercentage = false;
	AdaptiveTargetFrameTime = 10.f;
	MaxScreenPercentage = 1.f;
	MinScreenPercentage = 0.5f;
	bDebugWireframe = FlowVolumeRenderParams.debugMode;

	//Collision
	FCollisionResponseParams FlowResponseParams;
	int32 FlowChannelIdx = INDEX_NONE;
	// search engine trace channels for Flow
	for (int32 ChannelIdx = ECC_GameTraceChannel1; ChannelIdx <= ECC_GameTraceChannel18; ChannelIdx++)
	{
		if (FName(TEXT("Flow")) == UCollisionProfile::Get()->ReturnChannelNameFromContainerIndex(ChannelIdx))
		{
			FlowChannelIdx = ChannelIdx;
			break;
		}
	}
	if (FlowChannelIdx != INDEX_NONE)
	{
		ObjectType = (ECollisionChannel)FlowChannelIdx;
		FCollisionResponseTemplate Template;
		UCollisionProfile::Get()->GetProfileTemplate(UCollisionProfile::BlockAll_ProfileName, Template);
		ResponseToChannels = Template.ResponseToChannels;
	}
	else
	{
		ObjectType = ECC_WorldDynamic; /// ECC_Flow;
		FCollisionResponseTemplate Template;
		UCollisionProfile::Get()->GetProfileTemplate(TEXT("WorldDynamic"/*"Flow"*/), Template);
		ResponseToChannels = Template.ResponseToChannels;
	}

	bParticlesInteractionEnabled = false;
	InteractionChannel = EIC_Channel1;
	bParticleModeEnabled = false;

	ParticleToGridAccelTimeConstant = 0.01f;
	ParticleToGridDecelTimeConstant = 10.0f;
	ParticleToGridThresholdMultiplier = 2.f;
	GridToParticleAccelTimeConstant = 0.01f;
	GridToParticleDecelTimeConstant = 0.01f;
	GridToParticleThresholdMultiplier = 1.f;

	bDistanceFieldCollisionEnabled = false;
	MinActiveDistance = -1.0f;
	MaxActiveDistance = 0.0f;
	VelocitySlipFactor = 0.0f;
	VelocitySlipThickness = 0.0f;

	bVolumeShadowEnabled = false;
	ShadowIntensityScale = 0.5f;
	ShadowMinIntensity = 0.15f;

	ShadowBlendCompMask = { 0.0f, 0.0f, 0.0f, 0.0f };
	ShadowBlendBias = 1.0f;

	ShadowResolution = EFSR_High;
	ShadowFrustrumScale = 1.0f;
	ShadowMinResidentScale = 0.25f * (1.f / 64.f);
	ShadowMaxResidentScale = 4.f * 0.25f * (1.f / 64.f);
}

// NvFlow end