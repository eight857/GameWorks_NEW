#include "NvFlowPCH.h"

// NvFlow begin
#if WITH_NVFLOW

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

#if NVFLOW_ADAPTIVE
// For HMD support
#include "IHeadMountedDisplay.h"
#endif

#include "NvFlow.h"
#include "NvFlowContext.h"

#include "FlowGridSceneProxy.h"

// For dedicate GPU support
#if WITH_CUDA_CONTEXT
#include "PhysicsPublic.h"
#endif

DEFINE_STAT(STAT_Flow_SimulateGrids);
DEFINE_STAT(STAT_Flow_RenderGrids);

namespace NvFlow
{
	const float scale = 100.f;
	const float scaleInv = 1.0f / scale;
	const float sdfRadius = 0.8f;
	const float angularScale = PI / 180.f;

	struct Scene;

	struct Context
	{
		Context() {}
		~Context() { release(); }

		void init(FRHICommandListImmediate& RHICmdList);
		void interopBegin(FRHICommandListImmediate& RHICmdList, bool computeOnly);
		void interopEnd(FRHICommandListImmediate& RHICmdList, bool computeOnly, bool shouldFlush);
		void updateGridView(FRHICommandListImmediate& RHICmdList);
		void render(FRHICommandListImmediate& RHICmdList, const FViewInfo& View);
		void release();

		void updateScene(FRHICommandListImmediate& RHICmdList, FFlowGridSceneProxy* FlowGridSceneProxy, bool& shouldFlush);

		FVector getShapeUnitToActualScale(EFlowGeometryType geometryType, const FFlowShape::FGeometry& Geometry);

		void getShape(NvFlowShapeDesc& shapeDesc, NvFlowGridEmitParams& emitParams, EFlowGeometryType geometryType, const FFlowShape::FGeometry& Geometry);

		NvFlowContext* m_context = nullptr;
		NvFlowDepthStencilView* m_dsv = nullptr;
		NvFlowRenderTargetView* m_rtv = nullptr;

		int m_computeMaxFramesInFlight = 2;

		bool m_computeDeviceAvailable = false;
		int m_computeFramesInFlight = 0;
		NvFlowDevice* m_computeDevice = nullptr;
		NvFlowContext* m_computeContext = nullptr;

		TArray<Scene*> m_sceneList;
	};

	struct Scene
	{
		Scene() {}
		~Scene();

		void init(Context* context, FRHICommandListImmediate& RHICmdList, FFlowGridSceneProxy* InFlowGridSceneProxy);
		void release();
		void updateParameters(FRHICommandListImmediate& RHICmdList);
		void emit(const FFlowGridProperties& Properties, const FFlowEmitter& Emitter, float dt, uint32 substep, uint32 numSubsteps);
		void collide(const FFlowGridProperties& Properties, const FFlowShape& Collider, float dt, uint32 substep, uint32 numSubsteps);

		void updateSubstep(FRHICommandListImmediate& RHICmdList, float dt, uint32 substep, uint32 numSubsteps, bool& shouldFlush);
		void updateGridView(FRHICommandListImmediate& RHICmdList);
		void render(FRHICommandListImmediate& RHICmdList, const FViewInfo& View);

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
	auto& appctx = RHICmdList.GetContext();

	m_context = NvFlowInteropCreateContext(appctx);

	// register cleanup
	NvFlowCleanupFunc(appctx, cleanupContext, this);

	// create compute device if available
	// NVCHANGE: LRR - Check that the device is also dedicated to PhysX from the control panel
	bool bDedicatedPhysXGPU = true;
#if WITH_CUDA_CONTEXT
	NvIsPhysXHighSupported(bDedicatedPhysXGPU);
	UE_LOG(LogInit, Display, TEXT("NvFlow using dedicated PhysX GPU: %s"), bDedicatedPhysXGPU ? TEXT("true") : TEXT("false"));
#endif
	m_computeDeviceAvailable = NvFlowDedicatedDeviceAvailable(m_context) && bDedicatedPhysXGPU;
	if (m_computeDeviceAvailable)
	{
		NvFlowDeviceDesc desc = {};
		NvFlowDeviceDescDefaults(&desc);

		m_computeDevice = NvFlowCreateDevice(m_context, &desc);

		m_computeContext = NvFlowDeviceCreateContext(m_computeDevice);

		NvFlowDeviceStatus status = {};
		NvFlowDeviceUpdateContext(m_computeDevice, m_computeContext, &status);
	}
}

void NvFlow::Context::interopBegin(FRHICommandListImmediate& RHICmdList, bool computeOnly)
{
	auto& appctx = RHICmdList.GetContext();

	NvFlowInteropUpdateContext(appctx, m_context);
	if (!computeOnly)
	{
		if(m_dsv == nullptr) m_dsv = NvFlowInteropCreateDepthStencilView(appctx, m_context);
		if(m_rtv == nullptr) m_rtv = NvFlowInteropCreateRenderTargetView(appctx, m_context);

		NvFlowInteropUpdateDepthStencilView(appctx, m_context, m_dsv);
		NvFlowInteropUpdateRenderTargetView(appctx, m_context, m_rtv);
	}

	if (computeOnly && m_computeDevice)
	{
		NvFlowDeviceStatus status = {};
		NvFlowDeviceUpdateContext(m_computeDevice, m_computeContext, &status);
		m_computeFramesInFlight = status.framesInFlight;
	}

	NvFlowInteropPush(appctx, m_context);
}

void NvFlow::Context::interopEnd(FRHICommandListImmediate& RHICmdList, bool computeOnly, bool shouldFlush)
{
	auto& appctx = RHICmdList.GetContext();

	if (computeOnly && m_computeDevice && shouldFlush)
	{
		NvFlowDeviceFlush(m_computeDevice);
	}

	NvFlowInteropPop(appctx, m_context);
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

void NvFlow::Context::render(FRHICommandListImmediate& RHICmdList, const FViewInfo& View)
{
	// iterate scenes and render
	for (int32 i = 0; i < m_sceneList.Num(); i++)
	{
		Scene* scene = m_sceneList[i];
		scene->render(RHICmdList, View);
	}
}

void NvFlow::Context::release()
{
	// proxies and scenes should all be released by now
	check(m_sceneList.Num() == 0);

	if (m_rtv) NvFlowReleaseRenderTargetView(m_rtv);
	if (m_dsv) NvFlowReleaseDepthStencilView(m_dsv);
	if (m_context) NvFlowReleaseContext(m_context);

	if (m_computeDevice) NvFlowReleaseDevice(m_computeDevice);
	if (m_computeContext) NvFlowReleaseContext(m_computeContext);

	m_rtv = nullptr;
	m_dsv = nullptr;
	m_context = nullptr;

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
}

FVector NvFlow::Context::getShapeUnitToActualScale(EFlowGeometryType geometryType, const FFlowShape::FGeometry& Geometry)
{
	FVector UnitToActualScale;
	switch (geometryType)
	{
		case EFGT_eSphere:
		{
			UnitToActualScale = FVector(Geometry.Sphere.Radius * (1.f / sdfRadius) * scaleInv);
			break;
		}
		case EFGT_eBox:
		{
			UnitToActualScale = *(FVector*)(&Geometry.Box.Extends[0]) * (1.f / sdfRadius) * scaleInv;
			break;
		}
		case EFGT_eCapsule:
		{
			UnitToActualScale = FVector(Geometry.Capsule.Radius * (1.f / sdfRadius) * scaleInv);
			break;
		}
		case EFGT_eConvex:
		{
			UnitToActualScale = FVector(Geometry.Convex.Radius * (1.f / sdfRadius) * scaleInv);
			break;
		}
	};
	return UnitToActualScale;
}

void NvFlow::Context::getShape(NvFlowShapeDesc& shapeDesc, NvFlowGridEmitParams& emitParams, EFlowGeometryType geometryType, const FFlowShape::FGeometry& Geometry)
{
	emitParams.localToWorld = emitParams.bounds;

	switch (geometryType)
	{
		case EFGT_eSphere:
		{
			emitParams.shapeType = eNvFlowShapeTypeSphere;
			shapeDesc.sphere.radius = sdfRadius;
			break;
		}
		case EFGT_eBox:
		{
			emitParams.shapeType = eNvFlowShapeTypeBox;
			shapeDesc.box.halfSize.x = sdfRadius;
			shapeDesc.box.halfSize.y = sdfRadius;
			shapeDesc.box.halfSize.z = sdfRadius;
			break;
		}
		case EFGT_eCapsule:
		{
			emitParams.shapeType = eNvFlowShapeTypeCapsule;
			shapeDesc.capsule.radius = sdfRadius;
			shapeDesc.capsule.length = sdfRadius * (2.f * Geometry.Capsule.HalfHeight / Geometry.Capsule.Radius);
			break;
		}
		case EFGT_eConvex:
		{
			emitParams.shapeType = eNvFlowShapeTypePlane;
			break;
		}
	};

	// for boxes, do distortion correction
	if (emitParams.shapeType == eNvFlowShapeTypeBox)
	{
		// compute emitter aspect ratio
		auto& mat = emitParams.bounds;
		NvFlowFloat3 aspectRatio = {
			FMath::Sqrt(mat.x.x * mat.x.x + mat.x.y * mat.x.y + mat.x.z * mat.x.z),
			FMath::Sqrt(mat.y.x * mat.y.x + mat.y.y * mat.y.y + mat.y.z * mat.y.z),
			FMath::Sqrt(mat.z.x * mat.z.x + mat.z.y * mat.z.y + mat.z.z * mat.z.z),
		};
		float aspectRatioMin = FMath::Min(aspectRatio.x, FMath::Min(aspectRatio.y, aspectRatio.z));
		aspectRatio.x /= aspectRatioMin;
		aspectRatio.y /= aspectRatioMin;
		aspectRatio.z /= aspectRatioMin;

		// divide off aspectRatio on localToWorld
		emitParams.localToWorld.x.x /= aspectRatio.x;
		emitParams.localToWorld.x.y /= aspectRatio.x;
		emitParams.localToWorld.x.z /= aspectRatio.x;
		emitParams.localToWorld.y.x /= aspectRatio.y;
		emitParams.localToWorld.y.y /= aspectRatio.y;
		emitParams.localToWorld.y.z /= aspectRatio.y;
		emitParams.localToWorld.z.x /= aspectRatio.z;
		emitParams.localToWorld.z.y /= aspectRatio.z;
		emitParams.localToWorld.z.z /= aspectRatio.z;

		// scale box halfSize instead
		shapeDesc.box.halfSize.x *= aspectRatio.x;
		shapeDesc.box.halfSize.y *= aspectRatio.y;
		shapeDesc.box.halfSize.z *= aspectRatio.z;
	}

	// for capsules, extend bounds on x
	if (emitParams.shapeType == eNvFlowShapeTypeCapsule)
	{
		float scale = (0.5f * shapeDesc.capsule.length + 1.f);

		// scale on x
		emitParams.bounds.x.x *= scale;
		emitParams.bounds.x.y *= scale;
		emitParams.bounds.x.z *= scale;
	}
}

// ------------------ NvFlow::Scene -----------------

NvFlow::Scene::~Scene()
{
	release();
}

void NvFlow::Scene::release()
{
	if (m_grid) NvFlowReleaseGrid(m_grid);
	if (m_gridProxy) NvFlowReleaseGridProxy(m_gridProxy);
	if (m_volumeRender) NvFlowReleaseVolumeRender(m_volumeRender);
	if (m_colorMap) NvFlowReleaseColorMap(m_colorMap);

	m_grid = nullptr;
	m_gridProxy = nullptr;
	m_volumeRender = nullptr;
	m_colorMap = nullptr;

	m_context = nullptr;

	FlowGridSceneProxy = nullptr;
}

void NvFlow::Scene::init(Context* context, FRHICommandListImmediate& RHICmdList, FFlowGridSceneProxy* InFlowGridSceneProxy)
{
	m_context = context;
	FlowGridSceneProxy = InFlowGridSceneProxy;
	FlowGridSceneProxy->scenePtr = this;
	FlowGridSceneProxy->cleanupSceneFunc = NvFlow::cleanupScene;

	auto& appctx = RHICmdList.GetContext();

	// set grid defaults
	NvFlowGridDescDefaults(&m_gridDesc);

	// configure grid desc
	FVector FlowOrigin = FlowGridSceneProxy->GetLocalToWorld().GetOrigin() * scaleInv;
	m_gridDesc.initialLocation = *(NvFlowFloat3*)(&FlowOrigin.X);
	FVector FlowHalfSize = FlowGridSceneProxy->FlowGridProperties.HalfSize * scaleInv;
	m_gridDesc.halfSize = *(NvFlowFloat3*)(&FlowHalfSize.X);
	m_gridDesc.virtualDim = *(NvFlowDim*)(&FlowGridSceneProxy->FlowGridProperties.VirtualDim.X);
	m_gridDesc.residentScale *= FlowGridSceneProxy->FlowGridProperties.MemoryLimitScale;

	bool multiAdapterEnabled = FlowGridSceneProxy->FlowGridProperties.bMultiAdapterEnabled;
	m_multiAdapter = multiAdapterEnabled && context->m_computeDeviceAvailable;

	// if multiAdapter, disable VTR to remove VTR induced stalls
	if (m_multiAdapter) m_gridDesc.enableVTR = false;

	NvFlowContext* computeContext = m_multiAdapter ? context->m_computeContext : context->m_context;

	m_grid = NvFlowCreateGrid(computeContext, &m_gridDesc);

	NvFlowGridProxyDesc proxyDesc = {};
	proxyDesc.singleGPUMode = !m_multiAdapter;

	m_gridProxy = NvFlowCreateGridProxy(computeContext, m_grid, &proxyDesc);

	auto gridView = NvFlowGridProxyGetGridView(m_gridProxy, context->m_context);

	NvFlowVolumeRenderDesc volumeRenderDesc;
	volumeRenderDesc.downsampleFactor = eNvFlowVolumeRenderDownsample2x2;
	volumeRenderDesc.view = gridView;

	m_volumeRender = NvFlowCreateVolumeRender(m_context->m_context, &volumeRenderDesc);

	NvFlowColorMapDesc colorMapDesc;
	NvFlowColorMapDescDefaults(&colorMapDesc);
	m_colorMap = NvFlowCreateColorMap(m_context->m_context, &colorMapDesc);
}

void NvFlow::Scene::updateParameters(FRHICommandListImmediate& RHICmdList)
{
	auto& appctx = RHICmdList.GetContext();

	const FFlowGridProperties& Properties = FlowGridSceneProxy->FlowGridProperties;

	// configure grid params
	NvFlowGridParamsDefaults(&m_gridParams);

	m_gridParams.velocityWeight = Properties.VelocityWeight;
	m_gridParams.densityWeight = Properties.DensityWeight;
	m_gridParams.tempWeight = Properties.TempWeight;
	m_gridParams.fuelWeight = Properties.FuelWeight;
	m_gridParams.velocityThreshold = Properties.VelocityThreshold;
	m_gridParams.densityThreshold = Properties.DensityThreshold;
	m_gridParams.tempThreshold = Properties.TempThreshold;
	m_gridParams.fuelThreshold = Properties.FuelThreshold;
	m_gridParams.importanceThreshold = Properties.ImportanceThreshold;

	m_gridParams.velocityDamping = Properties.VelocityDamping;
	m_gridParams.densityDamping = Properties.DensityDamping;
	m_gridParams.velocityFade = Properties.VelocityFade;
	m_gridParams.densityFade = Properties.DensityFade;
	m_gridParams.velocityMacCormackBlendFactor = Properties.VelocityMacCormackBlendFactor;
	m_gridParams.densityMacCormackBlendFactor = Properties.DensityMacCormackBlendFactor;
	m_gridParams.vorticityStrength = Properties.VorticityStrength;
	m_gridParams.combustion.ignitionTemp = Properties.CombustionIgnitionTemperature;
	m_gridParams.combustion.coolingRate = Properties.CombustionCoolingRate;
	FVector ScaledGravity(Properties.Gravity * scaleInv);
	m_gridParams.gravity = *(NvFlowFloat3*)(&ScaledGravity.X);

	// configure render params
	NvFlowVolumeRenderParamsDefaults(&m_renderParams);
	m_renderParams.alphaScale = Properties.RenderingAlphaScale;
	m_renderParams.renderMode = Properties.RenderingMode;
	m_renderParams.debugMode = Properties.bDebugWireframe;
	m_renderParams.colorMapMinX = Properties.ColorMapMinX;
	m_renderParams.colorMapMaxX = Properties.ColorMapMaxX;

#if NVFLOW_ADAPTIVE
	// adaptive screen percentage
	bool bHMDConnected = (GEngine && GEngine->HMDDevice.IsValid() && GEngine->HMDDevice->IsHMDConnected());
	if (Properties.bAdaptiveScreenPercentage && bHMDConnected)
	{
		const float decayRate = 0.98f;
		const float reactRate = 0.002f;
		const float recoverRate = 0.001f;

		if (m_currentAdaptiveScale < 0.f) m_currentAdaptiveScale = Properties.MaxScreenPercentage;

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

		float targetFrameTime = Properties.AdaptiveTargetFrameTime;

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
		if (m_currentAdaptiveScale < Properties.MinScreenPercentage)
		{
			m_currentAdaptiveScale = Properties.MinScreenPercentage;
		}
		if (m_currentAdaptiveScale > Properties.MaxScreenPercentage)
		{
			m_currentAdaptiveScale = Properties.MaxScreenPercentage;
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
		m_renderParams.screenPercentage = Properties.MaxScreenPercentage;
	}

	// set color map
	{
		auto mapped = NvFlowColorMapMap(m_colorMap, m_context->m_context);
		if (mapped.data)
		{
			check(mapped.dim == Properties.ColorMap.Num());
			FMemory::Memcpy(mapped.data, Properties.ColorMap.GetData(), sizeof(NvFlowFloat4)*Properties.ColorMap.Num());
			NvFlowColorMapUnmap(m_colorMap, m_context->m_context);
		}
	}
}

void NvFlow::Scene::emit(const FFlowGridProperties& Properties, const FFlowEmitter& Emitter, float dt, uint32 substep, uint32 numSubsteps)
{	
	FVector UnitToActualScale = m_context->getShapeUnitToActualScale(Emitter.Shape.GeometryType, Emitter.Shape.Geometry);
	NvFlowGridEmitParams emitParams;
	
	NvFlowGridEmitParamsDefaults(&emitParams);
	FVector ScaledVelocityLinear = Emitter.Shape.LinearVelocity * scaleInv;
	FVector ScaledVelocityAngular = Emitter.Shape.AngularVelocity * angularScale;
	emitParams.velocityLinear = *(NvFlowFloat3*)(&ScaledVelocityLinear.X);
	emitParams.velocityAngular = *(NvFlowFloat3*)(&ScaledVelocityAngular.X);
	emitParams.fuel = Emitter.Fuel;
	emitParams.fuelReleaseTemp = Emitter.FuelReleaseTemp;
	emitParams.fuelRelease = Emitter.FuelRelease;
	emitParams.density = Emitter.Density;
	emitParams.temperature = Emitter.Temperature;
	emitParams.allocationPredict = Emitter.AllocationPredict;
	emitParams.allocationScale = { 
		Emitter.AllocationScale, 
		Emitter.AllocationScale, 
		Emitter.AllocationScale 
	};

	float coupleRate = Emitter.CoupleRate;
	float velocityCoupleRate = coupleRate * Emitter.VelocityMask;
	emitParams.velocityCoupleRate = { velocityCoupleRate, velocityCoupleRate, velocityCoupleRate };
	emitParams.fuelCoupleRate = coupleRate * Emitter.FuelMask;
	emitParams.temperatureCoupleRate = coupleRate * Emitter.TemperatureMask;
	emitParams.densityCoupleRate = coupleRate * Emitter.DensityMask;

	// Optimization: kick out early if couple rate is zero
	if (coupleRate <= 0.f)
	{
		return;
	}

	float CollisionFactor = Emitter.CollisionFactor;
	float EmitterInflate = Emitter.EmitterInflate;

	emitParams.maxActiveDist = EmitterInflate;
	emitParams.minActiveDist = -1.f + CollisionFactor;
	
	float emitterSubstepDt = dt / Emitter.NumSubsteps;

	//whole step start
	FTransform PreviousTransform = Emitter.Shape.PreviousTransform;
	PreviousTransform.SetLocation(Emitter.Shape.PreviousTransform.GetLocation() * scaleInv);
	PreviousTransform.SetScale3D(PreviousTransform.GetScale3D() * UnitToActualScale);

	//whole step end
	FTransform CurrentTransform = Emitter.Shape.Transform;
	CurrentTransform.SetLocation(Emitter.Shape.Transform.GetLocation() * scaleInv);
	CurrentTransform.SetScale3D(CurrentTransform.GetScale3D() * UnitToActualScale);

	// check if transforms are close enough to equal
	bool isStationary = CurrentTransform.Equals(PreviousTransform);
	uint32 iterations = isStationary ? 1u : Emitter.NumSubsteps;
	emitParams.numSubSteps = isStationary ? Emitter.NumSubsteps : 1u;

	for (uint32 i = 0; i < iterations; i++)
	{
		FTransform BlendedTransform;
		float alpha = float(substep*Emitter.NumSubsteps + i + 1) / (Emitter.NumSubsteps*numSubsteps);
		BlendedTransform.Blend(PreviousTransform, CurrentTransform, alpha);
		
		// compute centerOfMass in emitter local space
		FVector centerOfMass = scaleInv * Emitter.Shape.CenterOfRotationOffset;
		centerOfMass = BlendedTransform.InverseTransformPosition(centerOfMass);
		emitParams.centerOfMass = *(NvFlowFloat3*)(&centerOfMass.X);

		emitParams.bounds = *(NvFlowFloat4x4*)(&BlendedTransform.ToMatrixWithScale().M[0][0]);

		NvFlowShapeDesc shapeDesc;
		m_context->getShape(shapeDesc, emitParams, Emitter.Shape.GeometryType, Emitter.Shape.Geometry);

		// scale bounds as a function of emitter inflate
		{
			const float k = (EmitterInflate + 1.f);
			emitParams.bounds.x.x *= k;
			emitParams.bounds.x.y *= k;
			emitParams.bounds.x.z *= k;
			emitParams.bounds.y.x *= k;
			emitParams.bounds.y.y *= k;
			emitParams.bounds.y.z *= k;
			emitParams.bounds.z.x *= k;
			emitParams.bounds.z.y *= k;
			emitParams.bounds.z.z *= k;
		}

		emitParams.deltaTime = isStationary ? dt : emitterSubstepDt;

		if (emitParams.shapeType != eNvFlowShapeTypePlane)
		{
			NvFlowGridEmit(m_grid, &shapeDesc, 1u, &emitParams, 1u);
		}
		else
		{
			auto& FlowConvexParams = Emitter.Shape.ConvexParams;

			// scale bounds
			{
				FVector scale = (FlowConvexParams.Scale * 0.5f * (FlowConvexParams.LocalMax - FlowConvexParams.LocalMin));
				float norm = scale.GetAbsMax();
				scale /= norm;
				emitParams.bounds.x.x *= scale.X;
				emitParams.bounds.x.y *= scale.X;
				emitParams.bounds.x.z *= scale.X;
				emitParams.bounds.y.x *= scale.Y;
				emitParams.bounds.y.y *= scale.Y;
				emitParams.bounds.y.z *= scale.Y;
				emitParams.bounds.z.x *= scale.Z;
				emitParams.bounds.z.y *= scale.Z;
				emitParams.bounds.z.z *= scale.Z;
			}

			// scale local to world
			{
				auto matNoScale = BlendedTransform.ToMatrixNoScale();
				emitParams.localToWorld = *(NvFlowFloat4x4*)(&matNoScale.M[0][0]);
				emitParams.localToWorld.x.x *= FlowConvexParams.Scale.X * scaleInv;
				emitParams.localToWorld.x.y *= FlowConvexParams.Scale.X * scaleInv;
				emitParams.localToWorld.x.z *= FlowConvexParams.Scale.X * scaleInv;
				emitParams.localToWorld.y.x *= FlowConvexParams.Scale.Y * scaleInv;
				emitParams.localToWorld.y.y *= FlowConvexParams.Scale.Y * scaleInv;
				emitParams.localToWorld.y.z *= FlowConvexParams.Scale.Y * scaleInv;
				emitParams.localToWorld.z.x *= FlowConvexParams.Scale.Z * scaleInv;
				emitParams.localToWorld.z.y *= FlowConvexParams.Scale.Z * scaleInv;
				emitParams.localToWorld.z.z *= FlowConvexParams.Scale.Z * scaleInv;
			}

			NvFlowShapeDesc* planes = (NvFlowShapeDesc*)&Properties.Planes[Emitter.Shape.Geometry.Convex.PlaneArrayOffset];
			NvFlowUint numPlanes = Emitter.Shape.Geometry.Convex.NumPlanes;

			emitParams.shapeRangeSize = numPlanes;
			emitParams.shapeDistScale = scaleInv;

			NvFlowGridEmit(m_grid, planes, numPlanes, &emitParams, 1u);
		}

		if (CollisionFactor > 0.f)
		{
			NvFlowGridEmitParams collideParams = emitParams;
			collideParams.allocationScale = { 0.f, 0.f, 0.f };

			collideParams.slipFactor = 0.9f;
			collideParams.slipThickness = 0.1f;

			FVector ScaledVelocityLinear = Emitter.Shape.CollisionLinearVelocity * scaleInv;
			FVector ScaledVelocityAngular = Emitter.Shape.CollisionAngularVelocity * angularScale;
			collideParams.velocityLinear = *(NvFlowFloat3*)(&ScaledVelocityLinear.X);
			collideParams.velocityAngular = *(NvFlowFloat3*)(&ScaledVelocityAngular.X);
			float colideVelCR = 100.f * Emitter.VelocityMask;
			collideParams.velocityCoupleRate = { colideVelCR, colideVelCR, colideVelCR };

			collideParams.fuel = 0.f;
			collideParams.fuelCoupleRate = 100.f * Emitter.FuelMask;

			collideParams.density = 0.f;
			collideParams.densityCoupleRate = 100.f * Emitter.DensityMask;

			collideParams.temperature = 0.f;
			collideParams.temperatureCoupleRate = 100.0f * Emitter.TemperatureMask;

			collideParams.maxActiveDist = -1.f + CollisionFactor - collideParams.slipThickness;
			collideParams.minActiveDist = -1.f;

			NvFlowGridEmit(m_grid, &shapeDesc, 1u, &collideParams, 1u);
		}
	}
}

void NvFlow::Scene::collide(const FFlowGridProperties& Properties, const FFlowShape& Collider, float dt, uint32 substep, uint32 numSubsteps)
{
	FTransform ScaledTransform = Collider.Transform;
	ScaledTransform.SetLocation(Collider.Transform.GetLocation() * scaleInv);

	FVector UnitToActualScale = m_context->getShapeUnitToActualScale(Collider.GeometryType, Collider.Geometry);
	ScaledTransform.SetScale3D(ScaledTransform.GetScale3D() * UnitToActualScale);

	NvFlowGridEmitParams emitParams;
	NvFlowGridEmitParamsDefaults(&emitParams);
	emitParams.bounds = *(NvFlowFloat4x4*)(&ScaledTransform.ToMatrixWithScale().M[0][0]);
	FVector ScaledVelocityLinear = Collider.LinearVelocity * scaleInv;
	FVector ScaledVelocityAngular = Collider.AngularVelocity * angularScale;
	emitParams.velocityLinear = *(NvFlowFloat3*)(&ScaledVelocityLinear.X);
	emitParams.velocityAngular = *(NvFlowFloat3*)(&ScaledVelocityAngular.X);

	// compute centerOfMass in emitter local space
	FVector centerOfMass = scaleInv * Collider.CenterOfRotationOffset;
	centerOfMass = ScaledTransform.InverseTransformPosition(centerOfMass);
	emitParams.centerOfMass = *(NvFlowFloat3*)(&centerOfMass.X);

	emitParams.allocationScale = { 0.f, 0.f, 0.f };
	emitParams.velocityCoupleRate = { 100.f, 100.f, 100.f };

	emitParams.fuel = 0.f;
	emitParams.fuelCoupleRate = 100.f;

	emitParams.density = 0.f;
	emitParams.densityCoupleRate = 100.f;

	emitParams.temperature = 0.f;
	emitParams.temperatureCoupleRate = 100.0f;

	NvFlowShapeDesc shapeDesc;
	m_context->getShape(shapeDesc, emitParams, Collider.GeometryType, Collider.Geometry);

	emitParams.deltaTime = dt;
	
	NvFlowGridEmit(m_grid, &shapeDesc, 1u, &emitParams, 1u);
}

void NvFlow::Scene::updateSubstep(FRHICommandListImmediate& RHICmdList, float dt, uint32 substep, uint32 numSubsteps, bool& shouldFlush)
{
	auto& appctx = RHICmdList.GetContext();

	bool shouldUpdateGrid = (m_context->m_computeFramesInFlight < m_context->m_computeMaxFramesInFlight);
	bool shouldUpdateEmitters = (m_context->m_computeFramesInFlight < m_context->m_computeMaxFramesInFlight);

	NvFlowGridSetParams(m_grid, &m_gridParams);

	FFlowGridProperties& Properties = FlowGridSceneProxy->FlowGridProperties;

	if (shouldUpdateEmitters)
	{
		for (int32 i = 0; i < Properties.Emitters.Num(); i++)
		{
			FFlowEmitter& Emitter = Properties.Emitters[i];
			emit(Properties, Emitter, dt, substep, numSubsteps);
		}

		for (int32 i = 0; i < Properties.Colliders.Num(); i++)
		{
			FFlowShape& Collider = Properties.Colliders[i];
			collide(Properties, Collider, dt, substep, numSubsteps);
		}
	}

	NvFlowContext* computeContext = m_multiAdapter ? m_context->m_computeContext : m_context->m_context;

	shouldFlush = shouldFlush || (m_multiAdapter && shouldUpdateGrid);

	// update adaptive timing
	{
		const float decayRate = 0.98f;

		m_timeTotal += dt;
		if (shouldUpdateGrid) m_timeSuccess += dt;

		m_timeTotal *= decayRate;
		m_timeSuccess *= decayRate;
	}

	if (shouldUpdateGrid)
	{
		float adaptiveDt = dt;
		// adaptive timestepping for multi-GPU
		if (m_multiAdapter)
		{
			const float clampDt = 2.f * dt;
			adaptiveDt = m_timeTotal / m_timeSuccess * dt;
			if (adaptiveDt > clampDt) adaptiveDt = clampDt;
		}

		NvFlowGridUpdate(m_grid, computeContext, adaptiveDt);

		NvFlowGridProxyPush(m_gridProxy, computeContext, m_grid);
	}
}

void NvFlow::Scene::updateGridView(FRHICommandListImmediate& RHICmdList)
{
	NvFlowContext* computeContext = m_multiAdapter ? m_context->m_computeContext : m_context->m_context;

	NvFlowGridProxyFlush(m_gridProxy, computeContext);

	m_gridView = NvFlowGridProxyGetGridView(m_gridProxy, m_context->m_context);
}

void NvFlow::Scene::render(FRHICommandListImmediate& RHICmdList, const FViewInfo& View)
{
	auto& appctx = RHICmdList.GetContext();

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

	m_renderParams.depthStencilView = m_context->m_dsv;
	m_renderParams.renderTargetView = m_context->m_rtv;

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

	NvFlowVolumeRenderGridView(m_volumeRender, m_context->m_context, m_colorMap, m_gridView, &m_renderParams);

#if NVFLOW_SMP
	if (m_renderParams.lensMatchedShading.enabled)
	{
		RHICmdList.SetModifiedWMode(View.LensMatchedShadingConf, true, true);
	}
#endif
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

void NvFlowDoRender(FRHICommandListImmediate& RHICmdList, const FViewInfo& View)
{
	if (GUsingNullRHI)
	{
		return;
	}

	if (NvFlow::gContext)
	{
		SCOPE_CYCLE_COUNTER(STAT_Flow_RenderGrids);
		{
			SCOPED_DRAW_EVENT(RHICmdList, FlowRenderGrids);
			{
				SCOPED_DRAW_EVENT(RHICmdList, FlowContextRender);

				NvFlow::gContext->interopBegin(RHICmdList, false);

				NvFlow::gContext->render(RHICmdList, View);

				NvFlow::gContext->interopEnd(RHICmdList, false, false);
			}
		}
	}
}

#endif
// NvFlow end