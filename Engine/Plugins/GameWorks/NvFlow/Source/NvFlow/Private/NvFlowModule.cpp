#include "NvFlowPCH.h"

#include "GameWorks/RendererHooksNvFlow.h"
#include "GameWorks/GridAccessHooksNvFlow.h"

IMPLEMENT_MODULE( FNvFlowModule, NvFlow );
DEFINE_LOG_CATEGORY(LogNvFlow);

// NvFlow begin
#if WITH_NVFLOW
bool NvFlowUsesGlobalDistanceField();
void NvFlowUpdateScene(FRHICommandListImmediate& RHICmdList, TArray<FPrimitiveSceneInfo*>& Primitives, const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData);
bool NvFlowDoRenderPrimitive(FRHICommandList& RHICmdList, const FViewInfo& View, FPrimitiveSceneInfo* PrimitiveSceneInfo);
void NvFlowDoRenderFinish(FRHICommandListImmediate& RHICmdList, const FViewInfo& View);
uint32 NvFlowQueryGridExportParams(FRHICommandListImmediate& RHICmdList, const ParticleSimulationParamsNvFlow& ParticleSimulationParams, uint32 MaxCount, GridExportParamsNvFlow* ResultParamsList);
#endif
// NvFlow end

struct RendererHooksNvFlowImpl : public RendererHooksNvFlow
{
	virtual bool NvFlowUsesGlobalDistanceField() const
	{
		return ::NvFlowUsesGlobalDistanceField();
	}

	virtual void NvFlowUpdateScene(FRHICommandListImmediate& RHICmdList, TArray<FPrimitiveSceneInfo*>& Primitives, const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData)
	{
		::NvFlowUpdateScene(RHICmdList, Primitives, GlobalDistanceFieldParameterData);
	}

	virtual bool NvFlowDoRenderPrimitive(FRHICommandList& RHICmdList, const FViewInfo& View, FPrimitiveSceneInfo* PrimitiveSceneInfo)
	{
		return ::NvFlowDoRenderPrimitive(RHICmdList, View, PrimitiveSceneInfo);
	}

	virtual void NvFlowDoRenderFinish(FRHICommandListImmediate& RHICmdList, const FViewInfo& View)
	{
		::NvFlowDoRenderFinish(RHICmdList, View);
	}
};
RendererHooksNvFlowImpl GRendererHooksNvFlowImpl;

struct GridAccessHooksNvFlowImpl : public GridAccessHooksNvFlow
{
	virtual uint32 NvFlowQueryGridExportParams(FRHICommandListImmediate& RHICmdList, const ParticleSimulationParamsNvFlow& ParticleSimulationParams, uint32 MaxCount, GridExportParamsNvFlow* ResultParamsList)
	{
		return ::NvFlowQueryGridExportParams(RHICmdList, ParticleSimulationParams, MaxCount, ResultParamsList);
	}
};
GridAccessHooksNvFlowImpl GGridAccessHooksNvFlowImpl;

struct FNvFlowCommands
{
	FAutoConsoleCommand ConsoleCommandFlowVis;
	FAutoConsoleCommand ConsoleCommandFlowVisRenderChannel;
	FAutoConsoleCommand ConsoleCommandFlowVisRenderMode;
	FAutoConsoleCommand ConsoleCommandFlowVisMode;
	FAutoConsoleCommand ConsoleCommandFlowVisShadow;
	FAutoConsoleCommand ConsoleCommandFlowMultiGPU;

	static const uint32 debugVisDefault = eNvFlowGridDebugVisBlocks | eNvFlowGridDebugVisEmitBounds | eNvFlowGridDebugVisShapesSimple;

	void CommandFlowVis(const TArray<FString>& Args)
	{
		UFlowGridAsset::sGlobalDebugDraw = !UFlowGridAsset::sGlobalDebugDraw;
		if (UFlowGridAsset::sGlobalDebugDraw)
		{
			// reset to defaults
			UFlowGridAsset::sGlobalRenderChannel = eNvFlowGridTextureChannelDensity;
			UFlowGridAsset::sGlobalRenderMode = eNvFlowVolumeRenderMode_rainbow;
			UFlowGridAsset::sGlobalMode = debugVisDefault;
		}
	}

	void CommandFlowVisRenderChannel(const TArray<FString>& Args)
	{
		UFlowGridAsset::sGlobalDebugDraw = true;
		uint32 FlowVisMode = (Args.Num() >= 1) ? FCString::Atoi(*Args[0]) : eNvFlowGridTextureChannelDensity;
		UFlowGridAsset::sGlobalRenderChannel = FMath::Clamp<uint32>(FlowVisMode, eNvFlowGridTextureChannelVelocity, eNvFlowGridTextureChannelCount - 1);
		if (UFlowGridAsset::sGlobalRenderChannel == eNvFlowGridTextureChannelVelocity)
		{
			UFlowGridAsset::sGlobalRenderMode = eNvFlowVolumeRenderMode_debug;
		}
		else 
		{
			UFlowGridAsset::sGlobalRenderMode = eNvFlowVolumeRenderMode_rainbow;
		}
	}

	void CommandFlowVisRenderMode(const TArray<FString>& Args)
	{
		UFlowGridAsset::sGlobalDebugDraw = true;
		uint32 FlowVisMode = (Args.Num() >= 1) ? FCString::Atoi(*Args[0]) : eNvFlowVolumeRenderMode_rainbow;
		UFlowGridAsset::sGlobalRenderMode = FMath::Clamp<uint32>(FlowVisMode, eNvFlowVolumeRenderMode_colormap, eNvFlowVolumeRenderModeCount - 1);
	}

	void CommandFlowVisMode(const TArray<FString>& Args)
	{
		UFlowGridAsset::sGlobalDebugDraw = true;
		uint32 FlowVisMode = (Args.Num() >= 1) ? FCString::Atoi(*Args[0]) : debugVisDefault;
		UFlowGridAsset::sGlobalMode = FlowVisMode;
	}

	void CommandFlowVisShadow(const TArray<FString>& Args)
	{
		UFlowGridAsset::sGlobalDebugDrawShadow = !UFlowGridAsset::sGlobalDebugDrawShadow;
	}

	void CommandFlowMultiGPU(const TArray<FString>& Args)
	{
		UFlowGridAsset::sGlobalMultiGPU = (Args.Num() >= 1) ? FCString::Atoi(*Args[0]) : ((UFlowGridAsset::sGlobalMultiGPU + 1) % 3);
		UFlowGridAsset::sGlobalMultiGPUResetRequest = true;
	}

	FNvFlowCommands() :
		ConsoleCommandFlowVis(
			TEXT("flowvis"),
			*NSLOCTEXT("Flow", "CommandText_FlowVis", "Enable/Disable Flow debug visualization").ToString(),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FNvFlowCommands::CommandFlowVis)
		),
		ConsoleCommandFlowVisRenderChannel(
			TEXT("flowvisrenderchannel"),
			*NSLOCTEXT("Flow", "CommandText_FlowVisRenderChannel", "Set Flow debug render channel").ToString(),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FNvFlowCommands::CommandFlowVisRenderChannel)
		),
		ConsoleCommandFlowVisRenderMode(
			TEXT("flowvisrendermode"),
			*NSLOCTEXT("Flow", "CommandText_FlowVisRenderMode", "Set Flow debug render mode").ToString(),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FNvFlowCommands::CommandFlowVisRenderMode)
		),
		ConsoleCommandFlowVisMode(
			TEXT("flowvismode"),
			*NSLOCTEXT("Flow", "CommandText_FlowVisMode", "Set Flow grid debug visualization mode").ToString(),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FNvFlowCommands::CommandFlowVisMode)
		),
		ConsoleCommandFlowVisShadow(
			TEXT("flowvisshadow"),
			*NSLOCTEXT("Flow", "CommandText_FlowVisShadow", "Enable/Disable Flow debug visualization for shadow").ToString(),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FNvFlowCommands::CommandFlowVisShadow)
		),
		ConsoleCommandFlowMultiGPU(
			TEXT("flowmultigpu"),
			*NSLOCTEXT("Flow", "CommandText_FlowMultiGPU", "Enable/Disable Flow multiGPU").ToString(),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FNvFlowCommands::CommandFlowMultiGPU)
		)
		{
	}
};

void FNvFlowModule::StartupModule()
{
	Commands = MakeUnique<FNvFlowCommands>();

	FlowModule = nullptr;

	FString LibPath;
	FString LibName;

#if PLATFORM_WINDOWS
	#if PLATFORM_64BITS
	LibPath = FPaths::EngineDir() / TEXT("Plugins/GameWorks/NvFlow/Libraries/win64/");
	LibName = TEXT("NvFlowLibRelease_win64.dll");
	#else
	LibPath = FPaths::EngineDir() / TEXT("Plugins/GameWorks/NvFlow/Libraries/win32/");
	LibName = TEXT("NvFlowLibRelease_win32.dll");
	#endif
#endif

	FPlatformProcess::PushDllDirectory(*LibPath);
	FlowModule = FPlatformProcess::GetDllHandle(*(LibPath + LibName));
	FPlatformProcess::PopDllDirectory(*LibPath);

	GRendererNvFlowHooks = &GRendererHooksNvFlowImpl;
	GGridAccessNvFlowHooks = &GGridAccessHooksNvFlowImpl;
}

void FNvFlowModule::ShutdownModule()
{
	if (FlowModule)
	{
		FPlatformProcess::FreeDllHandle(FlowModule);
		FlowModule = nullptr;
	}
}