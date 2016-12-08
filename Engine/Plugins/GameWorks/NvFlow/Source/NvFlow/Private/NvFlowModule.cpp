#include "NvFlowPCH.h"

#include "GameWorks/RendererHooksNvFlow.h"

IMPLEMENT_MODULE( FNvFlowModule, NvFlow );
DEFINE_LOG_CATEGORY(LogNvFlow);

// NvFlow begin
#if WITH_NVFLOW
void NvFlowUpdateScene(FRHICommandListImmediate& RHICmdList, TArray<FPrimitiveSceneInfo*>& Primitives);
bool NvFlowDoRenderPrimitive(FRHICommandList& RHICmdList, const FViewInfo& View, FPrimitiveSceneInfo* PrimitiveSceneInfo);
void NvFlowDoRenderFinish(FRHICommandListImmediate& RHICmdList, const FViewInfo& View);
#endif
// NvFlow end

struct RendererHooksNvFlowImpl : public RendererHooksNvFlow
{
	virtual void NvFlowUpdateScene(FRHICommandListImmediate& RHICmdList, TArray<FPrimitiveSceneInfo*>& Primitives)
	{
		::NvFlowUpdateScene(RHICmdList, Primitives);
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

struct FNvFlowCommands
{
	FAutoConsoleCommand ConsoleCommandFlowVis;
	FAutoConsoleCommand ConsoleCommandFlowVisRenderChannel;
	FAutoConsoleCommand ConsoleCommandFlowVisRenderMode;
	FAutoConsoleCommand ConsoleCommandFlowVisMode;

	static const uint32 debugVisDefault = eNvFlowGridDebugVisBlocks | eNvFlowGridDebugVisEmitBounds | eNvFlowGridDebugVisShapesSimple;

	void CommandFlowVis(const TArray<FString>& Args)
	{
		UFlowGridAsset::sGlobalDebugDraw = !UFlowGridAsset::sGlobalDebugDraw;
		if (UFlowGridAsset::sGlobalDebugDraw)
		{
			// reset to defaults
			UFlowGridAsset::sGlobalRenderChannel = eNvFlowGridChannelDensity;
			UFlowGridAsset::sGlobalRenderMode = eNvFlowVolumeRenderMode_rainbow;
			UFlowGridAsset::sGlobalMode = debugVisDefault;
		}
	}

	void CommandFlowVisRenderChannel(const TArray<FString>& Args)
	{
		UFlowGridAsset::sGlobalDebugDraw = true;
		uint32 FlowVisMode = (Args.Num() >= 1) ? FCString::Atoi(*Args[0]) : eNvFlowGridChannelDensity;
		UFlowGridAsset::sGlobalRenderChannel = FMath::Clamp<uint32>(FlowVisMode, eNvFlowGridChannelVelocity, eNvFlowGridChannelCount - 1);
		if (UFlowGridAsset::sGlobalRenderChannel == eNvFlowGridChannelVelocity)
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
	LibName = TEXT("NvFlowD3D11Release_win64.dll");
	#else
	LibPath = FPaths::EngineDir() / TEXT("Plugins/GameWorks/NvFlow/Libraries/win32/");
	LibName = TEXT("NvFlowD3D11Release_win32.dll");
	#endif
#endif

	FPlatformProcess::PushDllDirectory(*LibPath);
	FlowModule = FPlatformProcess::GetDllHandle(*(LibPath + LibName));
	FPlatformProcess::PopDllDirectory(*LibPath);

	GRendererNvFlowHooks = &GRendererHooksNvFlowImpl;
}

void FNvFlowModule::ShutdownModule()
{
	if (FlowModule)
	{
		FPlatformProcess::FreeDllHandle(FlowModule);
		FlowModule = nullptr;
	}
}