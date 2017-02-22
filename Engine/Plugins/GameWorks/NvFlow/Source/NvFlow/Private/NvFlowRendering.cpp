#include "NvFlowPCH.h"

// NvFlow begin
#if WITH_NVFLOW

DECLARE_LOG_CATEGORY_EXTERN(LogFlow, Log, All);
DEFINE_LOG_CATEGORY(LogFlow);

/*=============================================================================
NFlowRendering.cpp: Translucent rendering implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "ScreenRendering.h"
#include "PostProcess/SceneFilterRendering.h"
#include "LightPropagationVolume.h"
#include "SceneUtils.h"
#include "HardwareInfo.h"

#include "Stats.h"
#include "GridAccessHooksNvFlow.h"

#include "StaticMeshResources.h"
#include "DistanceFieldAtlas.h"

#if NVFLOW_ADAPTIVE
// For HMD support
#include "IHeadMountedDisplay.h"
#endif

// For dedicate GPU support
#if WITH_CUDA_CONTEXT
#include "PhysicsPublic.h"
#endif

DEFINE_STAT(STAT_Flow_SimulateGrids);
DEFINE_STAT(STAT_Flow_RenderGrids);


namespace NvFlow
{
#if	STATS
	static TStatId NvFlowCreateStatId(const TCHAR* StatNameText, const TCHAR* StatDescText)
	{
		FName StatName(StatNameText);

		FStartupMessages::Get().AddMetadata(StatName, StatDescText,
			FStatGroup_STATGROUP_Flow::GetGroupName(),
			FStatGroup_STATGROUP_Flow::GetGroupCategory(),
			FStatGroup_STATGROUP_Flow::GetDescription(),
			false, EStatDataType::ST_int64, false);

		TStatId StatID = IStatGroupEnableManager::Get().GetHighPerformanceEnableForStat(StatName,
			FStatGroup_STATGROUP_Flow::GetGroupName(),
			FStatGroup_STATGROUP_Flow::GetGroupCategory(),
			FStatGroup_STATGROUP_Flow::DefaultEnable,
			false, EStatDataType::ST_int64, StatDescText, false);

		return StatID;
	}
#endif // STATS

	struct Scene;

	struct Context
	{
		Context() { }
		~Context() { release(); }

		FCriticalSection m_criticalSection;

		void init(FRHICommandListImmediate& RHICmdList);
		void conditionalInitMultiGPU(FRHICommandListImmediate& RHICmdList);
		void interopBegin(FRHICommandList& RHICmdList, bool computeOnly);
		void interopEnd(FRHICommandList& RHICmdList, bool computeOnly, bool shouldFlush);

		void updateGridView(FRHICommandListImmediate& RHICmdList);
		void renderScene(FRHICommandList& RHICmdList, const FViewInfo& View, FFlowGridSceneProxy* FlowGridSceneProxy);
		void release();

		void updateScene(FRHICommandListImmediate& RHICmdList, FFlowGridSceneProxy* FlowGridSceneProxy, bool& shouldFlush, const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData);

		TArray<Scene*> m_sceneList;
		TArray<Scene*> m_cleanupSceneList;

		NvFlowInterop* m_flowInterop = nullptr;
		NvFlowContext* m_renderContext = nullptr;
		NvFlowDepthStencilView* m_dsv = nullptr;
		NvFlowRenderTargetView* m_rtv = nullptr;

		int m_maxFramesInFlight = 3u;
		int m_framesInFlight = 0;

		NvFlowDevice* m_renderDevice = nullptr;
		NvFlowDevice* m_gridDevice = nullptr;
		NvFlowDeviceQueue* m_gridQueue = nullptr;
		NvFlowDeviceQueue* m_gridCopyQueue = nullptr;
		NvFlowDeviceQueue* m_renderCopyQueue = nullptr;
		NvFlowContext* m_gridContext = nullptr;
		NvFlowContext* m_gridCopyContext = nullptr;
		NvFlowContext* m_renderCopyContext = nullptr;

		struct SDFshapeData
		{
			const FDistanceFieldVolumeData* volumeData = nullptr;
			NvFlowShapeSDF* flowShapeSDF = nullptr;
		};
		TMap<UStaticMesh*, SDFshapeData> m_mapForShapeSDF;

		bool m_multiGPUSupported = false;
		bool m_enableMultiGPU = false;
		bool m_multiGPUActive = false;

#if STATS
		struct SceneStatData
		{
			TStatId statIdVelocityNumBlocks;
			TStatId statIdVelocityMaxBlocks;
			TStatId statIdDensityNumBlocks;
			TStatId statIdDensityMaxBlocks;
		};
		TMap<FName, SceneStatData> m_mapForSceneStats;
#endif

		// deferred mechanism for proper RHI command list support
		void initDeferred(IRHICommandContext* RHICmdCtx);
		void conditionalInitMultiGPUDeferred(IRHICommandContext* RHICmdCtx);
		void interopBeginDeferred(IRHICommandContext* RHICmdCtx, bool computeOnly);
		void interopEndDeferred(IRHICommandContext* RHICmdCtx, bool computeOnly, bool shouldFlush);
		void cleanupSceneListDeferred();

		struct InteropBeginEndParams
		{
			Context* context;
			bool computeOnly;
			bool shouldFlush;
		};

		static void initCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx);
		static void conditionalInitMultiGPUCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx);
		static void interopBeginCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx);
		static void interopEndCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx);
		static void cleanupSceneListCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx);
	};

	struct Scene
	{
		Scene() {}
		~Scene();

		void init(Context* context, FRHICommandListImmediate& RHICmdList, FFlowGridSceneProxy* InFlowGridSceneProxy);
		void release();
		void updateParameters(FRHICommandListImmediate& RHICmdList);

		void updateSubstep(FRHICommandListImmediate& RHICmdList, float dt, uint32 substep, uint32 numSubsteps, bool& shouldFlush, const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData);
		void finilizeUpdate(FRHICommandListImmediate& RHICmdList);

		void updateGridView(FRHICommandListImmediate& RHICmdList);
		void render(FRHICommandList& RHICmdList, const FViewInfo& View);

		bool getExportParams(FRHICommandListImmediate& RHICmdList, GridExportParamsNvFlow& OutParams);


		struct CallbackUserData
		{
			NvFlow::Scene* Scene;
			IRHICommandContext* RHICmdCtx;
			float DeltaTime;
			const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData;
		};

		static void sEmitCustomAllocCallback(void* userdata, const NvFlowGridEmitCustomAllocParams* params)
		{
			CallbackUserData* callbackUserData = (CallbackUserData*)userdata;
			callbackUserData->Scene->emitCustomAllocCallback(callbackUserData->RHICmdCtx, params, callbackUserData->GlobalDistanceFieldParameterData);
		}
		static void sEmitCustomEmitVelocityCallback(void* userdata, NvFlowUint* dataFrontIdx, const NvFlowGridEmitCustomEmitParams* params)
		{
			CallbackUserData* callbackUserData = (CallbackUserData*)userdata;
			callbackUserData->Scene->emitCustomEmitVelocityCallback(callbackUserData->RHICmdCtx, dataFrontIdx, params, callbackUserData->GlobalDistanceFieldParameterData, callbackUserData->DeltaTime);
		}
		static void sEmitCustomEmitDensityCallback(void* userdata, NvFlowUint* dataFrontIdx, const NvFlowGridEmitCustomEmitParams* params)
		{
			CallbackUserData* callbackUserData = (CallbackUserData*)userdata;
			callbackUserData->Scene->emitCustomEmitDensityCallback(callbackUserData->RHICmdCtx, dataFrontIdx, params, callbackUserData->GlobalDistanceFieldParameterData, callbackUserData->DeltaTime);
		}
		void emitCustomAllocCallback(IRHICommandContext* RHICmdCtx, const NvFlowGridEmitCustomAllocParams* params, const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData);
		void emitCustomEmitVelocityCallback(IRHICommandContext* RHICmdCtx, NvFlowUint* dataFrontIdx, const NvFlowGridEmitCustomEmitParams* params, const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData, float dt);
		void emitCustomEmitDensityCallback(IRHICommandContext* RHICmdCtx, NvFlowUint* dataFrontIdx, const NvFlowGridEmitCustomEmitParams* params, const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData, float dt);

		void applyDistanceField(IRHICommandContext* RHICmdCtx, NvFlowUint dataFrontIdx, const NvFlowGridEmitCustomEmitLayerParams& layerParams, const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData, float dt,
			FShaderResourceViewRHIRef DataInSRV, FUnorderedAccessViewRHIRef DataOutUAV, FShaderResourceViewRHIRef BlockListSRV, FShaderResourceViewRHIRef BlockTableSRV,
			float InSlipFactor = 0, float InSlipThickness = 0, FVector4 InEmitValue = FVector4(ForceInitToZero));

		bool m_multiAdapter = false;

#if NVFLOW_ADAPTIVE
		float m_frameTimeSum = 0.f;
		float m_frameTimeCount = 0.f;
		float m_frameTimeAverage = 0.f;
		float m_currentAdaptiveScale = -1.f;
#endif

		Context* m_context = nullptr;

		// Cache context pointers here, since some grid can be multi-GPU, some not
		NvFlowContext* m_renderContext = nullptr;
		NvFlowContext* m_gridContext = nullptr;
		NvFlowContext* m_gridCopyContext = nullptr;
		NvFlowContext* m_renderCopyContext = nullptr;

		NvFlowGrid* m_grid = nullptr;
		NvFlowGridProxy* m_gridProxy = nullptr;
		NvFlowVolumeRender* m_volumeRender = nullptr;
		NvFlowVolumeShadow* m_volumeShadow = nullptr;
		NvFlowRenderMaterialPool* m_renderMaterialPool = nullptr;

		float m_shadowMinResidentScale;
		float m_shadowMaxResidentScale;
		uint32 m_shadowResolution;

		NvFlowGridExport* m_gridExport4Render = nullptr;

		NvFlowGridDesc m_gridDesc;
		float m_diffGridCellSize = 0.f;
		FVector m_diffGridLocation = FVector(0.f);
		NvFlowGridParams m_gridParams;
		NvFlowVolumeRenderParams m_renderParams;

		FFlowGridSceneProxy* FlowGridSceneProxy = nullptr;

		TArray<ParticleSimulationParamsNvFlow> m_particleParamsArray;

		struct RenderMaterialData
		{
			enum
			{
				RELEASED = 2u,
				PENDING_RELEASE = 0u,
				CREATED = 1u
			};
			uint32 state = RELEASED;

			NvFlowRenderMaterialHandle renderMaterialHandle;
		};

		struct MaterialData
		{
			NvFlowGridMaterialHandle gridMaterialHandle;

			TMap<FlowRenderMaterialKeyType, RenderMaterialData> renderMaterialMap;
		};
		TMap<FlowMaterialKeyType, MaterialData> m_materialMap;

		const MaterialData& updateMaterial(FlowMaterialKeyType materialKey, const FFlowMaterialParams& materialParams);
		void updateShapeSDF(NvFlowShapeDesc& shapeDesc);

		// deferred mechanism for proper RHI command list support
		float m_updateSubstep_dt = 0.f;
#if STATS
		Context::SceneStatData m_stats;
#endif

		struct UpdateParams
		{
			Scene* Scene;
			const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData;
		};
		struct RenderParams
		{
			Scene* scene;
			NvFlowVolumeRenderParams volumeRenderParams;
		};

		void initDeferred(IRHICommandContext* RHICmdCtx);
		void updateParametersDeferred(IRHICommandContext* RHICmdCtx);
		void updateSubstepDeferred(IRHICommandContext* RHICmdCtx, UpdateParams* updateParams);
		void finilizeUpdateDeferred(IRHICommandContext* RHICmdCtx);
		void updateGridViewDeferred(IRHICommandContext* RHICmdCtx);
		void renderDeferred(IRHICommandContext* RHICmdCtx, RenderParams* renderParams);

		static void initCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx);
		static void updateParametersCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx);
		static void updateSubstepCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx);
		static void finilizeUpdateCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx);
		static void updateGridViewCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx);
		static void renderCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx);
	};

	void cleanupContext(void* ptr);
	void cleanupScene(void* ptr);

	Context gContextImpl;
	Context* gContext = nullptr;

	static uint32 getThreadID()
	{
		return FPlatformTLS::GetCurrentThreadId();
	}
}

// ------------------ NvFlow::Context -----------------

void NvFlow::cleanupContext(void* ptr)
{
	auto context = (Context*)ptr;
	if (context)
	{
		context->release();
	}
	gContext = nullptr;
}

void NvFlow::cleanupScene(void* ptr)
{
	auto scene = (Scene*)ptr;
	if (scene)
	{
		scene->m_context->m_criticalSection.Lock();

		TArray<Scene*>& sceneList = scene->m_context->m_sceneList;
		check(sceneList.Contains(scene));

		UE_LOG(LogFlow, Display, TEXT("NvFlow cleanup scene %p scheduled tid(%d)"), scene, getThreadID());

		scene->m_context->m_cleanupSceneList.Add(scene);

		sceneList.RemoveSingleSwap(scene);

		scene->m_context->m_criticalSection.Unlock();
	}
}

void NvFlow::Context::cleanupSceneListDeferred()
{
	m_criticalSection.Lock();
	for (int32 sceneIdx = 0u; sceneIdx < m_cleanupSceneList.Num(); sceneIdx++)
	{
		auto scene = m_cleanupSceneList[sceneIdx];

		UE_LOG(LogFlow, Display, TEXT("NvFlow cleanup scene %p executed tid(%d)"), scene, getThreadID());
		
		delete scene;
	}
	m_cleanupSceneList.Reset();
	m_criticalSection.Unlock();
}

void NvFlow::Context::cleanupSceneListCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx)
{
	auto context = (Context*)paramData;
	context->cleanupSceneListDeferred();
}

void NvFlow::Context::init(FRHICommandListImmediate& RHICmdList)
{
	UE_LOG(LogFlow, Display, TEXT("NvFlow Context Init"));

	RHICmdList.NvFlowWork(initCallback, this, 0u);
}

void NvFlow::Context::initCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx)
{
	auto context = (NvFlow::Context*)paramData;
	context->initDeferred(RHICmdCtx);
}

void NvFlow::Context::conditionalInitMultiGPU(FRHICommandListImmediate& RHICmdList)
{
	//UE_LOG(LogFlow, Display, TEXT("NvFlow Context MultiGPU init"));

	RHICmdList.NvFlowWork(conditionalInitMultiGPUCallback, this, 0u);
}

void NvFlow::Context::conditionalInitMultiGPUCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx)
{
	auto context = (NvFlow::Context*)paramData;
	context->conditionalInitMultiGPUDeferred(RHICmdCtx);
}

void NvFlow::Context::conditionalInitMultiGPUDeferred(IRHICommandContext* RHICmdCtx)
{
	auto& appctx = *RHICmdCtx;
	if (!m_multiGPUActive)
	{
		m_multiGPUActive = m_multiGPUSupported && (UFlowGridAsset::sGlobalMultiGPU > 0);
		if (m_multiGPUActive)
		{
			NvFlowDeviceDesc deviceDesc = {};
			NvFlowDeviceDescDefaults(&deviceDesc);

			deviceDesc.mode = eNvFlowDeviceModeProxy;
			m_renderDevice = NvFlowCreateDevice(m_renderContext, &deviceDesc);
			deviceDesc.mode = eNvFlowDeviceModeUnique;
			m_gridDevice = NvFlowCreateDevice(m_renderContext, &deviceDesc);

			NvFlowDeviceQueueDesc deviceQueueDesc = {};
			deviceQueueDesc.queueType = eNvFlowDeviceQueueTypeGraphics;
			deviceQueueDesc.lowLatency = false;
			m_gridQueue = NvFlowCreateDeviceQueue(m_gridDevice, &deviceQueueDesc);
			deviceQueueDesc.queueType = eNvFlowDeviceQueueTypeCopy;
			m_gridCopyQueue = NvFlowCreateDeviceQueue(m_gridDevice, &deviceQueueDesc);
			m_renderCopyQueue = NvFlowCreateDeviceQueue(m_renderDevice, &deviceQueueDesc);

			m_gridContext = NvFlowDeviceQueueCreateContext(m_gridQueue);
			m_gridCopyContext = NvFlowDeviceQueueCreateContext(m_gridCopyQueue);
			m_renderCopyContext = NvFlowDeviceQueueCreateContext(m_renderCopyQueue);

			NvFlowDeviceQueueStatus status = {};
			NvFlowDeviceQueueUpdateContext(m_gridQueue, m_gridContext, &status);
		}
		else
		{
			m_gridContext = m_renderContext;
			m_gridCopyContext = m_renderContext;
			m_renderCopyContext = m_renderContext;
		}
	}
}

void NvFlow::Context::initDeferred(IRHICommandContext* RHICmdCtx)
{
	auto& appctx = *RHICmdCtx;

	FString RHIName = TEXT("");
	{
		// Create the folder name based on the hardware specs we have been provided
		FString HardwareDetails = FHardwareInfo::GetHardwareDetailsString();
		FString RHILookup = NAME_RHI.ToString() + TEXT("=");
		FParse::Value(*HardwareDetails, *RHILookup, RHIName);
	}
	if (RHIName == TEXT("D3D11"))
	{
		m_flowInterop = NvFlowCreateInteropD3D11();
	}
	else
	if (RHIName == TEXT("D3D12"))
	{
		m_flowInterop = NvFlowCreateInteropD3D12();
	}
	else
	{
		UE_LOG(LogInit, Error, TEXT("Unsupported RHI type: %s"), *RHIName);
	}
	m_renderContext = m_flowInterop->CreateContext(appctx);

	// register cleanup
	m_flowInterop->CleanupFunc(appctx, cleanupContext, this);

	// create compute device if available
	// NVCHANGE: LRR - Check that the device is also dedicated to PhysX from the control panel
	bool bDedicatedPhysXGPU = true;
#if WITH_CUDA_CONTEXT
	NvIsPhysXHighSupported(bDedicatedPhysXGPU);
	UE_LOG(LogInit, Display, TEXT("NvFlow using dedicated PhysX GPU: %s"), bDedicatedPhysXGPU ? TEXT("true") : TEXT("false"));
#endif
	m_multiGPUSupported = NvFlowDedicatedDeviceAvailable(m_renderContext) && bDedicatedPhysXGPU;
	conditionalInitMultiGPUDeferred(RHICmdCtx);
}

void NvFlow::Context::interopBegin(FRHICommandList& RHICmdList, bool computeOnly)
{
	InteropBeginEndParams params = {};
	params.context = this;
	params.computeOnly = computeOnly;
	params.shouldFlush = false;
	RHICmdList.NvFlowWork(interopBeginCallback, &params, sizeof(params));
}

void NvFlow::Context::interopBeginCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx)
{
	auto params = (NvFlow::Context::InteropBeginEndParams*)paramData;
	params->context->interopBeginDeferred(RHICmdCtx, params->computeOnly);
}

void NvFlow::Context::interopBeginDeferred(IRHICommandContext* RHICmdCtx, bool computeOnly)
{
	auto& appctx = *RHICmdCtx;

	m_flowInterop->UpdateContext(appctx, m_renderContext);
	if (!computeOnly)
	{
		if (m_dsv == nullptr) m_dsv = m_flowInterop->CreateDepthStencilView(appctx, m_renderContext);
		if (m_rtv == nullptr) m_rtv = m_flowInterop->CreateRenderTargetView(appctx, m_renderContext);

		m_flowInterop->UpdateDepthStencilView(appctx, m_renderContext, m_dsv);
		m_flowInterop->UpdateRenderTargetView(appctx, m_renderContext, m_rtv);
	}

	if (m_gridDevice != m_renderDevice)
	{
		NvFlowDeviceQueueStatus status = {};
		NvFlowDeviceQueueUpdateContext(m_gridQueue, m_gridContext, &status);
		m_framesInFlight = status.framesInFlight;

		NvFlowDeviceQueueUpdateContext(m_gridCopyQueue, m_gridCopyContext, &status);
		NvFlowDeviceQueueUpdateContext(m_renderCopyQueue, m_renderCopyContext, &status);
	}

	m_flowInterop->Push(appctx, m_renderContext);
}

void NvFlow::Context::interopEnd(FRHICommandList& RHICmdList, bool computeOnly, bool shouldFlush)
{
	InteropBeginEndParams params = {};
	params.context = this;
	params.computeOnly = computeOnly;
	params.shouldFlush = shouldFlush;
	RHICmdList.NvFlowWork(interopEndCallback, &params, sizeof(params));
}

void NvFlow::Context::interopEndCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx)
{
	auto params = (NvFlow::Context::InteropBeginEndParams*)paramData;
	params->context->interopEndDeferred(RHICmdCtx, params->computeOnly, params->shouldFlush);
}

void NvFlow::Context::interopEndDeferred(IRHICommandContext* RHICmdCtx, bool computeOnly, bool shouldFlush)
{
	auto& appctx = *RHICmdCtx;

	if (computeOnly && m_gridDevice != m_renderDevice/* && shouldFlush*/)
	{
		NvFlowDeviceQueueConditionalFlush(m_gridQueue, m_gridContext);
		NvFlowDeviceQueueConditionalFlush(m_gridCopyQueue, m_gridCopyContext);
		NvFlowDeviceQueueConditionalFlush(m_renderCopyQueue, m_renderCopyContext);
	}

	m_flowInterop->Pop(appctx, m_renderContext);
}

void NvFlow::Context::updateGridView(FRHICommandListImmediate& RHICmdList)
{
	// iterate scenes and update grid view
	for (int32 i = 0; i < m_sceneList.Num(); i++)
	{
		Scene* scene = m_sceneList[i];
		scene->updateGridView(RHICmdList);
	}
}

void NvFlow::Context::renderScene(FRHICommandList& RHICmdList, const FViewInfo& View, FFlowGridSceneProxy* FlowGridSceneProxy)
{
	if (FlowGridSceneProxy->scenePtr != nullptr)
	{
		auto scene = (Scene*)FlowGridSceneProxy->scenePtr;
		scene->render(RHICmdList, View);
	}
}

void NvFlow::Context::release()
{
	cleanupSceneListDeferred();

	// proxies and scenes should all be released by now
	check(m_sceneList.Num() == 0);

	if (m_renderContext)
	{
		UE_LOG(LogFlow, Display, TEXT("NvFlow Context Cleanup"));
	}

	if (m_rtv) NvFlowReleaseRenderTargetView(m_rtv);
	if (m_dsv) NvFlowReleaseDepthStencilView(m_dsv);
	if (m_renderContext) NvFlowReleaseContext(m_renderContext);

	if (m_gridDevice != m_renderDevice)
	{
		NvFlowReleaseContext(m_gridContext);
		NvFlowReleaseContext(m_gridCopyContext);
		NvFlowReleaseContext(m_renderCopyContext);
		m_gridContext = nullptr;
		m_gridCopyContext = nullptr;
		m_renderCopyContext = nullptr;

		NvFlowReleaseDeviceQueue(m_gridQueue);
		NvFlowReleaseDeviceQueue(m_gridCopyQueue);
		NvFlowReleaseDeviceQueue(m_renderCopyQueue);
		m_gridQueue = nullptr;
		m_gridCopyQueue = nullptr;
		m_renderCopyQueue = nullptr;

		NvFlowReleaseDevice(m_gridDevice);
		NvFlowReleaseDevice(m_renderDevice);
		m_gridDevice = nullptr;
		m_renderDevice = nullptr;
	}
	else
	{
		m_gridContext = nullptr;
		m_gridCopyContext = nullptr;
		m_renderCopyContext = nullptr;

		m_gridQueue = nullptr;
		m_gridCopyQueue = nullptr;
		m_renderCopyQueue = nullptr;

		m_gridDevice = nullptr;
		m_renderDevice = nullptr;
	}

	if (m_flowInterop) NvFlowReleaseInterop(m_flowInterop);

	m_rtv = nullptr;
	m_dsv = nullptr;
	m_renderContext = nullptr;
	m_flowInterop = nullptr;

	NvFlowDeferredRelease(1000.f);
}

void NvFlow::Context::updateScene(FRHICommandListImmediate& RHICmdList, FFlowGridSceneProxy* FlowGridSceneProxy, bool& shouldFlush, const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData)
{
	// proxy not active, release scene if necessary and return
	if (!FlowGridSceneProxy->FlowGridProperties.bActive)
	{
		cleanupScene(FlowGridSceneProxy->scenePtr);
		FlowGridSceneProxy->scenePtr = nullptr;
		FlowGridSceneProxy->cleanupSceneFunc = nullptr;
		return;
	}

	// create scene if necessary
	if (FlowGridSceneProxy->scenePtr == nullptr)
	{
		Scene* newScene = new Scene();
		newScene->init(this, RHICmdList, FlowGridSceneProxy);
		m_sceneList.Add(newScene);
	}

	auto scene = (Scene*)FlowGridSceneProxy->scenePtr;

	scene->updateParameters(RHICmdList);

	// process simulation events
	if (FlowGridSceneProxy->FlowGridProperties.SubstepSize > 0.0f)
	{
		for (uint32 i = 0; i < uint32(FlowGridSceneProxy->NumScheduledSubsteps); i++)
		{
			scene->updateSubstep(RHICmdList, FlowGridSceneProxy->FlowGridProperties.SubstepSize, i, uint32(FlowGridSceneProxy->NumScheduledSubsteps), shouldFlush, GlobalDistanceFieldParameterData);
		}
	}
	FlowGridSceneProxy->NumScheduledSubsteps = 0;

	scene->finilizeUpdate(RHICmdList);
}

// ------------------ NvFlow::Scene -----------------

NvFlow::Scene::~Scene()
{
	release();
}

void NvFlow::Scene::release()
{
#if STATS
	FThreadStats::AddMessage(m_stats.statIdVelocityMaxBlocks.GetName(), EStatOperation::Clear, int64(0));
	FThreadStats::AddMessage(m_stats.statIdVelocityNumBlocks.GetName(), EStatOperation::Clear, int64(0));
	FThreadStats::AddMessage(m_stats.statIdDensityMaxBlocks.GetName(), EStatOperation::Clear, int64(0));
	FThreadStats::AddMessage(m_stats.statIdDensityNumBlocks.GetName(), EStatOperation::Clear, int64(0));
#endif

	if (m_context)
	{
		UE_LOG(LogFlow, Display, TEXT("NvFlow Scene %p Cleanup"), this);
	}

	if (m_grid) NvFlowReleaseGrid(m_grid);
	if (m_gridProxy) NvFlowReleaseGridProxy(m_gridProxy);
	if (m_volumeRender) NvFlowReleaseVolumeRender(m_volumeRender);
	if (m_volumeShadow) NvFlowReleaseVolumeShadow(m_volumeShadow);
	if (m_renderMaterialPool) NvFlowReleaseRenderMaterialPool(m_renderMaterialPool);

	m_grid = nullptr;
	m_gridProxy = nullptr;
	m_volumeRender = nullptr;
	m_volumeShadow = nullptr;
	m_renderMaterialPool = nullptr;

	m_context = nullptr;

	FlowGridSceneProxy = nullptr;
}

void NvFlow::Scene::init(Context* context, FRHICommandListImmediate& RHICmdList, FFlowGridSceneProxy* InFlowGridSceneProxy)
{
	UE_LOG(LogFlow, Display, TEXT("NvFlow Scene %p Init"), this);

	m_context = context;
	FlowGridSceneProxy = InFlowGridSceneProxy;
	FlowGridSceneProxy->scenePtr = this;
	FlowGridSceneProxy->cleanupSceneFunc = NvFlow::cleanupScene;

	RHICmdList.NvFlowWork(initCallback, this, 0u);

#if STATS
	const FName ownerName = InFlowGridSceneProxy->GetOwnerName();

	Context::SceneStatData* sceneStatData = m_context->m_mapForSceneStats.Find(ownerName);
	if (sceneStatData == nullptr)
	{
		int32 sceneId = m_context->m_mapForSceneStats.Num();

		sceneStatData = &m_context->m_mapForSceneStats.Add(ownerName);

		FString sceneName;
		InFlowGridSceneProxy->GetOwnerName().ToString(sceneName);

		sceneStatData->statIdVelocityNumBlocks = NvFlowCreateStatId(*FString::Printf(TEXT("STAT_Flow_%i_VelocityNumBlocks"), int32(sceneId)), *FString::Printf(TEXT("'%s' Velocity Num Blocks"), *sceneName));
		sceneStatData->statIdVelocityMaxBlocks = NvFlowCreateStatId(*FString::Printf(TEXT("STAT_Flow_%i_VelocityMaxBlocks"), int32(sceneId)), *FString::Printf(TEXT("'%s' Velocity Max Blocks"), *sceneName));

		sceneStatData->statIdDensityNumBlocks = NvFlowCreateStatId(*FString::Printf(TEXT("STAT_Flow_%i_DensityNumBlocks"), int32(sceneId)), *FString::Printf(TEXT("'%s' Density Num Blocks"), *sceneName));
		sceneStatData->statIdDensityMaxBlocks = NvFlowCreateStatId(*FString::Printf(TEXT("STAT_Flow_%i_DensityMaxBlocks"), int32(sceneId)), *FString::Printf(TEXT("'%s' Density Max Blocks"), *sceneName));
	}
	check(sceneStatData != nullptr)

	m_stats = *sceneStatData;
#endif
}

void NvFlow::Scene::initCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx)
{
	auto scene = (NvFlow::Scene*)paramData;
	scene->initDeferred(RHICmdCtx);
}

void NvFlow::Scene::initDeferred(IRHICommandContext* RHICmdCtx)
{
	auto& appctx = *RHICmdCtx;

	// create local grid desc copy
	m_gridDesc = FlowGridSceneProxy->FlowGridProperties.GridDesc;
	m_diffGridCellSize = FlowGridSceneProxy->FlowGridProperties.GridCellSize;
	m_diffGridLocation = FlowGridSceneProxy->GetLocalToWorld().GetOrigin();

	// set initial location using proxy location
	FVector FlowOrigin = FlowGridSceneProxy->GetLocalToWorld().GetOrigin() * scaleInv;
	m_gridDesc.initialLocation = *(NvFlowFloat3*)(&FlowOrigin.X);

	bool multiAdapterEnabled = FlowGridSceneProxy->FlowGridProperties.bMultiAdapterEnabled;
	m_multiAdapter = multiAdapterEnabled && m_context->m_multiGPUActive;
	if (UFlowGridAsset::sGlobalMultiGPU > 1)
	{
		m_multiAdapter = m_context->m_multiGPUActive;
	}

	if (m_multiAdapter)
	{
		m_renderContext = m_context->m_renderContext;
		m_gridContext = m_context->m_gridContext;
		m_gridCopyContext = m_context->m_gridCopyContext;
		m_renderCopyContext = m_context->m_renderCopyContext;
	}
	else
	{
		m_renderContext = m_context->m_renderContext;
		m_gridContext = m_context->m_renderContext;
		m_gridCopyContext = m_context->m_renderContext;
		m_renderCopyContext = m_context->m_renderContext;
	}

	m_grid = NvFlowCreateGrid(m_gridContext, &m_gridDesc);

	auto proxyGridExport = NvFlowGridGetGridExport(m_gridContext, m_grid);

	NvFlowGridProxyDesc proxyDesc = {};
	proxyDesc.gridContext = m_gridContext;
	proxyDesc.renderContext = m_renderContext;
	proxyDesc.gridCopyContext = m_gridCopyContext;
	proxyDesc.renderCopyContext = m_renderCopyContext;
	proxyDesc.gridExport = proxyGridExport;
	proxyDesc.proxyType = eNvFlowGridProxyTypePassThrough;
	if (m_multiAdapter)
	{
		proxyDesc.proxyType = eNvFlowGridProxyTypeMultiGPU;
	}

	m_gridProxy = NvFlowCreateGridProxy(&proxyDesc);

	NvFlowVolumeRenderDesc volumeRenderDesc;
	volumeRenderDesc.gridExport = NvFlowGridProxyGetGridExport(m_gridProxy, m_renderContext);

	m_volumeRender = NvFlowCreateVolumeRender(m_renderContext, &volumeRenderDesc);

	NvFlowRenderMaterialPoolDesc renderMaterialPoolDesc;
	renderMaterialPoolDesc.colorMapResolution = FlowGridSceneProxy->FlowGridProperties.ColorMapResolution;

	m_renderMaterialPool = NvFlowCreateRenderMaterialPool(m_renderContext, &renderMaterialPoolDesc);
}

void NvFlow::Scene::updateParameters(FRHICommandListImmediate& RHICmdList)
{
	auto& appctx = RHICmdList.GetContext();

	const FFlowGridProperties& Properties = FlowGridSceneProxy->FlowGridProperties;

	// configure grid params
	m_gridParams = Properties.GridParams;

	// configure render params
	NvFlowVolumeRenderParamsDefaults(&m_renderParams);
	m_renderParams.renderMode = Properties.RenderParams.RenderMode;
	m_renderParams.renderChannel = Properties.RenderParams.RenderChannel;
	m_renderParams.debugMode = Properties.RenderParams.bDebugWireframe;
	m_renderParams.materialPool = m_renderMaterialPool;

#if NVFLOW_ADAPTIVE
	// adaptive screen percentage
	bool bHMDConnected = (GEngine && GEngine->HMDDevice.IsValid() && GEngine->HMDDevice->IsHMDConnected());
	if (Properties.RenderParams.bAdaptiveScreenPercentage && bHMDConnected)
	{
		const float decayRate = 0.98f;
		const float reactRate = 0.002f;
		const float recoverRate = 0.001f;

		if (m_currentAdaptiveScale < 0.f) m_currentAdaptiveScale = Properties.RenderParams.MaxScreenPercentage;

		//float lastFrameTime = FPlatformTime::ToMilliseconds(GGPUFrameTime);

		float lastFrameTime = 1000.f * float(FApp::GetCurrentTime() - FApp::GetLastTime());
		if (GEngine)
		{
			if (GEngine->HMDDevice.Get())
			{
				float timing = GEngine->HMDDevice->GetFrameTiming();
				if (timing > 1.f)
				{
					lastFrameTime = timing;
				}
			}
		}

		float targetFrameTime = Properties.RenderParams.AdaptiveTargetFrameTime;

		m_frameTimeSum += lastFrameTime;
		m_frameTimeCount += 1.f;
		m_frameTimeSum *= decayRate;
		m_frameTimeCount *= decayRate;

		m_frameTimeAverage = float(m_frameTimeSum) / float(m_frameTimeCount);

		float error = m_frameTimeAverage - targetFrameTime;
		if (error > 0.f)
		{
			m_currentAdaptiveScale -= reactRate;
		}
		else if (error < 0.f)
		{
			m_currentAdaptiveScale += recoverRate;
		}

		// enforce clamps
		if (m_currentAdaptiveScale < Properties.RenderParams.MinScreenPercentage)
		{
			m_currentAdaptiveScale = Properties.RenderParams.MinScreenPercentage;
		}
		if (m_currentAdaptiveScale > Properties.RenderParams.MaxScreenPercentage)
		{
			m_currentAdaptiveScale = Properties.RenderParams.MaxScreenPercentage;
		}

		// ScreenPercentage > 1.0 not supported (would require reallocation)
		if (m_currentAdaptiveScale > 1.f)
		{
			m_currentAdaptiveScale = 1.f;
		}

		m_renderParams.screenPercentage = m_currentAdaptiveScale;
	}
	else
#endif
	{
		m_renderParams.screenPercentage = Properties.RenderParams.MaxScreenPercentage;
	}

	// deferred parameter updates
	RHICmdList.NvFlowWork(updateParametersCallback, this, 0u);
}

void NvFlow::Scene::updateParametersCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx)
{
	auto scene = (NvFlow::Scene*)paramData;
	scene->updateParametersDeferred(RHICmdCtx);
}

void NvFlow::Scene::updateParametersDeferred(IRHICommandContext* RHICmdCtx)
{
	auto& appctx = *RHICmdCtx;

	FFlowGridProperties& Properties = FlowGridSceneProxy->FlowGridProperties;

	for (auto It = Properties.Materials.CreateConstIterator(); It; ++It)
	{
		updateMaterial(It->Key, It->Value);
	}

	check(Properties.GridEmitParams.Num() == Properties.GridEmitMaterialKeys.Num());
	for (int32 i = 0; i < Properties.GridEmitParams.Num(); ++i)
	{
		NvFlowGridMaterialHandle gridMaterialHandle = { nullptr, 0 };

		FlowMaterialKeyType materialKey = Properties.GridEmitMaterialKeys[i];
		if (materialKey != nullptr)
		{
			MaterialData* materialData = m_materialMap.Find(materialKey);
			if (materialData != nullptr)
			{
				gridMaterialHandle = materialData->gridMaterialHandle;
			}
		}

		Properties.GridEmitParams[i].material = gridMaterialHandle;

		if (Properties.GridEmitParams[i].shapeType == eNvFlowShapeTypeSDF)
		{
			updateShapeSDF(Properties.GridEmitShapeDescs[Properties.GridEmitParams[i].shapeRangeOffset]);
		}
	}

	// update SDF shapes
	for (int32 i = 0; i < Properties.GridCollideParams.Num(); ++i)
	{
		if (Properties.GridCollideParams[i].shapeType == eNvFlowShapeTypeSDF)
		{
			updateShapeSDF(Properties.GridCollideShapeDescs[Properties.GridCollideParams[i].shapeRangeOffset]);
		}
	}
}

void NvFlow::Scene::updateShapeSDF(NvFlowShapeDesc& shapeDesc)
{
	check(shapeDesc.sdf.sdf != nullptr);

	UStaticMesh* StaticMesh = reinterpret_cast<UStaticMesh*>(shapeDesc.sdf.sdf);
	const FDistanceFieldVolumeData* DistanceFieldVolumeData = StaticMesh->RenderData->LODResources[0].DistanceFieldData;
	check(DistanceFieldVolumeData != nullptr);

	Context::SDFshapeData& shapeData = m_context->m_mapForShapeSDF.FindOrAdd(StaticMesh);
	if (shapeData.volumeData != DistanceFieldVolumeData)
	{
		shapeData.volumeData = DistanceFieldVolumeData;

		if (shapeData.flowShapeSDF != nullptr)
		{
			NvFlowReleaseShapeSDF(shapeData.flowShapeSDF);
		}

		//create new NvFlowShapeSDF
		NvFlowShapeSDFDesc descSDF;
		descSDF.resolution.x = DistanceFieldVolumeData->Size.X;
		descSDF.resolution.y = DistanceFieldVolumeData->Size.Y;
		descSDF.resolution.z = DistanceFieldVolumeData->Size.Z;

		shapeData.flowShapeSDF = NvFlowCreateShapeSDF(m_gridContext, &descSDF);

		NvFlowShapeSDFData mappedShapeData = NvFlowShapeSDFMap(shapeData.flowShapeSDF, m_gridContext);

		check(mappedShapeData.dim.x == descSDF.resolution.x);
		check(mappedShapeData.dim.y == descSDF.resolution.y);
		check(mappedShapeData.dim.z == descSDF.resolution.z);
		for (NvFlowUint z = 0; z < mappedShapeData.dim.z; ++z)
		{
			for (NvFlowUint y = 0; y < mappedShapeData.dim.y; ++y)
			{
				for (NvFlowUint x = 0; x < mappedShapeData.dim.x; ++x)
				{
					FFloat16 valueF16 = DistanceFieldVolumeData->DistanceFieldVolume[x + mappedShapeData.dim.x * (y + mappedShapeData.dim.y * z)];
					float value = valueF16.GetFloat();

					mappedShapeData.data[x + mappedShapeData.rowPitch * y + mappedShapeData.depthPitch * z] = value;
				}
			}
		}

		NvFlowShapeSDFUnmap(shapeData.flowShapeSDF, m_gridContext);
	}
	check(shapeData.flowShapeSDF != nullptr);

	shapeDesc.sdf.sdf = shapeData.flowShapeSDF;
}


const NvFlow::Scene::MaterialData& NvFlow::Scene::updateMaterial(FlowMaterialKeyType materialKey, const FFlowMaterialParams& materialParams)
{
	MaterialData* materialData = m_materialMap.Find(materialKey);
	if (materialData == nullptr)
	{
		materialData = &m_materialMap.Add(materialKey);

		materialData->gridMaterialHandle = NvFlowGridCreateMaterial(m_grid, &materialParams.GridParams);
	}
	else
	{
		//TODO: add dirty check
		NvFlowGridSetMaterialParams(m_grid, materialData->gridMaterialHandle, &materialParams.GridParams);
	}

	for (auto It = materialData->renderMaterialMap.CreateIterator(); It; ++It)
	{
		RenderMaterialData& renderMaterialData = It.Value();
		renderMaterialData.state &= ~RenderMaterialData::CREATED; // CREATED -> PENDING_RELEASE
	}

	for (auto It = materialParams.RenderMaterials.CreateConstIterator(); It; ++It)
	{
		const FFlowRenderMaterialParams& renderMaterialParams = *It;

		NvFlowRenderMaterialParams renderMaterialParamsCopy = renderMaterialParams;
		renderMaterialParamsCopy.material = materialData->gridMaterialHandle;

		RenderMaterialData& renderMaterialData = materialData->renderMaterialMap.FindOrAdd(renderMaterialParams.Key);
		if (renderMaterialData.state == RenderMaterialData::RELEASED)
		{
			renderMaterialData.renderMaterialHandle = NvFlowCreateRenderMaterial(m_renderContext, m_renderMaterialPool, &renderMaterialParamsCopy);
		}
		else
		{
			NvFlowRenderMaterialUpdate(renderMaterialData.renderMaterialHandle, &renderMaterialParamsCopy);
		}
		renderMaterialData.state = RenderMaterialData::CREATED;

		// update color map
		{
			//TODO: add dirty check
			auto mapped = NvFlowRenderMaterialColorMap(m_renderContext, renderMaterialData.renderMaterialHandle);
			if (mapped.data)
			{
				check(mapped.dim == renderMaterialParams.ColorMap.Num());
				FMemory::Memcpy(mapped.data, renderMaterialParams.ColorMap.GetData(), sizeof(NvFlowFloat4)*mapped.dim);
				NvFlowRenderMaterialColorUnmap(m_renderContext, renderMaterialData.renderMaterialHandle);
			}
		}
	}

	for (auto It = materialData->renderMaterialMap.CreateIterator(); It; ++It)
	{
		RenderMaterialData& renderMaterialData = It.Value();
		if (renderMaterialData.state == RenderMaterialData::PENDING_RELEASE)
		{
			NvFlowReleaseRenderMaterial(renderMaterialData.renderMaterialHandle);

			renderMaterialData.state = RenderMaterialData::RELEASED;
		}
	}

	return *materialData;
}


void NvFlow::Scene::updateSubstep(FRHICommandListImmediate& RHICmdList, float dt, uint32 substep, uint32 numSubsteps, bool& shouldFlush, const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData)
{
	bool shouldUpdateGrid = (m_context->m_framesInFlight < m_context->m_maxFramesInFlight);

	if (shouldUpdateGrid)
	{
		NvFlowContextFlushRequestPush(m_context->m_gridContext);
		NvFlowContextFlushRequestPush(m_context->m_gridCopyContext);
		NvFlowContextFlushRequestPush(m_context->m_renderCopyContext);
	}

	shouldFlush = shouldFlush || (m_multiAdapter && shouldUpdateGrid);

	m_updateSubstep_dt = dt;

	if (shouldUpdateGrid)
	{
		// Do this on the stack for thread safety
		UpdateParams updateParams = {};
		updateParams.Scene = this;
		updateParams.GlobalDistanceFieldParameterData = GlobalDistanceFieldParameterData;

		RHICmdList.NvFlowWork(updateSubstepCallback, &updateParams, sizeof(updateParams));
	}
}

void NvFlow::Scene::updateSubstepDeferred(IRHICommandContext* RHICmdCtx, UpdateParams* updateParams)
{
	auto& appctx = *RHICmdCtx;

	float dt = m_updateSubstep_dt;

	NvFlowGridSetParams(m_grid, &m_gridParams);

	FFlowGridProperties& Properties = FlowGridSceneProxy->FlowGridProperties;

	// update emitters
	{
		// emit
		NvFlowGridEmit(
			m_grid,
			Properties.GridEmitShapeDescs.GetData(),
			Properties.GridEmitShapeDescs.Num(),
			Properties.GridEmitParams.GetData(),
			Properties.GridEmitParams.Num()
			);

		// collide
		NvFlowGridEmit(
			m_grid,
			Properties.GridCollideShapeDescs.GetData(),
			Properties.GridCollideShapeDescs.Num(),
			Properties.GridCollideParams.GetData(),
			Properties.GridCollideParams.Num()
			);
	}

	{
		CallbackUserData callbackUserData;
		callbackUserData.Scene = this;
		callbackUserData.RHICmdCtx = RHICmdCtx;
		callbackUserData.DeltaTime = dt;
		callbackUserData.GlobalDistanceFieldParameterData = updateParams->GlobalDistanceFieldParameterData;

		NvFlowGridEmitCustomRegisterAllocFunc(m_grid, &NvFlow::Scene::sEmitCustomAllocCallback, &callbackUserData);
		NvFlowGridEmitCustomRegisterEmitFunc(m_grid, eNvFlowGridTextureChannelVelocity, &NvFlow::Scene::sEmitCustomEmitVelocityCallback, &callbackUserData);
		NvFlowGridEmitCustomRegisterEmitFunc(m_grid, eNvFlowGridTextureChannelDensity, &NvFlow::Scene::sEmitCustomEmitDensityCallback, &callbackUserData);

		// check for grid location or halfSize change
		{
			FVector newDiffGridLocation = FlowGridSceneProxy->GetLocalToWorld().GetOrigin();
			float newDiffGridCellSize = FlowGridSceneProxy->FlowGridProperties.GridCellSize;

			bool changed = (
				m_diffGridLocation.X != newDiffGridLocation.X ||
				m_diffGridLocation.Y != newDiffGridLocation.Y ||
				m_diffGridLocation.Z != newDiffGridLocation.Z ||
				m_diffGridCellSize != newDiffGridCellSize
				);

			m_diffGridLocation.X = newDiffGridLocation.X;
			m_diffGridLocation.Y = newDiffGridLocation.Y;
			m_diffGridLocation.Z = newDiffGridLocation.Z;
			m_diffGridCellSize = newDiffGridCellSize;

			if (changed)
			{
				FVector FlowOrigin = FlowGridSceneProxy->GetLocalToWorld().GetOrigin() * scaleInv;

				NvFlowGridResetDesc resetDesc = {};
				resetDesc.initialLocation = *(NvFlowFloat3*)(&FlowOrigin.X);
				resetDesc.halfSize = FlowGridSceneProxy->FlowGridProperties.GridDesc.halfSize;
			
				NvFlowGridReset(m_grid, &resetDesc);

				m_gridDesc.halfSize = resetDesc.halfSize;
				m_gridDesc.initialLocation = resetDesc.initialLocation;
			}
		}

		NvFlowGridUpdate(m_grid, m_gridContext, dt);

		NvFlowGridEmitCustomRegisterAllocFunc(m_grid, nullptr, nullptr);
		NvFlowGridEmitCustomRegisterEmitFunc(m_grid, eNvFlowGridTextureChannelVelocity, nullptr, nullptr);
		NvFlowGridEmitCustomRegisterEmitFunc(m_grid, eNvFlowGridTextureChannelDensity, nullptr, nullptr);
	}

	auto gridExport = NvFlowGridGetGridExport(m_gridContext, m_grid);

	NvFlowGridProxyFlushParams flushParams = {};
	flushParams.gridContext = m_gridContext;
	flushParams.gridCopyContext = m_gridCopyContext;
	flushParams.renderCopyContext = m_renderCopyContext;
	NvFlowGridProxyPush(m_gridProxy, gridExport, &flushParams);
}

void NvFlow::Scene::updateSubstepCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx)
{
	auto updateParams = (UpdateParams*)paramData;
	updateParams->Scene->updateSubstepDeferred(RHICmdCtx, updateParams);
}

void NvFlow::Scene::finilizeUpdate(FRHICommandListImmediate& RHICmdList)
{
	RHICmdList.NvFlowWork(finilizeUpdateCallback, this, 0u);
}

void NvFlow::Scene::finilizeUpdateCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx)
{
	auto scene = (NvFlow::Scene*)paramData;
	scene->finilizeUpdateDeferred(RHICmdCtx);
}

void NvFlow::Scene::finilizeUpdateDeferred(IRHICommandContext* RHICmdCtx)
{
	m_particleParamsArray.Reset();
}

void NvFlow::Scene::updateGridView(FRHICommandListImmediate& RHICmdList)
{
	RHICmdList.NvFlowWork(updateGridViewCallback, this, 0u);
}

void NvFlow::Scene::updateGridViewCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx)
{
	auto scene = (NvFlow::Scene*)paramData;
	scene->updateGridViewDeferred(RHICmdCtx);
}

void NvFlow::Scene::updateGridViewDeferred(IRHICommandContext* RHICmdCtx)
{
	NvFlowGridProxyFlushParams flushParams = {};
	flushParams.gridContext = m_gridContext;
	flushParams.gridCopyContext = m_gridCopyContext;
	flushParams.renderCopyContext = m_renderCopyContext;

	NvFlowGridProxyFlush(m_gridProxy, &flushParams);

	FFlowGridProperties& Properties = FlowGridSceneProxy->FlowGridProperties;

	m_gridExport4Render = NvFlowGridProxyGetGridExport(m_gridProxy, m_renderContext);

#if STATS
	if (FThreadStats::IsCollectingData(m_stats.statIdVelocityMaxBlocks))
	{
		NvFlowGridExportHandle gridExportHandle = NvFlowGridExportGetHandle(m_gridExport4Render, m_renderContext, eNvFlowGridTextureChannelVelocity);
		NvFlowGridExportLayeredView gridExportLayeredView;
		NvFlowGridExportGetLayeredView(gridExportHandle, &gridExportLayeredView);

		FThreadStats::AddMessage(m_stats.statIdVelocityMaxBlocks.GetName(), EStatOperation::Set, int64(gridExportLayeredView.mapping.maxBlocks));
		FThreadStats::AddMessage(m_stats.statIdVelocityNumBlocks.GetName(), EStatOperation::Set, int64(gridExportLayeredView.mapping.layeredNumBlocks));
	}
	if (FThreadStats::IsCollectingData(m_stats.statIdDensityMaxBlocks))
	{
		NvFlowGridExportHandle gridExportHandle = NvFlowGridExportGetHandle(m_gridExport4Render, m_renderContext, eNvFlowGridTextureChannelDensity);
		NvFlowGridExportLayeredView gridExportLayeredView;
		NvFlowGridExportGetLayeredView(gridExportHandle, &gridExportLayeredView);

		FThreadStats::AddMessage(m_stats.statIdDensityMaxBlocks.GetName(), EStatOperation::Set, int64(gridExportLayeredView.mapping.maxBlocks));
		FThreadStats::AddMessage(m_stats.statIdDensityNumBlocks.GetName(), EStatOperation::Set, int64(gridExportLayeredView.mapping.layeredNumBlocks));
	}
#endif

	FLightSceneInfo* DirectionalLight = nullptr;

	auto RenderScene = FlowGridSceneProxy->GetScene().GetRenderScene();
	if (RenderScene)
	{
		DirectionalLight = RenderScene->SimpleDirectionalLight;
	}


	if (Properties.RenderParams.bVolumeShadowEnabled && DirectionalLight != nullptr)
	{
		if (m_volumeShadow == nullptr || 
			m_shadowResolution != Properties.RenderParams.ShadowResolution ||
			m_shadowMinResidentScale != Properties.RenderParams.ShadowMinResidentScale ||
			m_shadowMaxResidentScale != Properties.RenderParams.ShadowMaxResidentScale)
		{
			if (m_volumeShadow != nullptr)
			{
				NvFlowReleaseVolumeShadow(m_volumeShadow);
			}

			NvFlowVolumeShadowDesc volumeShadowDesc;
			volumeShadowDesc.gridExport = m_gridExport4Render;
			volumeShadowDesc.mapWidth = Properties.RenderParams.ShadowResolution;
			volumeShadowDesc.mapHeight = Properties.RenderParams.ShadowResolution;
			volumeShadowDesc.mapDepth = Properties.RenderParams.ShadowResolution;
			volumeShadowDesc.minResidentScale = Properties.RenderParams.ShadowMinResidentScale;
			volumeShadowDesc.maxResidentScale = Properties.RenderParams.ShadowMaxResidentScale;

			m_volumeShadow = NvFlowCreateVolumeShadow(m_renderContext, &volumeShadowDesc);

			m_shadowResolution = Properties.RenderParams.ShadowResolution;
			m_shadowMinResidentScale = Properties.RenderParams.ShadowMinResidentScale;
			m_shadowMaxResidentScale = Properties.RenderParams.ShadowMaxResidentScale;
		}

		NvFlowVolumeShadowParams shadowParams;
		shadowParams.materialPool = m_renderMaterialPool;
		shadowParams.renderMode = Properties.RenderParams.RenderMode;
		shadowParams.renderChannel = Properties.RenderParams.RenderChannel;
		shadowParams.intensityScale = Properties.RenderParams.ShadowIntensityScale;
		shadowParams.minIntensity = Properties.RenderParams.ShadowMinIntensity;
		shadowParams.shadowBlendCompMask = Properties.RenderParams.ShadowBlendCompMask;
		shadowParams.shadowBlendBias = Properties.RenderParams.ShadowBlendBias;

		FMatrix ShadowViewMatrix = DirectionalLight->Proxy->GetWorldToLight();
		ShadowViewMatrix *= FMatrix(
			FPlane(0, 0, 1, 0),
			FPlane(1, 0, 0, 0),
			FPlane(0, 1, 0, 0),
			FPlane(0, 0, 0, 1));

		FBox BoundBox = FlowGridSceneProxy->GetBounds().GetBox();
		BoundBox.Min *= (1.0f / scale);
		BoundBox.Max *= (1.0f / scale);

		//set view origin to the center of bounding box
		ShadowViewMatrix.SetOrigin( -FVector(ShadowViewMatrix.TransformVector(BoundBox.GetCenter())) );

		FVector Extent = BoundBox.GetExtent();

		float ExtentX = FVector::DotProduct(ShadowViewMatrix.GetColumn(0).GetAbs(), Extent) * Properties.RenderParams.ShadowFrustrumScale;
		float ExtentY = FVector::DotProduct(ShadowViewMatrix.GetColumn(1).GetAbs(), Extent) * Properties.RenderParams.ShadowFrustrumScale;
		float ExtentZ = FVector::DotProduct(ShadowViewMatrix.GetColumn(2).GetAbs(), Extent) * Properties.RenderParams.ShadowFrustrumScale;

		FMatrix ShadowProjMatrix = FMatrix::Identity;
		ShadowProjMatrix.M[0][0] = 1.0f / ExtentX;
		ShadowProjMatrix.M[1][1] = 1.0f / ExtentY;
		ShadowProjMatrix.M[2][2] = 0.5f / ExtentZ;
		ShadowProjMatrix.M[3][2] = 0.5f;

		memcpy(&shadowParams.projectionMatrix, &ShadowProjMatrix.M[0][0], sizeof(shadowParams.projectionMatrix));
		memcpy(&shadowParams.viewMatrix, &ShadowViewMatrix.M[0][0], sizeof(shadowParams.viewMatrix));

		NvFlowVolumeShadowUpdate(m_volumeShadow, m_renderContext, m_gridExport4Render, &shadowParams);

		m_gridExport4Render = NvFlowVolumeShadowGetGridExport(m_volumeShadow, m_renderContext);
	}
	else
	{
		if (m_volumeShadow)
		{
			NvFlowReleaseVolumeShadow(m_volumeShadow);
			m_volumeShadow = nullptr;
		}
	}
}

void NvFlow::Scene::render(FRHICommandList& RHICmdList, const FViewInfo& View)
{
	FMatrix viewMatrix = View.ViewMatrices.GetViewMatrix();
	FMatrix projMatrix = View.ViewMatrices.GetProjectionMatrix();

	// Do this on the stack for thread safety
	RenderParams renderParams = {};
	renderParams.scene = this;
	renderParams.volumeRenderParams = m_renderParams;

	auto& rp = renderParams.volumeRenderParams;

	for (int j = 0; j < 3; j++)
	{
		for (int i = 0; i < 3; i++)
		{
			viewMatrix.M[j][i] *= scale;
		}
	}

	memcpy(&rp.projectionMatrix, &projMatrix.M[0][0], sizeof(rp.projectionMatrix));
	memcpy(&rp.viewMatrix, &viewMatrix.M[0][0], sizeof(rp.viewMatrix));

#if NVFLOW_SMP
	auto& multiResConfig = View.MultiResConf;

	rp.multiRes.enabled = View.bVRProjectEnabled && (View.VRProjMode == FSceneView::EVRProjectMode::MultiRes);
	rp.multiRes.centerWidth = multiResConfig.CenterWidth;
	rp.multiRes.centerHeight = multiResConfig.CenterHeight;
	rp.multiRes.centerX = multiResConfig.CenterX;
	rp.multiRes.centerY = multiResConfig.CenterY;
	rp.multiRes.densityScaleX[0] = multiResConfig.DensityScaleX[0];
	rp.multiRes.densityScaleX[1] = multiResConfig.DensityScaleX[1];
	rp.multiRes.densityScaleX[2] = multiResConfig.DensityScaleX[2];
	rp.multiRes.densityScaleY[0] = multiResConfig.DensityScaleY[0];
	rp.multiRes.densityScaleY[1] = multiResConfig.DensityScaleY[1];
	rp.multiRes.densityScaleY[2] = multiResConfig.DensityScaleY[2];
	rp.multiRes.viewport.topLeftX = View.ViewRect.Min.X;
	rp.multiRes.viewport.topLeftY = View.ViewRect.Min.Y;
	rp.multiRes.viewport.width = View.ViewRect.Width();
	rp.multiRes.viewport.height = View.ViewRect.Height();
	rp.multiRes.nonMultiResWidth = View.NonVRProjectViewRect.Width();
	rp.multiRes.nonMultiResHeight = View.NonVRProjectViewRect.Height();

	auto& LMSConfig = View.LensMatchedShadingConf;

	rp.lensMatchedShading.enabled = View.bVRProjectEnabled && (View.VRProjMode == FSceneView::EVRProjectMode::LensMatched);
	rp.lensMatchedShading.warpLeft = LMSConfig.WarpLeft;
	rp.lensMatchedShading.warpRight = LMSConfig.WarpRight;
	rp.lensMatchedShading.warpUp = LMSConfig.WarpUp;
	rp.lensMatchedShading.warpDown = LMSConfig.WarpDown;
	rp.lensMatchedShading.sizeLeft = FMath::CeilToInt(LMSConfig.RelativeSizeLeft * View.NonVRProjectViewRect.Width());
	rp.lensMatchedShading.sizeRight = FMath::CeilToInt(LMSConfig.RelativeSizeRight * View.NonVRProjectViewRect.Width());
	rp.lensMatchedShading.sizeUp = FMath::CeilToInt(LMSConfig.RelativeSizeUp * View.NonVRProjectViewRect.Height());
	rp.lensMatchedShading.sizeDown = FMath::CeilToInt(LMSConfig.RelativeSizeDown * View.NonVRProjectViewRect.Height());
	rp.lensMatchedShading.viewport.topLeftX = View.ViewRect.Min.X;
	rp.lensMatchedShading.viewport.topLeftY = View.ViewRect.Min.Y;
	rp.lensMatchedShading.viewport.width = View.ViewRect.Width();
	rp.lensMatchedShading.viewport.height = View.ViewRect.Height();
	rp.lensMatchedShading.nonLMSWidth = View.NonVRProjectViewRect.Width();
	rp.lensMatchedShading.nonLMSHeight = View.NonVRProjectViewRect.Height();

	if (rp.lensMatchedShading.enabled)
	{
		RHICmdList.SetModifiedWMode(View.LensMatchedShadingConf, true, false);
	}

#endif

	// push work
	RHICmdList.NvFlowWork(renderCallback, &renderParams, sizeof(RenderParams));

#if NVFLOW_SMP
	if (rp.lensMatchedShading.enabled)
	{
		RHICmdList.SetModifiedWMode(View.LensMatchedShadingConf, true, true);
	}
#endif
}

void NvFlow::Scene::renderCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx)
{
	auto renderParams = (RenderParams*)paramData;
	renderParams->scene->renderDeferred(RHICmdCtx, renderParams);
}

void NvFlow::Scene::renderDeferred(IRHICommandContext* RHICmdCtx, RenderParams* renderParams)
{
	auto& volumeRenderParams = renderParams->volumeRenderParams;

	volumeRenderParams.depthStencilView = m_context->m_dsv;
	volumeRenderParams.renderTargetView = m_context->m_rtv;

	NvFlowVolumeRenderGridExport(m_volumeRender, m_renderContext, m_gridExport4Render, &volumeRenderParams);

	if (m_volumeShadow && UFlowGridAsset::sGlobalDebugDrawShadow)
	{
		NvFlowVolumeShadowDebugRenderParams params = {};

		params.renderTargetView = m_context->m_rtv;

		params.projectionMatrix = volumeRenderParams.projectionMatrix;
		params.viewMatrix = volumeRenderParams.viewMatrix;

		NvFlowVolumeShadowDebugRender(m_volumeShadow, m_renderContext, &params);
	}

}

namespace
{
	inline FIntVector NvFlowConvert(const NvFlowUint4& in)
	{
		return FIntVector(in.x, in.y, in.z);
	}

	inline FVector NvFlowConvert(const NvFlowFloat4& in)
	{
		return FVector(in.x, in.y, in.z);
	}

	inline FMatrix NvFlowGetVolumeToWorld(FFlowGridSceneProxy* FlowGridSceneProxy)
	{
		const FBoxSphereBounds& LocalBounds = FlowGridSceneProxy->GetLocalBounds();
		const FMatrix& LocalToWorld = FlowGridSceneProxy->GetLocalToWorld();

		FMatrix VolumeToLocal = FMatrix(
			FPlane(LocalBounds.BoxExtent.X * 2, 0.0f, 0.0f, 0.0f),
			FPlane(0.0f, LocalBounds.BoxExtent.Y * 2, 0.0f, 0.0f),
			FPlane(0.0f, 0.0f, LocalBounds.BoxExtent.Z * 2, 0.0f),
			FPlane(LocalBounds.Origin - LocalBounds.BoxExtent, 1.0f));

		return VolumeToLocal * LocalToWorld;
	}
	inline FMatrix NvFlowGetWorldToVolume(FFlowGridSceneProxy* FlowGridSceneProxy)
	{
		return NvFlowGetVolumeToWorld(FlowGridSceneProxy).Inverse();
	}
}

bool NvFlow::Scene::getExportParams(FRHICommandListImmediate& RHICmdList, GridExportParamsNvFlow& OutParams)
{
	auto gridExport = NvFlowGridGetGridExport(m_renderContext, m_grid);

	auto gridExportHandle = NvFlowGridExportGetHandle(gridExport, m_renderContext, eNvFlowGridTextureChannelVelocity);
	check(gridExportHandle.numLayerViews > 0);

	// Note: assuming single layer
	const NvFlowUint layerIdx = 0u;

	NvFlowGridExportLayeredView gridExportLayeredView;
	NvFlowGridExportGetLayeredView(gridExportHandle, &gridExportLayeredView);
	NvFlowGridExportLayerView gridExportLayerView;
	NvFlowGridExportGetLayerView(gridExportHandle, layerIdx, &gridExportLayerView);

	OutParams.DataSRV = m_context->m_flowInterop->CreateSRV(RHICmdList.GetContext(), m_renderContext, gridExportLayerView.data);
	OutParams.BlockTableSRV = m_context->m_flowInterop->CreateSRV(RHICmdList.GetContext(), m_renderContext, gridExportLayerView.mapping.blockTable);

	const auto& shaderParams = gridExportLayeredView.mapping.shaderParams;

	OutParams.BlockDim = NvFlowConvert(shaderParams.blockDim);
	OutParams.BlockDimBits = NvFlowConvert(shaderParams.blockDimBits);
	OutParams.BlockDimInv = NvFlowConvert(shaderParams.blockDimInv);
	OutParams.LinearBlockDim = NvFlowConvert(shaderParams.linearBlockDim);
	OutParams.LinearBlockOffset = NvFlowConvert(shaderParams.linearBlockOffset);
	OutParams.DimInv = NvFlowConvert(shaderParams.dimInv);
	OutParams.VDim = NvFlowConvert(shaderParams.vdim);
	OutParams.VDimInv = NvFlowConvert(shaderParams.vdimInv);
	OutParams.PoolGridDim = NvFlowConvert(shaderParams.poolGridDim);
	OutParams.GridDim = NvFlowConvert(shaderParams.gridDim);
	OutParams.IsVTR = (shaderParams.isVTR.x != 0);

	OutParams.WorldToVolume = NvFlowGetWorldToVolume(FlowGridSceneProxy);
	OutParams.VelocityScale = scale;

	OutParams.GridToParticleAccelTimeConstant = FlowGridSceneProxy->FlowGridProperties.GridToParticleAccelTimeConstant;
	OutParams.GridToParticleDecelTimeConstant = FlowGridSceneProxy->FlowGridProperties.GridToParticleDecelTimeConstant;
	OutParams.GridToParticleThresholdMultiplier = FlowGridSceneProxy->FlowGridProperties.GridToParticleThresholdMultiplier;
	return true;
}


#define MASK_FROM_PARTICLES_THREAD_COUNT 64

BEGIN_UNIFORM_BUFFER_STRUCT(FNvFlowMaskFromParticlesParameters, )
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32, TextureSizeX)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32, TextureSizeY)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32, ParticleCount)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FIntVector, MaskDim)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix, WorldToVolume)
END_UNIFORM_BUFFER_STRUCT(FNvFlowMaskFromParticlesParameters)

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FNvFlowMaskFromParticlesParameters, TEXT("NvFlowMaskFromParticles"));
typedef TUniformBufferRef<FNvFlowMaskFromParticlesParameters> FNvFlowMaskFromParticlesUniformBufferRef;

class FNvFlowMaskFromParticlesCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FNvFlowMaskFromParticlesCS, Global);

public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), MASK_FROM_PARTICLES_THREAD_COUNT);
	}

	/** Default constructor. */
	FNvFlowMaskFromParticlesCS()
	{
	}

	/** Initialization constructor. */
	explicit FNvFlowMaskFromParticlesCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		InParticleIndices.Bind(Initializer.ParameterMap, TEXT("InParticleIndices"));
		PositionTexture.Bind(Initializer.ParameterMap, TEXT("PositionTexture"));
		PositionTextureSampler.Bind(Initializer.ParameterMap, TEXT("PositionTextureSampler"));
		OutMask.Bind(Initializer.ParameterMap, TEXT("OutMask"));
	}

	/** Serialization. */
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << InParticleIndices;
		Ar << PositionTexture;
		Ar << PositionTextureSampler;
		Ar << OutMask;
		return bShaderHasOutdatedParameters;
	}

	/**
	* Set output buffers for this shader.
	*/
	void SetOutput(IRHICommandContext* RHICmdCtx, FUnorderedAccessViewRHIParamRef OutMaskUAV)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (OutMask.IsBound())
		{
			RHICmdCtx->RHISetUAVParameter(ComputeShaderRHI, OutMask.GetBaseIndex(), OutMaskUAV);
		}
	}

	/**
	* Set input parameters.
	*/
	void SetParameters(
		IRHICommandContext* RHICmdCtx,
		FNvFlowMaskFromParticlesUniformBufferRef& UniformBuffer,
		FShaderResourceViewRHIParamRef InIndicesSRV,
		FTexture2DRHIParamRef PositionTextureRHI
	)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		//SetUniformBufferParameter(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FNvFlowMaskFromParticlesParameters>(), UniformBuffer);
		{
			auto Shader = ComputeShaderRHI;
			const auto& Parameter = GetUniformBufferParameter<FNvFlowMaskFromParticlesParameters>();
			auto UniformBufferRHI = UniformBuffer;
			if (Parameter.IsBound())
			{
				RHICmdCtx->RHISetShaderUniformBuffer(Shader, Parameter.GetBaseIndex(), UniformBufferRHI);
			}
		}
		if (InParticleIndices.IsBound())
		{
			RHICmdCtx->RHISetShaderResourceViewParameter(ComputeShaderRHI, InParticleIndices.GetBaseIndex(), InIndicesSRV);
		}
		if (PositionTexture.IsBound())
		{
			RHICmdCtx->RHISetShaderTexture(ComputeShaderRHI, PositionTexture.GetBaseIndex(), PositionTextureRHI);
		}
	}

	/**
	* Unbinds any buffers that have been bound.
	*/
	void UnbindBuffers(IRHICommandContext* RHICmdCtx)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (InParticleIndices.IsBound())
		{
			RHICmdCtx->RHISetShaderResourceViewParameter(ComputeShaderRHI, InParticleIndices.GetBaseIndex(), FShaderResourceViewRHIParamRef());
		}
		if (OutMask.IsBound())
		{
			RHICmdCtx->RHISetUAVParameter(ComputeShaderRHI, OutMask.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
		}
	}

private:

	/** Input buffer containing particle indices. */
	FShaderResourceParameter InParticleIndices;
	/** Texture containing particle positions. */
	FShaderResourceParameter PositionTexture;
	FShaderResourceParameter PositionTextureSampler;
	/** Output key buffer. */
	FShaderResourceParameter OutMask;
};
IMPLEMENT_SHADER_TYPE(, FNvFlowMaskFromParticlesCS, TEXT("NvFlowAllocShader"), TEXT("ComputeMaskFromParticles"), SF_Compute);


void NvFlow::Scene::emitCustomAllocCallback(IRHICommandContext* RHICmdCtx, const NvFlowGridEmitCustomAllocParams* params, const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData)
{
	if (m_particleParamsArray.Num() == 0)
	{
		return;
	}
	m_context->m_flowInterop->Pop(*RHICmdCtx, m_renderContext);

	TShaderMapRef<FNvFlowMaskFromParticlesCS> MaskFromParticlesCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdCtx->RHISetComputeShader(MaskFromParticlesCS->GetComputeShader());

	FNvFlowMaskFromParticlesParameters MaskFromParticlesParameters;
	MaskFromParticlesParameters.WorldToVolume = NvFlowGetWorldToVolume(FlowGridSceneProxy);
	MaskFromParticlesParameters.MaskDim = FIntVector(params->maskDim.x, params->maskDim.y, params->maskDim.z);

	FUnorderedAccessViewRHIRef MaskUAV;
	FRHINvFlowResourceRW* pMaskResourceRW = m_context->m_flowInterop->CreateResourceRW(
		*RHICmdCtx, m_renderContext, params->maskResourceRW, nullptr, &MaskUAV);

	for (int32 i = 0; i < m_particleParamsArray.Num(); ++i)
	{
		const ParticleSimulationParamsNvFlow& ParticleParams = m_particleParamsArray[i];
		if (ParticleParams.ParticleCount > 0)
		{
			MaskFromParticlesParameters.ParticleCount = ParticleParams.ParticleCount;
			MaskFromParticlesParameters.TextureSizeX = ParticleParams.TextureSizeX;
			MaskFromParticlesParameters.TextureSizeY = ParticleParams.TextureSizeY;
			FNvFlowMaskFromParticlesUniformBufferRef UniformBuffer = FNvFlowMaskFromParticlesUniformBufferRef::CreateUniformBufferImmediate(MaskFromParticlesParameters, UniformBuffer_SingleFrame);

			uint32 GroupCount = (ParticleParams.ParticleCount + MASK_FROM_PARTICLES_THREAD_COUNT - 1) / MASK_FROM_PARTICLES_THREAD_COUNT;

			MaskFromParticlesCS->SetOutput(RHICmdCtx, MaskUAV);
			MaskFromParticlesCS->SetParameters(RHICmdCtx, UniformBuffer, ParticleParams.VertexBufferSRV, ParticleParams.PositionTextureRHI);
			//DispatchComputeShader(RHICmdCtx, *MaskFromParticlesCS, GroupCount, 1, 1);
			RHICmdCtx->RHIDispatchComputeShader(GroupCount, 1, 1);
			MaskFromParticlesCS->UnbindBuffers(RHICmdCtx);
		}
	}

	m_context->m_flowInterop->ReleaseResourceRW(*RHICmdCtx, pMaskResourceRW);

	m_context->m_flowInterop->Push(*RHICmdCtx, m_renderContext);
}


#define COPY_THREAD_COUNT_X 4
#define COPY_THREAD_COUNT_Y 4
#define COPY_THREAD_COUNT_Z 4

BEGIN_UNIFORM_BUFFER_STRUCT(FNvFlowCopyGridDataParameters, )
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FIntVector, ThreadDim)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FIntVector, BlockDim)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FIntVector, BlockDimBits)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int32, IsVTR)
END_UNIFORM_BUFFER_STRUCT(FNvFlowCopyGridDataParameters)

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FNvFlowCopyGridDataParameters, TEXT("NvFlowCopyGridData"));
typedef TUniformBufferRef<FNvFlowCopyGridDataParameters> FNvFlowCopyGridDataUniformBufferRef;

BEGIN_UNIFORM_BUFFER_STRUCT(FNvFlowApplyDistanceFieldParameters, )
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FIntVector, ThreadDim)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FIntVector, BlockDim)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FIntVector, BlockDimBits)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int32, IsVTR)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, VDimInv)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix, VolumeToWorld)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, DistanceScale)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, MinActiveDist)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, MaxActiveDist)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, ValueCoupleRate)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, EmitValue)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, SlipFactor)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, SlipThickness)
END_UNIFORM_BUFFER_STRUCT(FNvFlowApplyDistanceFieldParameters)

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FNvFlowApplyDistanceFieldParameters, TEXT("NvFlowApplyDistanceField"));
typedef TUniformBufferRef<FNvFlowApplyDistanceFieldParameters> FNvFlowApplyDistanceFieldUniformBufferRef;


class FNvFlowCopyGridDataCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FNvFlowCopyGridDataCS, Global);

public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT_X"), COPY_THREAD_COUNT_X);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT_Y"), COPY_THREAD_COUNT_Y);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT_Z"), COPY_THREAD_COUNT_Z);
	}

	/** Default constructor. */
	FNvFlowCopyGridDataCS()
	{
	}

	/** Initialization constructor. */
	explicit FNvFlowCopyGridDataCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		BlockList.Bind(Initializer.ParameterMap, TEXT("BlockList"));
		BlockTable.Bind(Initializer.ParameterMap, TEXT("BlockTable"));
		DataIn.Bind(Initializer.ParameterMap, TEXT("DataIn"));
		DataOut.Bind(Initializer.ParameterMap, TEXT("DataOut"));
	}

	/** Serialization. */
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << BlockList;
		Ar << BlockTable;
		Ar << DataIn;
		Ar << DataOut;
		return bShaderHasOutdatedParameters;
	}

	/**
	* Set output buffers for this shader.
	*/
	void SetOutput(IRHICommandContext* RHICmdCtx, FUnorderedAccessViewRHIParamRef DataOutUAV)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (DataOut.IsBound())
		{
			RHICmdCtx->RHISetUAVParameter(ComputeShaderRHI, DataOut.GetBaseIndex(), DataOutUAV);
		}
	}

	/**
	* Set input parameters.
	*/
	void SetParameters(
		IRHICommandContext* RHICmdCtx,
		TUniformBufferRef<FNvFlowCopyGridDataParameters>& UniformBuffer,
		FShaderResourceViewRHIParamRef BlockListSRV,
		FShaderResourceViewRHIParamRef BlockTableSRV,
		FShaderResourceViewRHIParamRef DataInSRV
	)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		//SetUniformBufferParameter(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FNvFlowCopyGridDataParameters>(), UniformBuffer);
		{
			auto Shader = ComputeShaderRHI;
			const auto& Parameter = GetUniformBufferParameter<FNvFlowCopyGridDataParameters>();
			auto UniformBufferRHI = UniformBuffer;
			if (Parameter.IsBound())
			{
				RHICmdCtx->RHISetShaderUniformBuffer(Shader, Parameter.GetBaseIndex(), UniformBufferRHI);
			}
		}
		if (BlockList.IsBound())
		{
			RHICmdCtx->RHISetShaderResourceViewParameter(ComputeShaderRHI, BlockList.GetBaseIndex(), BlockListSRV);
		}
		if (BlockTable.IsBound())
		{
			RHICmdCtx->RHISetShaderResourceViewParameter(ComputeShaderRHI, BlockTable.GetBaseIndex(), BlockTableSRV);
		}
		if (DataIn.IsBound())
		{
			RHICmdCtx->RHISetShaderResourceViewParameter(ComputeShaderRHI, DataIn.GetBaseIndex(), DataInSRV);
		}
	}

	/**
	* Unbinds any buffers that have been bound.
	*/
	void UnbindBuffers(IRHICommandContext* RHICmdCtx)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (DataOut.IsBound())
		{
			RHICmdCtx->RHISetUAVParameter(ComputeShaderRHI, DataOut.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
		}
	}

private:
	FShaderResourceParameter BlockList;
	FShaderResourceParameter BlockTable;
	FShaderResourceParameter DataIn;
	FShaderResourceParameter DataOut;
};
IMPLEMENT_SHADER_TYPE(, FNvFlowCopyGridDataCS, TEXT("NvFlowCopyShader"), TEXT("CopyGridData"), SF_Compute);


class FNvFlowApplyDistanceFieldCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FNvFlowApplyDistanceFieldCS, Global);

public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT_X"), COPY_THREAD_COUNT_X);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT_Y"), COPY_THREAD_COUNT_Y);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT_Z"), COPY_THREAD_COUNT_Z);
	}

	/** Default constructor. */
	FNvFlowApplyDistanceFieldCS()
	{
	}

	/** Initialization constructor. */
	explicit FNvFlowApplyDistanceFieldCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		BlockList.Bind(Initializer.ParameterMap, TEXT("BlockList"));
		BlockTable.Bind(Initializer.ParameterMap, TEXT("BlockTable"));
		DataIn.Bind(Initializer.ParameterMap, TEXT("DataIn"));
		DataOut.Bind(Initializer.ParameterMap, TEXT("DataOut"));

		GlobalDistanceFieldParameters.Bind(Initializer.ParameterMap);
	}

	/** Serialization. */
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << BlockList;
		Ar << BlockTable;
		Ar << DataIn;
		Ar << DataOut;
		Ar << GlobalDistanceFieldParameters;
		return bShaderHasOutdatedParameters;
	}

	/**
	* Set output buffers for this shader.
	*/
	void SetOutput(IRHICommandContext* RHICmdCtx, FUnorderedAccessViewRHIParamRef DataOutUAV)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (DataOut.IsBound())
		{
			RHICmdCtx->RHISetUAVParameter(ComputeShaderRHI, DataOut.GetBaseIndex(), DataOutUAV);
		}
	}

	/**
	* Set input parameters.
	*/
	void SetParameters(
		IRHICommandContext* RHICmdCtx,
		TUniformBufferRef<FNvFlowApplyDistanceFieldParameters>& UniformBuffer,
		FShaderResourceViewRHIParamRef BlockListSRV,
		FShaderResourceViewRHIParamRef BlockTableSRV,
		FShaderResourceViewRHIParamRef DataInSRV,
		const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData
	)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		//SetUniformBufferParameter(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FNvFlowApplyDistanceFieldParameters>(), UniformBuffer);
		{
			auto Shader = ComputeShaderRHI;
			const auto& Parameter = GetUniformBufferParameter<FNvFlowApplyDistanceFieldParameters>();
			auto UniformBufferRHI = UniformBuffer;
			if (Parameter.IsBound())
			{
				RHICmdCtx->RHISetShaderUniformBuffer(Shader, Parameter.GetBaseIndex(), UniformBufferRHI);
			}
		}
		if (BlockList.IsBound())
		{
			RHICmdCtx->RHISetShaderResourceViewParameter(ComputeShaderRHI, BlockList.GetBaseIndex(), BlockListSRV);
		}
		if (BlockTable.IsBound())
		{
			RHICmdCtx->RHISetShaderResourceViewParameter(ComputeShaderRHI, BlockTable.GetBaseIndex(), BlockTableSRV);
		}
		if (DataIn.IsBound())
		{
			RHICmdCtx->RHISetShaderResourceViewParameter(ComputeShaderRHI, DataIn.GetBaseIndex(), DataInSRV);
		}
		if (GlobalDistanceFieldParameterData != nullptr)
		{
			GlobalDistanceFieldParameters.Set(RHICmdCtx, ComputeShaderRHI, *GlobalDistanceFieldParameterData);
		}
	}

	/**
	* Unbinds any buffers that have been bound.
	*/
	void UnbindBuffers(IRHICommandContext* RHICmdCtx)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (DataOut.IsBound())
		{
			RHICmdCtx->RHISetUAVParameter(ComputeShaderRHI, DataOut.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
		}
	}

private:
	FShaderResourceParameter BlockList;
	FShaderResourceParameter BlockTable;
	FShaderResourceParameter DataIn;
	FShaderResourceParameter DataOut;

	FGlobalDistanceFieldParameters GlobalDistanceFieldParameters;
};
IMPLEMENT_SHADER_TYPE(, FNvFlowApplyDistanceFieldCS, TEXT("NvFlowDistanceFieldShader"), TEXT("ApplyDistanceField"), SF_Compute);


#define COUPLE_PARTICLES_THREAD_COUNT 64

BEGIN_UNIFORM_BUFFER_STRUCT(FNvFlowCoupleParticlesParameters, )
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32, TextureSizeX)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32, TextureSizeY)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32, ParticleCount)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix, WorldToVolume)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FIntVector, VDim)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FIntVector, BlockDim)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FIntVector, BlockDimBits)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int32, IsVTR)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, AccelRate)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, DecelRate)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, Threshold)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, InvVelocityScale)
END_UNIFORM_BUFFER_STRUCT(FNvFlowCoupleParticlesParameters)

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FNvFlowCoupleParticlesParameters, TEXT("NvFlowCoupleParticles"));
typedef TUniformBufferRef<FNvFlowCoupleParticlesParameters> FNvFlowCoupleParticlesUniformBufferRef;

class FNvFlowCoupleParticlesCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FNvFlowCoupleParticlesCS, Global);

public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), COUPLE_PARTICLES_THREAD_COUNT);
	}

	/** Default constructor. */
	FNvFlowCoupleParticlesCS()
	{
	}

	/** Initialization constructor. */
	explicit FNvFlowCoupleParticlesCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		InParticleIndices.Bind(Initializer.ParameterMap, TEXT("InParticleIndices"));
		PositionTexture.Bind(Initializer.ParameterMap, TEXT("PositionTexture"));
		VelocityTexture.Bind(Initializer.ParameterMap, TEXT("VelocityTexture"));
		BlockTable.Bind(Initializer.ParameterMap, TEXT("BlockTable"));
		DataIn.Bind(Initializer.ParameterMap, TEXT("DataIn"));
		DataOut.Bind(Initializer.ParameterMap, TEXT("DataOut"));
	}

	/** Serialization. */
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << InParticleIndices;
		Ar << PositionTexture;
		Ar << VelocityTexture;
		Ar << BlockTable;
		Ar << DataIn;
		Ar << DataOut;
		return bShaderHasOutdatedParameters;
	}

	/**
	* Set output buffers for this shader.
	*/
	void SetOutput(IRHICommandContext* RHICmdCtx, FUnorderedAccessViewRHIParamRef DataOutUAV)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (DataOut.IsBound())
		{
			RHICmdCtx->RHISetUAVParameter(ComputeShaderRHI, DataOut.GetBaseIndex(), DataOutUAV);
		}
	}

	/**
	* Set input parameters.
	*/
	void SetParameters(
		IRHICommandContext* RHICmdCtx,
		FNvFlowCoupleParticlesUniformBufferRef& UniformBuffer,
		FShaderResourceViewRHIParamRef InIndicesSRV,
		FTexture2DRHIParamRef PositionTextureRHI,
		FTexture2DRHIParamRef VelocityTextureRHI,
		FShaderResourceViewRHIParamRef BlockTableSRV,
		FShaderResourceViewRHIParamRef DataInSRV)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		//SetUniformBufferParameter(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FNvFlowCoupleParticlesParameters>(), UniformBuffer);
		{
			auto Shader = ComputeShaderRHI;
			const auto& Parameter = GetUniformBufferParameter<FNvFlowCoupleParticlesParameters>();
			auto UniformBufferRHI = UniformBuffer;
			if (Parameter.IsBound())
			{
				RHICmdCtx->RHISetShaderUniformBuffer(Shader, Parameter.GetBaseIndex(), UniformBufferRHI);
			}
		}
		if (InParticleIndices.IsBound())
		{
			RHICmdCtx->RHISetShaderResourceViewParameter(ComputeShaderRHI, InParticleIndices.GetBaseIndex(), InIndicesSRV);
		}
		if (PositionTexture.IsBound())
		{
			RHICmdCtx->RHISetShaderTexture(ComputeShaderRHI, PositionTexture.GetBaseIndex(), PositionTextureRHI);
		}
		if (VelocityTexture.IsBound())
		{
			RHICmdCtx->RHISetShaderTexture(ComputeShaderRHI, VelocityTexture.GetBaseIndex(), VelocityTextureRHI);
		}
		if (BlockTable.IsBound())
		{
			RHICmdCtx->RHISetShaderResourceViewParameter(ComputeShaderRHI, BlockTable.GetBaseIndex(), BlockTableSRV);
		}
		if (DataIn.IsBound())
		{
			RHICmdCtx->RHISetShaderResourceViewParameter(ComputeShaderRHI, DataIn.GetBaseIndex(), DataInSRV);
		}
	}

	/**
	* Unbinds any buffers that have been bound.
	*/
	void UnbindBuffers(IRHICommandContext* RHICmdCtx)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if (InParticleIndices.IsBound())
		{
			RHICmdCtx->RHISetShaderResourceViewParameter(ComputeShaderRHI, InParticleIndices.GetBaseIndex(), FShaderResourceViewRHIParamRef());
		}
		if (DataOut.IsBound())
		{
			RHICmdCtx->RHISetUAVParameter(ComputeShaderRHI, DataOut.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
		}
	}

private:

	/** Input buffer containing particle indices. */
	FShaderResourceParameter InParticleIndices;
	/** Texture containing particle positions. */
	FShaderResourceParameter PositionTexture;
	FShaderResourceParameter VelocityTexture;

	FShaderResourceParameter BlockTable;
	FShaderResourceParameter DataIn;
	FShaderResourceParameter DataOut;
};
IMPLEMENT_SHADER_TYPE(, FNvFlowCoupleParticlesCS, TEXT("NvFlowCoupleShader"), TEXT("CoupleParticlesToGrid"), SF_Compute);


void NvFlow::Scene::applyDistanceField(IRHICommandContext* RHICmdCtx, NvFlowUint dataFrontIdx, const NvFlowGridEmitCustomEmitLayerParams& layerParams, const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData, float dt,
	FShaderResourceViewRHIRef DataInSRV, FUnorderedAccessViewRHIRef DataOutUAV, FShaderResourceViewRHIRef BlockListSRV, FShaderResourceViewRHIRef BlockTableSRV,
	float InSlipFactor, float InSlipThickness, FVector4 InEmitValue)
{
	if (layerParams.numBlocks == 0)
	{
		return;
	}

	TShaderMapRef<FNvFlowApplyDistanceFieldCS> ApplyDistanceFieldCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdCtx->RHISetComputeShader(ApplyDistanceFieldCS->GetComputeShader());

	FIntVector VDim = FIntVector(
		layerParams.shaderParams.blockDim.x * layerParams.shaderParams.gridDim.x,
		layerParams.shaderParams.blockDim.y * layerParams.shaderParams.gridDim.y,
		layerParams.shaderParams.blockDim.z * layerParams.shaderParams.gridDim.z);

	FNvFlowApplyDistanceFieldParameters Parameters;
	Parameters.BlockDim = NvFlowConvert(layerParams.shaderParams.blockDim);
	Parameters.BlockDimBits = NvFlowConvert(layerParams.shaderParams.blockDimBits);
	Parameters.IsVTR = layerParams.shaderParams.isVTR.x;
	Parameters.ThreadDim = Parameters.BlockDim;
	Parameters.ThreadDim.X *= layerParams.numBlocks;
	Parameters.VDimInv = FVector(1.0f / VDim.X, 1.0f / VDim.Y, 1.0f / VDim.Z);
	Parameters.VolumeToWorld = NvFlowGetVolumeToWorld(FlowGridSceneProxy);
	Parameters.MinActiveDist = FlowGridSceneProxy->FlowGridProperties.MinActiveDistance;
	Parameters.MaxActiveDist = FlowGridSceneProxy->FlowGridProperties.MaxActiveDistance;
	Parameters.ValueCoupleRate = 100.0f * dt;
	Parameters.DistanceScale = 1.0f / scale;
	Parameters.EmitValue = InEmitValue;
	Parameters.SlipFactor = InSlipFactor;
	Parameters.SlipThickness = InSlipThickness;

	FNvFlowApplyDistanceFieldUniformBufferRef UniformBuffer = FNvFlowApplyDistanceFieldUniformBufferRef::CreateUniformBufferImmediate(Parameters, UniformBuffer_SingleFrame);

	uint32 GroupCountX = (Parameters.ThreadDim.X + COPY_THREAD_COUNT_X - 1) / COPY_THREAD_COUNT_X;
	uint32 GroupCountY = (Parameters.ThreadDim.Y + COPY_THREAD_COUNT_Y - 1) / COPY_THREAD_COUNT_Y;
	uint32 GroupCountZ = (Parameters.ThreadDim.Z + COPY_THREAD_COUNT_Z - 1) / COPY_THREAD_COUNT_Z;

	ApplyDistanceFieldCS->SetOutput(RHICmdCtx, DataOutUAV);
	ApplyDistanceFieldCS->SetParameters(RHICmdCtx, UniformBuffer, BlockListSRV, BlockTableSRV, DataInSRV, GlobalDistanceFieldParameterData);
	//DispatchComputeShader(RHICmdCtx, *ApplyDistanceFieldCS, GroupCountX, GroupCountY, GroupCountZ);
	RHICmdCtx->RHIDispatchComputeShader(GroupCountX, GroupCountY, GroupCountZ);
	ApplyDistanceFieldCS->UnbindBuffers(RHICmdCtx);
}

void NvFlow::Scene::emitCustomEmitVelocityCallback(IRHICommandContext* RHICmdCtx, NvFlowUint* dataFrontIdx, const NvFlowGridEmitCustomEmitParams* emitParams, const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData, float dt)
{
	const bool bHasDistanceFieldCollision = FlowGridSceneProxy->FlowGridProperties.bDistanceFieldCollisionEnabled &&
		(GlobalDistanceFieldParameterData->Textures[0] != nullptr);

	const bool bHasParticles = m_particleParamsArray.Num() > 0;

	if (emitParams->numLayers == 0 || !(bHasDistanceFieldCollision || bHasParticles))
	{
		return;
	}

	m_context->m_flowInterop->Pop(*RHICmdCtx, m_renderContext);

	for (uint32 layerId = 0; layerId < emitParams->numLayers; ++layerId)
	{
		NvFlowGridEmitCustomEmitLayerParams layerParams;
		NvFlowGridEmitCustomGetLayerParams(emitParams, layerId, &layerParams);

		FShaderResourceViewRHIRef Data0SRV, Data1SRV;
		FUnorderedAccessViewRHIRef Data0UAV, Data1UAV;

		FRHINvFlowResourceRW* pData0ResourceRW = m_context->m_flowInterop->CreateResourceRW(
			*RHICmdCtx, m_renderContext, layerParams.dataRW[*dataFrontIdx], &Data0SRV, &Data0UAV);

		FRHINvFlowResourceRW* pData1ResourceRW = m_context->m_flowInterop->CreateResourceRW(
			*RHICmdCtx, m_renderContext, layerParams.dataRW[*dataFrontIdx ^ 1], &Data1SRV, &Data1UAV);

		FShaderResourceViewRHIRef BlockListSRV = m_context->m_flowInterop->CreateSRV(*RHICmdCtx, m_renderContext, layerParams.blockList);
		FShaderResourceViewRHIRef BlockTableSRV = m_context->m_flowInterop->CreateSRV(*RHICmdCtx, m_renderContext, layerParams.blockTable);

		if (bHasParticles)
		{
			{
				TShaderMapRef<FNvFlowCopyGridDataCS> CopyGridDataCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
				RHICmdCtx->RHISetComputeShader(CopyGridDataCS->GetComputeShader());

				FNvFlowCopyGridDataParameters CopyGridDataParameters;
				CopyGridDataParameters.BlockDim = NvFlowConvert(layerParams.shaderParams.blockDim);
				CopyGridDataParameters.BlockDimBits = NvFlowConvert(layerParams.shaderParams.blockDimBits);
				CopyGridDataParameters.IsVTR = layerParams.shaderParams.isVTR.x;
				CopyGridDataParameters.ThreadDim = CopyGridDataParameters.BlockDim;
				CopyGridDataParameters.ThreadDim.X *= layerParams.numBlocks;
				FNvFlowCopyGridDataUniformBufferRef UniformBuffer = FNvFlowCopyGridDataUniformBufferRef::CreateUniformBufferImmediate(CopyGridDataParameters, UniformBuffer_SingleFrame);

				uint32 GroupCountX = (CopyGridDataParameters.ThreadDim.X + COPY_THREAD_COUNT_X - 1) / COPY_THREAD_COUNT_X;
				uint32 GroupCountY = (CopyGridDataParameters.ThreadDim.Y + COPY_THREAD_COUNT_Y - 1) / COPY_THREAD_COUNT_Y;
				uint32 GroupCountZ = (CopyGridDataParameters.ThreadDim.Z + COPY_THREAD_COUNT_Z - 1) / COPY_THREAD_COUNT_Z;

				FShaderResourceViewRHIRef DataInSRV = Data0SRV;
				FUnorderedAccessViewRHIRef DataOutUAV = Data1UAV;

				CopyGridDataCS->SetOutput(RHICmdCtx, DataOutUAV);
				CopyGridDataCS->SetParameters(RHICmdCtx, UniformBuffer, BlockListSRV, BlockTableSRV, DataInSRV);
				//DispatchComputeShader(RHICmdCtx, *CopyGridDataCS, GroupCountX, GroupCountY, GroupCountZ);
				RHICmdCtx->RHIDispatchComputeShader(GroupCountX, GroupCountY, GroupCountZ);
				CopyGridDataCS->UnbindBuffers(RHICmdCtx);
			}

			TShaderMapRef<FNvFlowCoupleParticlesCS> CoupleParticlesCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			RHICmdCtx->RHISetComputeShader(CoupleParticlesCS->GetComputeShader());

			FNvFlowCoupleParticlesParameters CoupleParticlesParameters;
			CoupleParticlesParameters.WorldToVolume = NvFlowGetWorldToVolume(FlowGridSceneProxy);
			CoupleParticlesParameters.VDim = FIntVector(
				layerParams.shaderParams.blockDim.x * layerParams.shaderParams.gridDim.x,
				layerParams.shaderParams.blockDim.y * layerParams.shaderParams.gridDim.y,
				layerParams.shaderParams.blockDim.z * layerParams.shaderParams.gridDim.z);
			CoupleParticlesParameters.BlockDim = NvFlowConvert(layerParams.shaderParams.blockDim);
			CoupleParticlesParameters.BlockDimBits = NvFlowConvert(layerParams.shaderParams.blockDimBits);
			CoupleParticlesParameters.IsVTR = layerParams.shaderParams.isVTR.x;

			CoupleParticlesParameters.AccelRate = dt / FlowGridSceneProxy->FlowGridProperties.ParticleToGridAccelTimeConstant;
			CoupleParticlesParameters.DecelRate = dt / FlowGridSceneProxy->FlowGridProperties.ParticleToGridDecelTimeConstant;
			CoupleParticlesParameters.Threshold = FlowGridSceneProxy->FlowGridProperties.ParticleToGridThresholdMultiplier;

			CoupleParticlesParameters.InvVelocityScale = 1.0f / scale;

			FShaderResourceViewRHIRef DataInSRV = Data1SRV;
			FUnorderedAccessViewRHIRef DataOutUAV = Data0UAV;

			for (int32 i = 0; i < m_particleParamsArray.Num(); ++i)
			{
				const ParticleSimulationParamsNvFlow& ParticleParams = m_particleParamsArray[i];
				if (ParticleParams.ParticleCount > 0)
				{
					CoupleParticlesParameters.ParticleCount = ParticleParams.ParticleCount;
					CoupleParticlesParameters.TextureSizeX = ParticleParams.TextureSizeX;
					CoupleParticlesParameters.TextureSizeY = ParticleParams.TextureSizeY;
					FNvFlowCoupleParticlesUniformBufferRef UniformBuffer = FNvFlowCoupleParticlesUniformBufferRef::CreateUniformBufferImmediate(CoupleParticlesParameters, UniformBuffer_SingleFrame);

					uint32 GroupCount = (ParticleParams.ParticleCount + COUPLE_PARTICLES_THREAD_COUNT - 1) / COUPLE_PARTICLES_THREAD_COUNT;

					CoupleParticlesCS->SetOutput(RHICmdCtx, DataOutUAV);
					CoupleParticlesCS->SetParameters(RHICmdCtx, UniformBuffer, ParticleParams.VertexBufferSRV, ParticleParams.PositionTextureRHI, ParticleParams.VelocityTextureRHI, BlockTableSRV, DataInSRV);
					//DispatchComputeShader(RHICmdCtx, *CoupleParticlesCS, GroupCount, 1, 1);
					RHICmdCtx->RHIDispatchComputeShader(GroupCount, 1, 1);
					CoupleParticlesCS->UnbindBuffers(RHICmdCtx);
				}
			}
		}

		if (bHasDistanceFieldCollision)
		{
			applyDistanceField(RHICmdCtx, *dataFrontIdx, layerParams, GlobalDistanceFieldParameterData, dt,
				Data0SRV, Data1UAV, BlockListSRV, BlockTableSRV,
				FlowGridSceneProxy->FlowGridProperties.VelocitySlipFactor, FlowGridSceneProxy->FlowGridProperties.VelocitySlipThickness);
		}

		m_context->m_flowInterop->ReleaseResourceRW(*RHICmdCtx, pData1ResourceRW);
		m_context->m_flowInterop->ReleaseResourceRW(*RHICmdCtx, pData0ResourceRW);
	}

	if (bHasDistanceFieldCollision)
	{
		*dataFrontIdx ^= 1;
	}

	m_context->m_flowInterop->Push(*RHICmdCtx, m_renderContext);
}

void NvFlow::Scene::emitCustomEmitDensityCallback(IRHICommandContext* RHICmdCtx, NvFlowUint* dataFrontIdx, const NvFlowGridEmitCustomEmitParams* emitParams, const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData, float dt)
{
	bool bHasDistanceFieldCollision = FlowGridSceneProxy->FlowGridProperties.bDistanceFieldCollisionEnabled &&
		(GlobalDistanceFieldParameterData->Textures[0] != nullptr);

	if (emitParams->numLayers == 0 || !bHasDistanceFieldCollision)
	{
		return;
	}

	m_context->m_flowInterop->Pop(*RHICmdCtx, m_renderContext);

	for (uint32 layerId = 0; layerId < emitParams->numLayers; ++layerId)
	{
		NvFlowGridEmitCustomEmitLayerParams layerParams;
		NvFlowGridEmitCustomGetLayerParams(emitParams, layerId, &layerParams);

		FShaderResourceViewRHIRef Data0SRV;
		FUnorderedAccessViewRHIRef Data1UAV;

		FRHINvFlowResourceRW* pData0ResourceRW = m_context->m_flowInterop->CreateResourceRW(
			*RHICmdCtx, m_renderContext, layerParams.dataRW[*dataFrontIdx], &Data0SRV, nullptr);

		FRHINvFlowResourceRW* pData1ResourceRW = m_context->m_flowInterop->CreateResourceRW(
			*RHICmdCtx, m_renderContext, layerParams.dataRW[*dataFrontIdx ^ 1], nullptr, &Data1UAV);

		FShaderResourceViewRHIRef BlockListSRV = m_context->m_flowInterop->CreateSRV(*RHICmdCtx, m_renderContext, layerParams.blockList);
		FShaderResourceViewRHIRef BlockTableSRV = m_context->m_flowInterop->CreateSRV(*RHICmdCtx, m_renderContext, layerParams.blockTable);

		applyDistanceField(RHICmdCtx, *dataFrontIdx, layerParams, GlobalDistanceFieldParameterData, dt,
			Data0SRV, Data1UAV, BlockListSRV, BlockTableSRV);

		m_context->m_flowInterop->ReleaseResourceRW(*RHICmdCtx, pData1ResourceRW);
		m_context->m_flowInterop->ReleaseResourceRW(*RHICmdCtx, pData0ResourceRW);
	}

	*dataFrontIdx ^= 1;

	m_context->m_flowInterop->Push(*RHICmdCtx, m_renderContext);
}

// ---------------- global interface functions ---------------------

bool NvFlowUsesGlobalDistanceField()
{
	bool bResult = false;
	if (NvFlow::gContext)
	{
		for (int32 i = 0; i < NvFlow::gContext->m_sceneList.Num(); i++)
		{
			NvFlow::Scene* Scene = NvFlow::gContext->m_sceneList[i];
			FFlowGridSceneProxy* FlowGridSceneProxy = Scene->FlowGridSceneProxy;
			bResult |= FlowGridSceneProxy->FlowGridProperties.bDistanceFieldCollisionEnabled;
		}
	}
	return bResult;
}

void NvFlowUpdateScene(FRHICommandListImmediate& RHICmdList, TArray<FPrimitiveSceneInfo*>& Primitives, const class FGlobalDistanceFieldParameterData* GlobalDistanceFieldParameterData)
{
	if (GUsingNullRHI)
	{
		return;
	}

	bool shouldFlush = false;

	SCOPE_CYCLE_COUNTER(STAT_Flow_SimulateGrids);
	SCOPED_DRAW_EVENT(RHICmdList, FlowSimulateGrids);
	{
		SCOPED_DRAW_EVENT(RHICmdList, FlowContextSimulate);

		// create a context if one does not exist
		if (NvFlow::gContext == nullptr)
		{
			NvFlow::gContext = &NvFlow::gContextImpl;

			NvFlow::gContext->init(RHICmdList);
		}

		NvFlow::gContext->conditionalInitMultiGPU(RHICmdList);

		NvFlow::gContext->interopBegin(RHICmdList, true);

		// look for FFlowGridSceneProxy, TODO replace with adding special member to FScene
		FFlowGridSceneProxy* FlowGridSceneProxy = nullptr;
		for (int32 i = 0; i < Primitives.Num(); i++)
		{
			FPrimitiveSceneInfo* PrimitiveSceneInfo = Primitives[i];
			if (PrimitiveSceneInfo->Proxy->FlowData.bFlowGrid)
			{
				FlowGridSceneProxy = (FFlowGridSceneProxy*)PrimitiveSceneInfo->Proxy;

				if (UFlowGridAsset::sGlobalMultiGPUResetRequest)
				{
					FlowGridSceneProxy->FlowGridProperties.bActive = false;
				}

				NvFlow::gContext->updateScene(RHICmdList, FlowGridSceneProxy, shouldFlush, GlobalDistanceFieldParameterData);
			}
		}

		if (UFlowGridAsset::sGlobalMultiGPUResetRequest)
		{
			UFlowGridAsset::sGlobalMultiGPUResetRequest = false;
		}
	}
	{
		SCOPED_DRAW_EVENT(RHICmdList, FlowUpdateGridViews);
		{
			SCOPED_DRAW_EVENT(RHICmdList, FlowContextUpdateGridView);

			NvFlow::gContext->updateGridView(RHICmdList);
		}

		NvFlow::gContext->interopEnd(RHICmdList, true, shouldFlush);
	}
	if (NvFlow::gContext)
	{
		RHICmdList.NvFlowWork(NvFlow::Context::cleanupSceneListCallback, NvFlow::gContext, 0u);
	}
}

bool NvFlowDoRenderPrimitive(FRHICommandList& RHICmdList, const FViewInfo& View, FPrimitiveSceneInfo* PrimitiveSceneInfo)
{
	if (!GUsingNullRHI && NvFlow::gContext)
	{
		if (PrimitiveSceneInfo->Proxy->FlowData.bFlowGrid)
		{
			FFlowGridSceneProxy* FlowGridSceneProxy = (FFlowGridSceneProxy*)PrimitiveSceneInfo->Proxy;
			if (FlowGridSceneProxy->FlowGridProperties.bParticleModeEnabled != 0 && 
				FlowGridSceneProxy->FlowGridProperties.RenderParams.bDebugWireframe == 0)
			{
				return false;
			}

			SCOPE_CYCLE_COUNTER(STAT_Flow_RenderGrids);
			SCOPED_DRAW_EVENT(RHICmdList, FlowRenderGrids);
			{
				SCOPED_DRAW_EVENT(RHICmdList, FlowContextRenderGrids);

				NvFlow::gContext->interopBegin(RHICmdList, false);

				NvFlow::gContext->renderScene(RHICmdList, View, FlowGridSceneProxy);

				NvFlow::gContext->interopEnd(RHICmdList, false, false);
			}
			return true;
		}
	}
	return false;
}

void NvFlowDoRenderFinish(FRHICommandListImmediate& RHICmdList, const FViewInfo& View)
{
	//if (!GUsingNullRHI && NvFlow::gContext)
	//{
	//}
}

uint32 NvFlowQueryGridExportParams(FRHICommandListImmediate& RHICmdList, const ParticleSimulationParamsNvFlow& ParticleSimulationParams, uint32 MaxCount, GridExportParamsNvFlow* ResultParamsList)
{
	if (NvFlow::gContext)
	{
		uint32 Count = 0;
		for (int32 i = 0; i < NvFlow::gContext->m_sceneList.Num() && Count < MaxCount; i++)
		{
			NvFlow::Scene* Scene = NvFlow::gContext->m_sceneList[i];
			FFlowGridSceneProxy* FlowGridSceneProxy = Scene->FlowGridSceneProxy;
			if (FlowGridSceneProxy &&
				FlowGridSceneProxy->FlowGridProperties.bParticlesInteractionEnabled &&
				ParticleSimulationParams.Bounds.Intersect(FlowGridSceneProxy->GetBounds().GetBox()))
			{
				EInteractionResponseNvFlow ParticleSystemResponse = ParticleSimulationParams.ResponseToInteractionChannels.GetResponse(FlowGridSceneProxy->FlowGridProperties.InteractionChannel);
				EInteractionResponseNvFlow GridRespone = FlowGridSceneProxy->FlowGridProperties.ResponseToInteractionChannels.GetResponse(ParticleSimulationParams.InteractionChannel);

				bool GridAffectsParticleSystem =
					(ParticleSystemResponse == EIR_Receive || ParticleSystemResponse == EIR_TwoWay) &&
					(GridRespone == EIR_Produce || GridRespone == EIR_TwoWay);

				bool ParticleSystemAffectsGrid =
					(GridRespone == EIR_Receive || GridRespone == EIR_TwoWay) &&
					(ParticleSystemResponse == EIR_Produce || ParticleSystemResponse == EIR_TwoWay);

				if (GridAffectsParticleSystem && Scene->getExportParams(RHICmdList, ResultParamsList[Count]))
				{
					++Count;
				}
				if (ParticleSystemAffectsGrid)
				{
					Scene->m_particleParamsArray.Push(ParticleSimulationParams);
				}
			}
		}
		return Count;
	}
	return 0;
}

#endif
// NvFlow end
