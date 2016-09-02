#include "NvFlowPCH.h"

#include "GameWorks/RendererHooksNvFlow.h"

IMPLEMENT_MODULE( FNvFlowModule, NvFlow );
DEFINE_LOG_CATEGORY(LogNvFlow);

// NvFlow begin
#if WITH_NVFLOW
void NvFlowUpdateScene(FRHICommandListImmediate& RHICmdList, TArray<FPrimitiveSceneInfo*>& Primitives);
void NvFlowDoRenderBegin(FRHICommandListImmediate& RHICmdList, const FViewInfo& View);
void NvFlowDoRenderPrimitive(FRHICommandList& RHICmdList, const FViewInfo& View, FPrimitiveSceneInfo* PrimitiveSceneInfo);
void NvFlowDoRenderEnd(FRHICommandListImmediate& RHICmdList, const FViewInfo& View);
#endif
// NvFlow end

struct RendererHooksNvFlowImpl : public RendererHooksNvFlow
{
	virtual void NvFlowUpdateScene(FRHICommandListImmediate& RHICmdList, TArray<FPrimitiveSceneInfo*>& Primitives)
	{
		::NvFlowUpdateScene(RHICmdList, Primitives);
	}

	virtual void NvFlowDoRenderBegin(FRHICommandListImmediate& RHICmdList, const FViewInfo& View)
	{
		::NvFlowDoRenderBegin(RHICmdList, View);
	}

	virtual void NvFlowDoRenderPrimitive(FRHICommandList& RHICmdList, const FViewInfo& View, FPrimitiveSceneInfo* PrimitiveSceneInfo)
	{
		::NvFlowDoRenderPrimitive(RHICmdList, View, PrimitiveSceneInfo);
	}

	virtual void NvFlowDoRenderEnd(FRHICommandListImmediate& RHICmdList, const FViewInfo& View)
	{
		::NvFlowDoRenderEnd(RHICmdList, View);
	}
};
RendererHooksNvFlowImpl GRendererHooksNvFlowImpl;

struct FNvFlowCommands
{
	FAutoConsoleCommand ConsoleCommandFlowVis;
	FAutoConsoleCommand ConsoleCommandFlowVisMode;

	void CommandFlowVis(const TArray<FString>& Args)
	{
		UFlowGridAsset::sGlobalDebugDraw = !UFlowGridAsset::sGlobalDebugDraw;
	}

	void CommandFlowVisMode(const TArray<FString>& Args)
	{
		uint32 FlowVisMode = FCString::Atoi(*Args[0]);
		UFlowGridAsset::sGlobalDebugDraw = true;
		UFlowGridAsset::sGlobalRenderingMode = FMath::Clamp<uint32>(FlowVisMode, 0, 5);
	}

	FNvFlowCommands() :
		ConsoleCommandFlowVis(
			TEXT("flowvis"),
			*NSLOCTEXT("Flow", "CommandText_FlowVis", "Enable/Disable Flow debug visualization").ToString(),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FNvFlowCommands::CommandFlowVis)
			),
		ConsoleCommandFlowVisMode(
			TEXT("flowvismode"),
			*NSLOCTEXT("Flow", "CommandText_FlowVisMode", "Set Flow debug visualization mode").ToString(),
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