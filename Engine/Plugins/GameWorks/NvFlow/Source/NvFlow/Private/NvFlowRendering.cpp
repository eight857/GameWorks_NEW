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

#include "Stats.h"
#include "GridAccessHooksNvFlow.h"

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
	struct Scene;

	struct Context
	{
		Context() { }
		~Context() { release(); }

		void init(FRHICommandListImmediate& RHICmdList);
		void interopBegin(FRHICommandList& RHICmdList, bool computeOnly);
		void interopEnd(FRHICommandList& RHICmdList, bool computeOnly, bool shouldFlush);

		void updateGridView(FRHICommandListImmediate& RHICmdList);
		void renderScene(FRHICommandList& RHICmdList, const FViewInfo& View, FFlowGridSceneProxy* FlowGridSceneProxy);
		void release();

		void updateScene(FRHICommandListImmediate& RHICmdList, FFlowGridSceneProxy* FlowGridSceneProxy, bool& shouldFlush);

		TArray<Scene*> m_sceneList;

		NvFlowContext* m_flowContext = nullptr;
		NvFlowDepthStencilView* m_dsv = nullptr;
		NvFlowRenderTargetView* m_rtv = nullptr;

		int m_computeMaxFramesInFlight = 2;

		bool m_computeDeviceAvailable = false;
		int m_computeFramesInFlight = 0;
		NvFlowDevice* m_computeDevice = nullptr;
		NvFlowContext* m_computeContext = nullptr;

		// deferred mechanism for proper RHI command list support
		void initDeferred(IRHICommandContext* RHICmdCtx);
		void interopBeginDeferred(IRHICommandContext* RHICmdCtx, bool computeOnly);
		void interopEndDeferred(IRHICommandContext* RHICmdCtx, bool computeOnly, bool shouldFlush);

		struct InteropBeginEndParams
		{
			Context* context;
			bool computeOnly;
			bool shouldFlush;
		};

		static void initCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx);
		static void interopBeginCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx);
		static void interopEndCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx);
	};

	struct Scene
	{
		Scene() {}
		~Scene();

		void init(Context* context, FRHICommandListImmediate& RHICmdList, FFlowGridSceneProxy* InFlowGridSceneProxy);
		void release();
		void updateParameters(FRHICommandListImmediate& RHICmdList);

		void updateSubstep(FRHICommandListImmediate& RHICmdList, float dt, uint32 substep, uint32 numSubsteps, bool& shouldFlush);
		void finilizeUpdate(FRHICommandListImmediate& RHICmdList);

		void updateGridView(FRHICommandListImmediate& RHICmdList);
		void render(FRHICommandList& RHICmdList, const FViewInfo& View);

		bool getExportParams(FRHICommandListImmediate& RHICmdList, GridExportParamsNvFlow& OutParams);


		struct CallbackUserData
		{
			NvFlow::Scene* Scene;
			IRHICommandContext* RHICmdCtx;
			float DeltaTime;
		};

		static void sEmitCustomAllocCallback(void* userdata, const NvFlowGridEmitCustomAllocParams* params)
		{
			CallbackUserData* callbackUserData = (CallbackUserData*)userdata;
			callbackUserData->Scene->emitCustomAllocCallback(callbackUserData->RHICmdCtx, params);
		}
		static void sEmitCustomEmitCallback(void* userdata, NvFlowUint* dataFrontIdx, const NvFlowGridEmitCustomEmitParams* params)
		{
			CallbackUserData* callbackUserData = (CallbackUserData*)userdata;
			callbackUserData->Scene->emitCustomEmitCallback(callbackUserData->RHICmdCtx, dataFrontIdx, params, callbackUserData->DeltaTime);
		}
		void emitCustomAllocCallback(IRHICommandContext* RHICmdCtx, const NvFlowGridEmitCustomAllocParams* params);
		void emitCustomEmitCallback(IRHICommandContext* RHICmdCtx, NvFlowUint* dataFrontIdx, const NvFlowGridEmitCustomEmitParams* params, float dt);


		bool m_multiAdapter = false;
		float m_timeSuccess = 0.f;
		float m_timeTotal = 0.f;

		float m_frameTimeSum = 0.f;
		float m_frameTimeCount = 0.f;
		float m_frameTimeAverage = 0.f;
		float m_currentAdaptiveScale = -1.f;

		NvFlowGridView* m_gridView = nullptr;

		Context* m_context = nullptr;

		NvFlowGrid* m_grid = nullptr;
		NvFlowGridProxy* m_gridProxy = nullptr;
		NvFlowColorMap* m_colorMap = nullptr;
		NvFlowVolumeRender* m_volumeRender = nullptr;

		NvFlowGridDesc m_gridDesc;
		NvFlowGridParams m_gridParams;
		NvFlowVolumeRenderParams m_renderParams;

		FFlowGridSceneProxy* FlowGridSceneProxy = nullptr;

		NvFlowGridExport* m_gridExport = nullptr;
		TArray<ParticleSimulationParamsNvFlow> m_particleParamsArray;

		// deferred mechanism for proper RHI command list support
		float m_updateSubstep_dt = 0.f;

		struct RenderParams
		{
			Scene* scene;
			NvFlowVolumeRenderParams volumeRenderParams;
		};

		void initDeferred(IRHICommandContext* RHICmdCtx);
		void updateParametersDeferred(IRHICommandContext* RHICmdCtx);
		void updateSubstepDeferred(IRHICommandContext* RHICmdCtx);
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
		TArray<Scene*>& sceneList = scene->m_context->m_sceneList;
		check(sceneList.Contains(scene));
		sceneList.RemoveSingleSwap(scene);

		delete scene;
	}
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

void NvFlow::Context::initDeferred(IRHICommandContext* RHICmdCtx)
{
	auto& appctx = *RHICmdCtx;

	m_flowContext = NvFlowInteropCreateContext(appctx);

	// register cleanup
	NvFlowCleanupFunc(appctx, cleanupContext, this);

	// create compute device if available
	// NVCHANGE: LRR - Check that the device is also dedicated to PhysX from the control panel
	bool bDedicatedPhysXGPU = true;
#if WITH_CUDA_CONTEXT
	NvIsPhysXHighSupported(bDedicatedPhysXGPU);
	UE_LOG(LogInit, Display, TEXT("NvFlow using dedicated PhysX GPU: %s"), bDedicatedPhysXGPU ? TEXT("true") : TEXT("false"));
#endif
	m_computeDeviceAvailable = NvFlowDedicatedDeviceAvailable(m_flowContext) && bDedicatedPhysXGPU;
	if (m_computeDeviceAvailable)
	{
		NvFlowDeviceDesc desc = {};
		NvFlowDeviceDescDefaults(&desc);

		m_computeDevice = NvFlowCreateDevice(m_flowContext, &desc);

		m_computeContext = NvFlowDeviceCreateContext(m_computeDevice);

		NvFlowDeviceStatus status = {};
		NvFlowDeviceUpdateContext(m_computeDevice, m_computeContext, &status);
	}
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

	NvFlowInteropUpdateContext(appctx, m_flowContext);
	if (!computeOnly)
	{
		if (m_dsv == nullptr) m_dsv = NvFlowInteropCreateDepthStencilView(appctx, m_flowContext);
		if (m_rtv == nullptr) m_rtv = NvFlowInteropCreateRenderTargetView(appctx, m_flowContext);

		NvFlowInteropUpdateDepthStencilView(appctx, m_flowContext, m_dsv);
		NvFlowInteropUpdateRenderTargetView(appctx, m_flowContext, m_rtv);
	}

	if (computeOnly && m_computeDevice)
	{
		NvFlowDeviceStatus status = {};
		NvFlowDeviceUpdateContext(m_computeDevice, m_computeContext, &status);
		m_computeFramesInFlight = status.framesInFlight;
	}

	NvFlowInteropPush(appctx, m_flowContext);
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

	if (computeOnly && m_computeDevice && shouldFlush)
	{
		NvFlowDeviceFlush(m_computeDevice);
	}

	NvFlowInteropPop(appctx, m_flowContext);
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
	// proxies and scenes should all be released by now
	check(m_sceneList.Num() == 0);

	if (m_flowContext)
	{
		UE_LOG(LogFlow, Display, TEXT("NvFlow Context Cleanup"));
	}

	if (m_rtv) NvFlowReleaseRenderTargetView(m_rtv);
	if (m_dsv) NvFlowReleaseDepthStencilView(m_dsv);
	if (m_flowContext) NvFlowReleaseContext(m_flowContext);

	if (m_computeDevice) NvFlowReleaseDevice(m_computeDevice);
	if (m_computeContext) NvFlowReleaseContext(m_computeContext);

	m_rtv = nullptr;
	m_dsv = nullptr;
	m_flowContext = nullptr;

	m_computeDevice = nullptr;
	m_computeContext = nullptr;
}

void NvFlow::Context::updateScene(FRHICommandListImmediate& RHICmdList, FFlowGridSceneProxy* FlowGridSceneProxy, bool& shouldFlush)
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
			scene->updateSubstep(RHICmdList, FlowGridSceneProxy->FlowGridProperties.SubstepSize, i, uint32(FlowGridSceneProxy->NumScheduledSubsteps), shouldFlush);
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
	if (m_context)
	{
		UE_LOG(LogFlow, Display, TEXT("NvFlow Scene %p Cleanup"), this);
	}

	if (m_gridExport) NvFlowReleaseGridExport(m_gridExport);
	if (m_grid) NvFlowReleaseGrid(m_grid);
	if (m_gridProxy) NvFlowReleaseGridProxy(m_gridProxy);
	if (m_volumeRender) NvFlowReleaseVolumeRender(m_volumeRender);
	if (m_colorMap) NvFlowReleaseColorMap(m_colorMap);

	m_gridExport = nullptr;
	m_grid = nullptr;
	m_gridProxy = nullptr;
	m_volumeRender = nullptr;
	m_colorMap = nullptr;

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

	// set initial location using proxy location
	FVector FlowOrigin = FlowGridSceneProxy->GetLocalToWorld().GetOrigin() * scaleInv;
	m_gridDesc.initialLocation = *(NvFlowFloat3*)(&FlowOrigin.X);

	bool multiAdapterEnabled = FlowGridSceneProxy->FlowGridProperties.bMultiAdapterEnabled;
	m_multiAdapter = multiAdapterEnabled && m_context->m_computeDeviceAvailable;

	// if multiAdapter, disable VTR to remove VTR induced stalls
	if (m_multiAdapter) m_gridDesc.enableVTR = false;

	NvFlowContext* computeContext = m_multiAdapter ? m_context->m_computeContext : m_context->m_flowContext;

	m_grid = NvFlowCreateGrid(computeContext, &m_gridDesc);

	NvFlowGridProxyDesc proxyDesc = {};
	proxyDesc.singleGPUMode = !m_multiAdapter;

	m_gridProxy = NvFlowCreateGridProxy(computeContext, m_grid, &proxyDesc);

	auto gridView = NvFlowGridProxyGetGridView(m_gridProxy, m_context->m_flowContext);

	NvFlowVolumeRenderDesc volumeRenderDesc;
	volumeRenderDesc.view = gridView;

	m_volumeRender = NvFlowCreateVolumeRender(m_context->m_flowContext, &volumeRenderDesc);

	NvFlowColorMapDesc colorMapDesc;
	NvFlowColorMapDescDefaults(&colorMapDesc);
	m_colorMap = NvFlowCreateColorMap(m_context->m_flowContext, &colorMapDesc);
}

void NvFlow::Scene::updateParameters(FRHICommandListImmediate& RHICmdList)
{
	auto& appctx = RHICmdList.GetContext();

	const FFlowGridProperties& Properties = FlowGridSceneProxy->FlowGridProperties;

	// configure grid params
	m_gridParams = Properties.GridParams;

	// configure render params
	NvFlowVolumeRenderParamsDefaults(&m_renderParams);
	m_renderParams.alphaScale = Properties.RenderParams.RenderingAlphaScale;
	m_renderParams.renderMode = (NvFlowVolumeRenderMode)Properties.RenderParams.RenderingMode;
	m_renderParams.debugMode = Properties.RenderParams.bDebugWireframe;
	m_renderParams.colorMapMinX = Properties.RenderParams.ColorMapMinX;
	m_renderParams.colorMapMaxX = Properties.RenderParams.ColorMapMaxX;

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

	const FFlowGridProperties& Properties = FlowGridSceneProxy->FlowGridProperties;

	// set color map
	{
		auto mapped = NvFlowColorMapMap(m_colorMap, m_context->m_flowContext);
		if (mapped.data)
		{
			check(mapped.dim == Properties.RenderParams.ColorMap.Num());
			FMemory::Memcpy(mapped.data, Properties.RenderParams.ColorMap.GetData(), sizeof(NvFlowFloat4)*Properties.RenderParams.ColorMap.Num());
			NvFlowColorMapUnmap(m_colorMap, m_context->m_flowContext);
		}
	}
}

void NvFlow::Scene::updateSubstep(FRHICommandListImmediate& RHICmdList, float dt, uint32 substep, uint32 numSubsteps, bool& shouldFlush)
{
	bool shouldUpdateGrid = (m_context->m_computeFramesInFlight < m_context->m_computeMaxFramesInFlight);

	shouldFlush = shouldFlush || (m_multiAdapter && shouldUpdateGrid);

	// update adaptive timing
	{
		const float decayRate = 0.98f;

		m_timeTotal += dt;
		if (shouldUpdateGrid) m_timeSuccess += dt;

		m_timeTotal *= decayRate;
		m_timeSuccess *= decayRate;
	}

	m_updateSubstep_dt = dt;

	if (shouldUpdateGrid)
	{
		RHICmdList.NvFlowWork(updateSubstepCallback, this, 0u);
	}
}

void NvFlow::Scene::updateSubstepDeferred(IRHICommandContext* RHICmdCtx)
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

	NvFlowContext* computeContext = m_multiAdapter ? m_context->m_computeContext : m_context->m_flowContext;

	float adaptiveDt = dt;
	// adaptive timestepping for multi-GPU
	if (m_multiAdapter)
	{
		const float clampDt = 2.f * dt;
		adaptiveDt = m_timeTotal / m_timeSuccess * dt;
		if (adaptiveDt > clampDt) adaptiveDt = clampDt;
	}

	{
		CallbackUserData callbackUserData;
		callbackUserData.Scene = this;
		callbackUserData.RHICmdCtx = RHICmdCtx;
		callbackUserData.DeltaTime = adaptiveDt;

		NvFlowGridEmitCustomRegisterAllocFunc(m_grid, &NvFlow::Scene::sEmitCustomAllocCallback, &callbackUserData);
		NvFlowGridEmitCustomRegisterEmitFunc(m_grid, eNvFlowGridChannelVelocity, &NvFlow::Scene::sEmitCustomEmitCallback, &callbackUserData);

		NvFlowGridUpdate(m_grid, computeContext, adaptiveDt);

		NvFlowGridEmitCustomRegisterAllocFunc(m_grid, nullptr, nullptr);
		NvFlowGridEmitCustomRegisterEmitFunc(m_grid, eNvFlowGridChannelVelocity, nullptr, nullptr);
	}

	NvFlowGridProxyPush(m_gridProxy, computeContext, m_grid);
}

void NvFlow::Scene::updateSubstepCallback(void* paramData, SIZE_T numBytes, IRHICommandContext* RHICmdCtx)
{
	auto scene = (NvFlow::Scene*)paramData;
	scene->updateSubstepDeferred(RHICmdCtx);
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
	NvFlowContext* computeContext = m_multiAdapter ? m_context->m_computeContext : m_context->m_flowContext;

	NvFlowGridProxyFlush(m_gridProxy, computeContext);

	m_gridView = NvFlowGridProxyGetGridView(m_gridProxy, m_context->m_flowContext);
}

void NvFlow::Scene::render(FRHICommandList& RHICmdList, const FViewInfo& View)
{
	FMatrix viewMatrix = View.ViewMatrices.ViewMatrix;
	FMatrix projMatrix = View.ViewMatrices.ProjMatrix;

	for (int j = 0; j < 3; j++)
	{
		for (int i = 0; i < 3; i++)
		{
			viewMatrix.M[j][i] *= scale;
		}
	}

	memcpy(&m_renderParams.projectionMatrix, &projMatrix.M[0][0], sizeof(m_renderParams.projectionMatrix));
	memcpy(&m_renderParams.viewMatrix, &viewMatrix.M[0][0], sizeof(m_renderParams.viewMatrix));

	m_renderParams.colorMap = m_colorMap;

#if NVFLOW_SMP
	auto& multiResConfig = View.MultiResConf;

	m_renderParams.multiRes.enabled = View.bVRProjectEnabled && (View.VRProjMode == FSceneView::EVRProjectMode::MultiRes);
	m_renderParams.multiRes.centerWidth = multiResConfig.CenterWidth;
	m_renderParams.multiRes.centerHeight = multiResConfig.CenterHeight;
	m_renderParams.multiRes.centerX = multiResConfig.CenterX;
	m_renderParams.multiRes.centerY = multiResConfig.CenterY;
	m_renderParams.multiRes.densityScaleX[0] = multiResConfig.DensityScaleX[0];
	m_renderParams.multiRes.densityScaleX[1] = multiResConfig.DensityScaleX[1];
	m_renderParams.multiRes.densityScaleX[2] = multiResConfig.DensityScaleX[2];
	m_renderParams.multiRes.densityScaleY[0] = multiResConfig.DensityScaleY[0];
	m_renderParams.multiRes.densityScaleY[1] = multiResConfig.DensityScaleY[1];
	m_renderParams.multiRes.densityScaleY[2] = multiResConfig.DensityScaleY[2];
	m_renderParams.multiRes.viewport.topLeftX = View.ViewRect.Min.X;
	m_renderParams.multiRes.viewport.topLeftY = View.ViewRect.Min.Y;
	m_renderParams.multiRes.viewport.width = View.ViewRect.Width();
	m_renderParams.multiRes.viewport.height = View.ViewRect.Height();
	m_renderParams.multiRes.nonMultiResWidth = View.NonVRProjectViewRect.Width();
	m_renderParams.multiRes.nonMultiResHeight = View.NonVRProjectViewRect.Height();

	auto& LMSConfig = View.LensMatchedShadingConf;

	m_renderParams.lensMatchedShading.enabled = View.bVRProjectEnabled && (View.VRProjMode == FSceneView::EVRProjectMode::LensMatched);
	m_renderParams.lensMatchedShading.warpLeft = LMSConfig.WarpLeft;
	m_renderParams.lensMatchedShading.warpRight = LMSConfig.WarpRight;
	m_renderParams.lensMatchedShading.warpUp = LMSConfig.WarpUp;
	m_renderParams.lensMatchedShading.warpDown = LMSConfig.WarpDown;
	m_renderParams.lensMatchedShading.sizeLeft = FMath::CeilToInt(LMSConfig.RelativeSizeLeft * View.NonVRProjectViewRect.Width());
	m_renderParams.lensMatchedShading.sizeRight = FMath::CeilToInt(LMSConfig.RelativeSizeRight * View.NonVRProjectViewRect.Width());
	m_renderParams.lensMatchedShading.sizeUp = FMath::CeilToInt(LMSConfig.RelativeSizeUp * View.NonVRProjectViewRect.Height());
	m_renderParams.lensMatchedShading.sizeDown = FMath::CeilToInt(LMSConfig.RelativeSizeDown * View.NonVRProjectViewRect.Height());
	m_renderParams.lensMatchedShading.viewport.topLeftX = View.ViewRect.Min.X;
	m_renderParams.lensMatchedShading.viewport.topLeftY = View.ViewRect.Min.Y;
	m_renderParams.lensMatchedShading.viewport.width = View.ViewRect.Width();
	m_renderParams.lensMatchedShading.viewport.height = View.ViewRect.Height();
	m_renderParams.lensMatchedShading.nonLMSWidth = View.NonVRProjectViewRect.Width();
	m_renderParams.lensMatchedShading.nonLMSHeight = View.NonVRProjectViewRect.Height();

	if (m_renderParams.lensMatchedShading.enabled)
	{
		RHICmdList.SetModifiedWMode(View.LensMatchedShadingConf, true, false);
	}

#endif

	// push work
	{
		RenderParams renderParams = {};

		renderParams.scene = this;
		renderParams.volumeRenderParams = m_renderParams;

		RHICmdList.NvFlowWork(renderCallback, &renderParams, sizeof(RenderParams));
	}

#if NVFLOW_SMP
	if (m_renderParams.lensMatchedShading.enabled)
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

	NvFlowVolumeRenderGridView(m_volumeRender, m_context->m_flowContext, m_gridView, &volumeRenderParams);
}

namespace
{
	FShaderResourceViewRHIRef NvFlowConvertSRV(IRHICommandContext& RHICmdCtx, NvFlowContext* Context, NvFlowResource* Resource)
	{
		if (Resource)
		{
			NvFlowResourceViewDesc ViewDesc;
			NvFlowUpdateResourceViewDesc(Context, Resource, &ViewDesc);

			FRHINvFlowResourceViewDesc RHIViewDesc;
			RHIViewDesc.srv = ViewDesc.srv;
			return RHICmdCtx.NvFlowCreateSRV(&RHIViewDesc);
		}
		return FShaderResourceViewRHIRef();
	}

	FShaderResourceViewRHIRef NvFlowConvertSRV(IRHICommandContext& RHICmdCtx, NvFlowContext* Context, NvFlowResourceRW* ResourceRW)
	{
		if (ResourceRW)
		{
			NvFlowResourceRWViewDesc ViewDesc;
			NvFlowUpdateResourceRWViewDesc(Context, ResourceRW, &ViewDesc);

			FRHINvFlowResourceViewDesc RHIViewDesc;
			RHIViewDesc.srv = ViewDesc.resourceView.srv;
			return RHICmdCtx.NvFlowCreateSRV(&RHIViewDesc);
		}
		return FShaderResourceViewRHIRef();
	}

	FUnorderedAccessViewRHIRef NvFlowConvertUAV(IRHICommandContext& RHICmdCtx, NvFlowContext* Context, NvFlowResourceRW* ResourceRW)
	{
		if (ResourceRW)
		{
			NvFlowResourceRWViewDesc ViewDesc;
			NvFlowUpdateResourceRWViewDesc(Context, ResourceRW, &ViewDesc);

			FRHINvFlowResourceRWViewDesc RHIViewDesc;
			RHIViewDesc.uav = ViewDesc.uav;
			return RHICmdCtx.NvFlowCreateUAV(&RHIViewDesc);
		}
		return FUnorderedAccessViewRHIRef();
	}

	inline FIntVector NvFlowConvert(const NvFlowUint4& in)
	{
		return FIntVector(in.x, in.y, in.z);
	}

	inline FVector NvFlowConvert(const NvFlowFloat4& in)
	{
		return FVector(in.x, in.y, in.z);
	}


	inline FMatrix NvFlowGetWorldToVolume(FFlowGridSceneProxy* FlowGridSceneProxy)
	{
		const FBoxSphereBounds& LocalBounds = FlowGridSceneProxy->GetLocalBounds();
		const FMatrix& LocalToWorld = FlowGridSceneProxy->GetLocalToWorld();

		FMatrix VolumeToLocal = FMatrix(
			FPlane(LocalBounds.BoxExtent.X * 2, 0.0f, 0.0f, 0.0f),
			FPlane(0.0f, LocalBounds.BoxExtent.Y * 2, 0.0f, 0.0f),
			FPlane(0.0f, 0.0f, LocalBounds.BoxExtent.Z * 2, 0.0f),
			FPlane(LocalBounds.Origin - LocalBounds.BoxExtent, 1.0f));

		FMatrix VolumeToWorld = VolumeToLocal * LocalToWorld;
		return VolumeToWorld.Inverse();
	}
}

bool NvFlow::Scene::getExportParams(FRHICommandListImmediate& RHICmdList, GridExportParamsNvFlow& OutParams)
{
	if (m_gridExport == nullptr)
	{
		if (m_gridView == nullptr)
		{
			return false;
		}

		NvFlowGridExportDesc Desc;
		Desc.gridView = m_gridView;

		m_gridExport = NvFlowCreateGridExport(m_context->m_flowContext, &Desc);
	}


	NvFlowGridExportUpdate(m_gridExport, m_context->m_flowContext, m_gridView, eNvFlowGridChannelVelocity);

	NvFlowGridExportView GridExportView;
	NvFlowGridExportGetView(m_gridExport, m_context->m_flowContext, &GridExportView, eNvFlowGridChannelVelocity);

	OutParams.DataSRV = NvFlowConvertSRV(RHICmdList.GetContext(), m_context->m_flowContext, GridExportView.data);
	OutParams.BlockTableSRV = NvFlowConvertSRV(RHICmdList.GetContext(), m_context->m_flowContext, GridExportView.blockTable);

	OutParams.BlockDim = NvFlowConvert(GridExportView.shaderParams.blockDim);
	OutParams.BlockDimBits = NvFlowConvert(GridExportView.shaderParams.blockDimBits);
	OutParams.BlockDimInv = NvFlowConvert(GridExportView.shaderParams.blockDimInv);
	OutParams.LinearBlockDim = NvFlowConvert(GridExportView.shaderParams.linearBlockDim);
	OutParams.LinearBlockOffset = NvFlowConvert(GridExportView.shaderParams.linearBlockOffset);
	OutParams.DimInv = NvFlowConvert(GridExportView.shaderParams.dimInv);
	OutParams.VDim = NvFlowConvert(GridExportView.shaderParams.vdim);
	OutParams.VDimInv = NvFlowConvert(GridExportView.shaderParams.vdimInv);
	OutParams.PoolGridDim = NvFlowConvert(GridExportView.shaderParams.poolGridDim);
	OutParams.GridDim = NvFlowConvert(GridExportView.shaderParams.gridDim);
	OutParams.IsVTR = (GridExportView.shaderParams.isVTR.x != 0);

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


void NvFlow::Scene::emitCustomAllocCallback(IRHICommandContext* RHICmdCtx, const NvFlowGridEmitCustomAllocParams* params)
{
	if (m_particleParamsArray.Num() > 0)
	{
		NvFlowContextPop(m_context->m_flowContext);

		TShaderMapRef<FNvFlowMaskFromParticlesCS> MaskFromParticlesCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		RHICmdCtx->RHISetComputeShader(MaskFromParticlesCS->GetComputeShader());

		FNvFlowMaskFromParticlesParameters MaskFromParticlesParameters;
		MaskFromParticlesParameters.WorldToVolume = NvFlowGetWorldToVolume(FlowGridSceneProxy);
		MaskFromParticlesParameters.MaskDim = FIntVector(params->maskDim.x, params->maskDim.y, params->maskDim.z);

		FUnorderedAccessViewRHIRef MaskUAV = NvFlowConvertUAV(*RHICmdCtx, m_context->m_flowContext, params->maskResourceRW);

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

		NvFlowContextPush(m_context->m_flowContext);
	}
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
		FNvFlowCopyGridDataUniformBufferRef& UniformBuffer,
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

void NvFlow::Scene::emitCustomEmitCallback(IRHICommandContext* RHICmdCtx, NvFlowUint* dataFrontIdx, const NvFlowGridEmitCustomEmitParams* params, float dt)
{
	if (m_particleParamsArray.Num() > 0 && params->numBlocks > 0)
	{
		NvFlowContextPop(m_context->m_flowContext);
		{
			TShaderMapRef<FNvFlowCopyGridDataCS> CopyGridDataCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			RHICmdCtx->RHISetComputeShader(CopyGridDataCS->GetComputeShader());

			FNvFlowCopyGridDataParameters CopyGridDataParameters;
			CopyGridDataParameters.BlockDim = NvFlowConvert(params->shaderParams.blockDim);
			CopyGridDataParameters.BlockDimBits = NvFlowConvert(params->shaderParams.blockDimBits);
			CopyGridDataParameters.IsVTR = params->shaderParams.isVTR.x;
			CopyGridDataParameters.ThreadDim = CopyGridDataParameters.BlockDim;
			CopyGridDataParameters.ThreadDim.X *= params->numBlocks;
			FNvFlowCopyGridDataUniformBufferRef UniformBuffer = FNvFlowCopyGridDataUniformBufferRef::CreateUniformBufferImmediate(CopyGridDataParameters, UniformBuffer_SingleFrame);

			uint32 GroupCountX = (CopyGridDataParameters.ThreadDim.X + COPY_THREAD_COUNT_X - 1) / COPY_THREAD_COUNT_X;
			uint32 GroupCountY = (CopyGridDataParameters.ThreadDim.Y + COPY_THREAD_COUNT_Y - 1) / COPY_THREAD_COUNT_Y;
			uint32 GroupCountZ = (CopyGridDataParameters.ThreadDim.Z + COPY_THREAD_COUNT_Z - 1) / COPY_THREAD_COUNT_Z;

			FShaderResourceViewRHIRef BlockListSRV = NvFlowConvertSRV(*RHICmdCtx, m_context->m_flowContext, params->blockList);
			FShaderResourceViewRHIRef BlockTableSRV = NvFlowConvertSRV(*RHICmdCtx, m_context->m_flowContext, params->blockTable);
			FShaderResourceViewRHIRef DataInSRV = NvFlowConvertSRV(*RHICmdCtx, m_context->m_flowContext, params->dataRW[*dataFrontIdx]);
			FUnorderedAccessViewRHIRef DataOutUAV = NvFlowConvertUAV(*RHICmdCtx, m_context->m_flowContext, params->dataRW[*dataFrontIdx ^ 1]);

			CopyGridDataCS->SetOutput(RHICmdCtx, DataOutUAV);
			CopyGridDataCS->SetParameters(RHICmdCtx, UniformBuffer, BlockListSRV, BlockTableSRV, DataInSRV);
			//DispatchComputeShader(RHICmdCtx, *CopyGridDataCS, GroupCountX, GroupCountY, GroupCountZ);
			RHICmdCtx->RHIDispatchComputeShader(GroupCountX, GroupCountY, GroupCountZ);
			CopyGridDataCS->UnbindBuffers(RHICmdCtx);

			*dataFrontIdx ^= 1;
		}

		TShaderMapRef<FNvFlowCoupleParticlesCS> CoupleParticlesCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		RHICmdCtx->RHISetComputeShader(CoupleParticlesCS->GetComputeShader());
		
		FNvFlowCoupleParticlesParameters CoupleParticlesParameters;
		CoupleParticlesParameters.WorldToVolume = NvFlowGetWorldToVolume(FlowGridSceneProxy);
		CoupleParticlesParameters.VDim = FIntVector(
			params->shaderParams.blockDim.x * params->shaderParams.gridDim.x,
			params->shaderParams.blockDim.y * params->shaderParams.gridDim.y,
			params->shaderParams.blockDim.z * params->shaderParams.gridDim.z);
		CoupleParticlesParameters.BlockDim = NvFlowConvert(params->shaderParams.blockDim);
		CoupleParticlesParameters.BlockDimBits = NvFlowConvert(params->shaderParams.blockDimBits);
		CoupleParticlesParameters.IsVTR = params->shaderParams.isVTR.x;

		CoupleParticlesParameters.AccelRate = dt / FlowGridSceneProxy->FlowGridProperties.ParticleToGridAccelTimeConstant;
		CoupleParticlesParameters.DecelRate = dt / FlowGridSceneProxy->FlowGridProperties.ParticleToGridDecelTimeConstant;
		CoupleParticlesParameters.Threshold = FlowGridSceneProxy->FlowGridProperties.ParticleToGridThresholdMultiplier;

		CoupleParticlesParameters.InvVelocityScale = 1.0f / scale;

		FShaderResourceViewRHIRef BlockTableSRV = NvFlowConvertSRV(*RHICmdCtx, m_context->m_flowContext, params->blockTable);
		FShaderResourceViewRHIRef DataInSRV = NvFlowConvertSRV(*RHICmdCtx, m_context->m_flowContext, params->dataRW[*dataFrontIdx]);
		FUnorderedAccessViewRHIRef DataOutUAV = NvFlowConvertUAV(*RHICmdCtx, m_context->m_flowContext, params->dataRW[*dataFrontIdx ^ 1]);

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

		*dataFrontIdx ^= 1;

		NvFlowContextPush(m_context->m_flowContext);
	}
}

// ---------------- global interface functions ---------------------

void NvFlowUpdateScene(FRHICommandListImmediate& RHICmdList, TArray<FPrimitiveSceneInfo*>& Primitives)
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

		NvFlow::gContext->interopBegin(RHICmdList, true);

		// look for FFlowGridSceneProxy, TODO replace with adding special member to FScene
		FFlowGridSceneProxy* FlowGridSceneProxy = nullptr;
		for (int32 i = 0; i < Primitives.Num(); i++)
		{
			FPrimitiveSceneInfo* PrimitiveSceneInfo = Primitives[i];
			if (PrimitiveSceneInfo->Proxy->FlowData.bFlowGrid)
			{
				FlowGridSceneProxy = (FFlowGridSceneProxy*)PrimitiveSceneInfo->Proxy;
				NvFlow::gContext->updateScene(RHICmdList, FlowGridSceneProxy, shouldFlush);
			}
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
}

bool NvFlowDoRenderPrimitive(FRHICommandList& RHICmdList, const FViewInfo& View, FPrimitiveSceneInfo* PrimitiveSceneInfo)
{
	if (!GUsingNullRHI && NvFlow::gContext)
	{
		if (PrimitiveSceneInfo->Proxy->FlowData.bFlowGrid)
		{
			FFlowGridSceneProxy* FlowGridSceneProxy = (FFlowGridSceneProxy*)PrimitiveSceneInfo->Proxy;
			if (FlowGridSceneProxy->FlowGridProperties.bEnableParticleMode != 0 && 
				FlowGridSceneProxy->FlowGridProperties.RenderParams.bDebugWireframe == 0)
			{
				return false;
			}

			SCOPE_CYCLE_COUNTER(STAT_Flow_RenderGrids);
			SCOPED_DRAW_EVENT(RHICmdList, FlowRenderGrids);

			NvFlow::gContext->interopBegin(RHICmdList, false);

			NvFlow::gContext->renderScene(RHICmdList, View, FlowGridSceneProxy);

			NvFlow::gContext->interopEnd(RHICmdList, false, false);

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
				FlowGridSceneProxy->FlowGridProperties.bEnableParticlesInteraction &&
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
