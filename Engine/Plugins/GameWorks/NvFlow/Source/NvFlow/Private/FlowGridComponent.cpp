#include "NvFlowPCH.h"

/*=============================================================================
	FlowGridComponent.cpp: UFlowGridComponent methods.
=============================================================================*/

// NvFlow begin

#include "EnginePrivate.h"
#include "PhysicsEngine/PhysXSupport.h"
#include "FlowGridSceneProxy.h"
#include "Curves/CurveLinearColor.h"
#include "RHIStaticStates.h"
#include "PhysicsPublic.h"

// CPU stats, use "stat flow" to enable
DECLARE_CYCLE_STAT(TEXT("Tick Grid Component"), STAT_Flow_Tick, STATGROUP_Flow);
DECLARE_CYCLE_STAT(TEXT("Update Emit and Collide Shapes"), STAT_Flow_UpdateShapes, STATGROUP_Flow);
DECLARE_CYCLE_STAT(TEXT("Update Color Map"), STAT_Flow_UpdateColorMap, STATGROUP_Flow);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Grid Count"), STAT_Flow_GridCount, STATGROUP_Flow);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Emitter Count"), STAT_Flow_EmitterCount, STATGROUP_Flow);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Collider Count"), STAT_Flow_ColliderCount, STATGROUP_Flow);

FTimeStepper::FTimeStepper() 
{
	DeltaTime = 0.f;
	TimeError = 0.f;
	FixedDt = (1.f / 60.f);
	MaxSteps = 1;
}

int32 FTimeStepper::GetNumSteps(float TimeStep)
{
	DeltaTime = TimeStep;

	// compute time steps
	TimeError += DeltaTime;

	int32 NumSteps = int32(TimeError / FixedDt);
	if (NumSteps < 0) NumSteps = 0;

	TimeError -= FixedDt * float(NumSteps);

	if (NumSteps > MaxSteps) NumSteps = MaxSteps;

	return NumSteps;
}

UFlowGridComponent::UFlowGridComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BodyInstance.SetUseAsyncScene(true);

	bFlowGridCollisionEnabled = true;

	static FName CollisionProfileName(TEXT("Flow"));
	SetCollisionProfileName(CollisionProfileName);

	bAlwaysCreatePhysicsState = true;
	bIsActive = true;
	bAutoActivate = true;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	FlowGridProperties.bActive = false;
	FlowGridProperties.VirtualGridExtents = FVector(0);
	FlowGridProperties.SubstepSize = 0.0f;
}

FBoxSphereBounds UFlowGridComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds NewBounds(ForceInit);

	if (FlowGridAsset)
	{
		NewBounds.Origin = FVector(0.0f);
		NewBounds.BoxExtent = FVector(FlowGridAsset->GetVirtualGridExtent());
		NewBounds.SphereRadius = 0.0f;
	}

	return NewBounds.TransformBy( LocalToWorld );
}

FPrimitiveSceneProxy* UFlowGridComponent::CreateSceneProxy()
{
	return new FFlowGridSceneProxy(this);
}

namespace
{
	void FillColorMap(FFlowGridProperties& FlowGridProperties, UFlowGridAsset& FlowGridAsset)
	{
		SCOPE_CYCLE_COUNTER(STAT_Flow_UpdateColorMap);

		//Alloc color map size to default specified by the flow library. NvFlowRendering.cpp assumes that for now.
		if (FlowGridProperties.ColorMap.Num() == 0)
		{
			NvFlowColorMapDesc FlowColorMapDesc;
			NvFlowColorMapDescDefaults(&FlowColorMapDesc);
			FlowGridProperties.ColorMap.SetNum(FlowColorMapDesc.resolution);
		}

		float xmin = FlowGridAsset.ColorMapMinX;
		float xmax = FlowGridAsset.ColorMapMaxX;
		FlowGridProperties.ColorMapMinX = xmin;
		FlowGridProperties.ColorMapMaxX = xmax;

		for (int32 i = 0; i < FlowGridProperties.ColorMap.Num(); i++)
		{
			float t = float(i) / (FlowGridProperties.ColorMap.Num() - 1);

			float s = (xmax - xmin) * t + xmin;

			FlowGridProperties.ColorMap[i] = FlowGridAsset.ColorMap ? FlowGridAsset.ColorMap->GetLinearColorValue(s) : FLinearColor(0.f, 0.f, 0.f, 1.f);
		}
	}

	// helpers to find actor, shape pairs in a TSet
	bool operator == (const PxActorShape& lhs, const PxActorShape& rhs) { return lhs.actor == rhs.actor && lhs.shape == rhs.shape; }
	uint32 GetTypeHash(const PxActorShape& h) { return ::GetTypeHash((void*)(h.actor)) ^ ::GetTypeHash((void*)(h.shape)); }
}

// send bodies from synchronous PhysX scene to Flow scene
void UFlowGridComponent::UpdateShapes()
{
	SCOPE_CYCLE_COUNTER(STAT_Flow_UpdateShapes);

	DEC_DWORD_STAT_BY(STAT_Flow_EmitterCount, FlowGridProperties.Emitters.Num());
	DEC_DWORD_STAT_BY(STAT_Flow_ColliderCount, FlowGridProperties.Colliders.Num());
	FlowGridProperties.Emitters.SetNum(0);
	FlowGridProperties.Colliders.SetNum(0);

	// only update if enabled
	if (!bFlowGridCollisionEnabled)
	{
		return;
	}

	// used to test if an actor shape pair has already been reported
	TSet<PxActorShape> OverlapSet;

	// buffer for overlaps
	TArray<FOverlapResult> Overlaps;
	TArray<PxShape*> Shapes;

	// get PhysX Scene
	FPhysScene* PhysScene = GetWorld()->GetPhysicsScene();
	PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);

	// lock the scene to perform scene queries
	SCENE_LOCK_READ(SyncScene);

	// create FCollisionShape from flow grid domain
	FBoxSphereBounds ComponentBounds = CalcBounds(GetComponentTransform());
	const FVector Center = ComponentBounds.Origin;
	const FVector HalfEdge = ComponentBounds.BoxExtent;
	FCollisionShape Shape;
	Shape.SetBox(HalfEdge);

	// do PhysX quer
	Overlaps.Reset();
	GetWorld()->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity, FlowGridAsset->ObjectType, Shape, FCollisionQueryParams(NAME_None, false), FCollisionResponseParams(FlowGridAsset->ResponseToChannels));

	for (int32 i = 0; i < Overlaps.Num(); ++i)
	{
		const FOverlapResult& hit = Overlaps[i];

		const UPrimitiveComponent* PrimComp = hit.Component.Get();
		if (!PrimComp)
			continue;

		ECollisionResponse Response = PrimComp->GetCollisionResponseToChannel(FlowGridAsset->ObjectType);

		// try to grab any attached component
		auto owner = PrimComp->GetOwner();
		auto rootComponent = owner ? owner->GetRootComponent() : nullptr;
		if (rootComponent)
		{
			auto& children = rootComponent->GetAttachChildren();
			for (int32 j = 0; j < children.Num(); j++)
			{
				//OverlapMultiple returns ECollisionResponse::ECR_Overlap types, which we want to ignore
				ECollisionResponse ResponseChild = children[j]->GetCollisionResponseToChannel(FlowGridAsset->ObjectType);
				if (ResponseChild != ECollisionResponse::ECR_Ignore)
				{
					Response = ResponseChild;
				}
			}
		}
		
		if (Response == ECollisionResponse::ECR_Ignore)
			continue;

		FBodyInstance* Body = PrimComp->GetBodyInstance();
		if (!Body)
			continue;

		PxRigidActor* PhysXActor = Body->GetPxRigidActor_AssumesLocked();
		if (!PhysXActor)
			continue;

		Shapes.SetNum(0);

		int32 NumSyncShapes;
		NumSyncShapes = Body->GetAllShapes_AssumesLocked(Shapes);

		for (int ShapeIndex = 0; ShapeIndex < NumSyncShapes; ++ShapeIndex)
		{
			PxShape* PhysXShape = Shapes[ShapeIndex];

			if (!PhysXActor || !PhysXShape)
				continue;

			// check if we've already processed this actor-shape pair
			bool alreadyProcessed = false;
			OverlapSet.Add(PxActorShape(PhysXActor, PhysXShape), &alreadyProcessed);
			if (alreadyProcessed)
				continue;

			const PxTransform& ActorTransform = PhysXActor->getGlobalPose();
			const PxTransform& ShapeTransform = PhysXShape->getLocalPose();

			PxFilterData Filter = PhysXShape->getQueryFilterData();

			// only process simple collision shapes for now
			if ((Filter.word3 & EPDF_SimpleCollision) == 0)
				continue;

			// get emitter parameters, if available
			AActor* Actor = Body->OwnerComponent->GetOwner();
			UFlowEmitterComponent* FlowEmitterComponent = Actor->FindComponentByClass<UFlowEmitterComponent>();

			// search in attached component actors, as needed
			if (rootComponent && FlowEmitterComponent == nullptr)
			{
				auto& children = rootComponent->GetAttachChildren();
				for (int32 j = 0; j < children.Num(); j++)
				{
					auto owner = children[j]->GetOwner();
					if (owner)
					{
						FlowEmitterComponent = owner->FindComponentByClass<UFlowEmitterComponent>();
						if (FlowEmitterComponent)
						{
							break;
						}
					}
				}
			}

			switch (PhysXShape->getGeometryType())
			{
				case PxGeometryType::eSPHERE:
				case PxGeometryType::eBOX:
				case PxGeometryType::eCAPSULE:
				{
					PxTransform WorldTransform = ActorTransform*ShapeTransform;
					
					EFlowGeometryType FlowShapeGeometryType = EFGT_eSphere;
					FFlowShape::FGeometry FlowShapeGeometry = {};

					if (PhysXShape->getGeometryType() == PxGeometryType::eSPHERE)
					{
						PxSphereGeometry SphereGeometry;
						PhysXShape->getSphereGeometry(SphereGeometry);
						FlowShapeGeometry.Sphere.Radius = SphereGeometry.radius;
						FlowShapeGeometryType = EFGT_eSphere;
					}
					else if (PhysXShape->getGeometryType() == PxGeometryType::eBOX)
					{
						PxBoxGeometry BoxGeometry;
						PhysXShape->getBoxGeometry(BoxGeometry);
						*(FVector*)&FlowShapeGeometry.Box.Extends[0] = P2UVector(BoxGeometry.halfExtents);
						FlowShapeGeometryType = EFGT_eBox;
					}
					else if (PhysXShape->getGeometryType() == PxGeometryType::eCAPSULE)
					{
						PxCapsuleGeometry CapsuleGeometry;
						PhysXShape->getCapsuleGeometry(CapsuleGeometry);
						FlowShapeGeometry.Capsule.Radius = CapsuleGeometry.radius;
						FlowShapeGeometry.Capsule.HalfHeight = CapsuleGeometry.halfHeight;
						FlowShapeGeometryType = EFGT_eCapsule;
					}

					if (FlowEmitterComponent)
					{
						INC_DWORD_STAT(STAT_Flow_EmitterCount);
						int32 Index = FlowGridProperties.Emitters.AddUninitialized(1);
						FFlowEmitter& FlowEmitter = FlowGridProperties.Emitters[Index];

						FlowEmitter.Shape.GeometryType = FlowShapeGeometryType;
						FlowEmitter.Shape.Geometry = FlowShapeGeometry;

						FlowEmitter.Shape.Transform = P2UTransform(WorldTransform);

						if (!FlowEmitterComponent->bHasPreviousTransform)
						{
							FlowEmitterComponent->PreviousTransform = FlowEmitter.Shape.Transform;
							FlowEmitterComponent->bHasPreviousTransform = true;
						}

						FlowEmitter.Shape.PreviousTransform = FlowEmitterComponent->PreviousTransform;
						FlowEmitterComponent->PreviousTransform = FlowEmitter.Shape.Transform;

						FVector physicsLinearVelocity = Body->OwnerComponent->GetPhysicsLinearVelocity();
						FVector physicsAngularVelocity = Body->OwnerComponent->GetPhysicsAngularVelocity();
						FVector physicsCenterOfMass = Body->OwnerComponent->GetCenterOfMass();

						physicsLinearVelocity = FlowEmitter.Shape.Transform.InverseTransformVector(physicsLinearVelocity);
						physicsAngularVelocity = FlowEmitter.Shape.Transform.InverseTransformVector(physicsAngularVelocity);
						physicsCenterOfMass = physicsCenterOfMass;

						FlowEmitter.Shape.LinearVelocity = FlowEmitterComponent->LinearVelocity + physicsLinearVelocity * FlowEmitterComponent->BlendInPhysicalVelocity;
						FlowEmitter.Shape.AngularVelocity = FlowEmitterComponent->AngularVelocity + physicsAngularVelocity * FlowEmitterComponent->BlendInPhysicalVelocity;
						FlowEmitter.Shape.CenterOfRotationOffset = physicsCenterOfMass * FlowEmitterComponent->BlendInPhysicalVelocity;

						FlowEmitter.Shape.CollisionLinearVelocity = physicsLinearVelocity;
						FlowEmitter.Shape.CollisionAngularVelocity = physicsAngularVelocity;
						FlowEmitter.Shape.CollisionCenterOfRotationOffset = physicsCenterOfMass;

						FlowEmitter.Density = FlowEmitterComponent->Density;
						FlowEmitter.Temperature = FlowEmitterComponent->Temperature;
						FlowEmitter.Fuel = FlowEmitterComponent->Fuel;
						FlowEmitter.FuelReleaseTemp = FlowEmitterComponent->FuelReleaseTemp;
						FlowEmitter.FuelRelease = FlowEmitterComponent->FuelRelease;
						FlowEmitter.AllocationPredict = FlowEmitterComponent->AllocationPredict;
						FlowEmitter.AllocationScale = FlowEmitterComponent->AllocationScale;
						FlowEmitter.CollisionFactor = FlowEmitterComponent->CollisionFactor;
						FlowEmitter.EmitterInflate = FlowEmitterComponent->EmitterInflate;
						FlowEmitter.CoupleRate = FlowEmitterComponent->CoupleRate;
						FlowEmitter.VelocityMask = FlowEmitterComponent->VelocityMask;
						FlowEmitter.DensityMask = FlowEmitterComponent->DensityMask;
						FlowEmitter.TemperatureMask = FlowEmitterComponent->TemperatureMask;
						FlowEmitter.FuelMask = FlowEmitterComponent->FuelMask;

						FlowEmitter.NumSubsteps = FlowEmitterComponent->NumSubsteps;
					}

					if (Response == ECollisionResponse::ECR_Block)
					{
						INC_DWORD_STAT(STAT_Flow_ColliderCount);
						int32 Index = FlowGridProperties.Colliders.AddUninitialized(1);
						FFlowShape& ColliderShape = FlowGridProperties.Colliders[Index];

						ColliderShape.GeometryType = FlowShapeGeometryType;
						ColliderShape.Geometry = FlowShapeGeometry;

						ColliderShape.Transform = P2UTransform(WorldTransform);

						FVector physicsLinearVelocity = Body->OwnerComponent->GetPhysicsLinearVelocity();
						FVector physicsAngularVelocity = Body->OwnerComponent->GetPhysicsAngularVelocity();
						FVector physicsCenterOfMass = Body->OwnerComponent->GetCenterOfMass();

						physicsLinearVelocity = ColliderShape.Transform.InverseTransformVector(physicsLinearVelocity);
						physicsAngularVelocity = ColliderShape.Transform.InverseTransformVector(physicsAngularVelocity);
						physicsCenterOfMass = physicsCenterOfMass;

						ColliderShape.LinearVelocity = physicsLinearVelocity;
						ColliderShape.AngularVelocity = physicsAngularVelocity;
						ColliderShape.CenterOfRotationOffset = physicsCenterOfMass;

						ColliderShape.CollisionLinearVelocity = physicsLinearVelocity;
						ColliderShape.CollisionAngularVelocity = physicsAngularVelocity;
						ColliderShape.CollisionCenterOfRotationOffset = physicsCenterOfMass;
					}
					break;
				}
			};
		}
	}

	SCENE_UNLOCK_READ(SyncScene);
}


void UFlowGridComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SCOPE_CYCLE_COUNTER(STAT_Flow_Tick);

	if (FlowGridAsset && SceneProxy)
	{
		//Properties that require rebuild of grid

		//NvFlowGridDesc
		FVector NewHalfSize = FVector(FlowGridAsset->GetVirtualGridDimension() * 0.5f * FlowGridAsset->GridCellSize);
		FIntVector NewVirtualDim = FIntVector(FlowGridAsset->GetVirtualGridDimension());
		float NewMemoryLimitScale = FlowGridAsset->MemoryLimitScale;
		bool OldMultiAdapterEnabled = FlowGridProperties.bMultiAdapterEnabled;
		bool NewMultiAdapterEnabled = FlowGridAsset->bMultiAdapterEnabled;

		if (FlowGridProperties.bActive && 
			(NewHalfSize != FlowGridProperties.HalfSize ||
			 NewVirtualDim != FlowGridProperties.VirtualDim ||
			 NewMemoryLimitScale != FlowGridProperties.MemoryLimitScale || 
			 NewMultiAdapterEnabled != OldMultiAdapterEnabled))
		{
			// rebuild required
			FlowGridProperties.bActive = false;
			MarkRenderDynamicDataDirty();
			return;
		}

		FlowGridProperties.HalfSize = NewHalfSize;
		FlowGridProperties.VirtualDim = NewVirtualDim;
		FlowGridProperties.MemoryLimitScale = NewMemoryLimitScale;
		FlowGridProperties.bMultiAdapterEnabled = NewMultiAdapterEnabled;

		//Properties that can be changed without rebuilding grid
		FlowGridProperties.VirtualGridExtents = FVector(FlowGridAsset->GetVirtualGridExtent());
		FlowGridProperties.SubstepSize = TimeStepper.FixedDt;

		//NvFlowGridParams
		FlowGridProperties.VelocityWeight = FlowGridAsset->VelocityWeight;
		FlowGridProperties.DensityWeight = FlowGridAsset->DensityWeight;
		FlowGridProperties.TempWeight = FlowGridAsset->TempWeight;
		FlowGridProperties.FuelWeight = FlowGridAsset->FuelWeight;
		FlowGridProperties.VelocityThreshold = FlowGridAsset->VelocityThreshold;
		FlowGridProperties.DensityThreshold = FlowGridAsset->DensityThreshold;
		FlowGridProperties.TempThreshold = FlowGridAsset->TempThreshold;
		FlowGridProperties.FuelThreshold = FlowGridAsset->FuelThreshold;
		FlowGridProperties.ImportanceThreshold = FlowGridAsset->ImportanceThreshold;

		FlowGridProperties.Gravity = FlowGridAsset->Gravity;
		FlowGridProperties.VelocityDamping = FlowGridAsset->VelocityDamping;
		FlowGridProperties.DensityDamping = FlowGridAsset->DensityDamping;
		FlowGridProperties.VelocityFade = FlowGridAsset->VelocityFade;
		FlowGridProperties.DensityFade = FlowGridAsset->DensityFade;
		FlowGridProperties.VelocityMacCormackBlendFactor = FlowGridAsset->VelocityMacCormackBlendFactor;
		FlowGridProperties.DensityMacCormackBlendFactor = FlowGridAsset->DensityMacCormackBlendFactor;
		FlowGridProperties.VorticityStrength = FlowGridAsset->VorticityStrength;
		FlowGridProperties.CombustionIgnitionTemperature = FlowGridAsset->IgnitionTemperature;
		FlowGridProperties.CombustionCoolingRate = FlowGridAsset->CoolingRate;
		
		//NvFlowVolumeRenderParams
		FlowGridProperties.RenderingAlphaScale = FlowGridAsset->RenderingAlphaScale;
		FlowGridProperties.bAdaptiveScreenPercentage = FlowGridAsset->bAdaptiveScreenPercentage;
		FlowGridProperties.AdaptiveTargetFrameTime = FlowGridAsset->AdaptiveTargetFrameTime;
		FlowGridProperties.MaxScreenPercentage = FlowGridAsset->MaxScreenPercentage;
		FlowGridProperties.MinScreenPercentage = FlowGridAsset->MinScreenPercentage;
		
		if (UFlowGridAsset::sGlobalDebugDraw)
		{
			FlowGridProperties.bDebugWireframe = true;
			FlowGridProperties.RenderingMode = UFlowGridAsset::sGlobalRenderingMode;
		}
		else
		{
			FlowGridProperties.bDebugWireframe = FlowGridAsset->bDebugWireframe;
			FlowGridProperties.RenderingMode = FlowGridAsset->RenderingMode;
		}

		//ColorMap
		FillColorMap(FlowGridProperties, *FlowGridAsset);

		//EmitShapes & CollisionShapes
		UpdateShapes();

		//set active, since we are ticking
		FlowGridProperties.bActive = true;

		//push all flow properties to proxy
		MarkRenderDynamicDataDirty();

		TimeStepper.FixedDt = 1.f / FlowGridAsset->SimulationRate;

		//trigger simulation substeps in render thread
		int32 NumSubSteps = TimeStepper.GetNumSteps(DeltaTime);

		if (NumSubSteps > 0)
		{
			// Enqueue command to send to render thread
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
				FSendFlowGridSimulate,
				FFlowGridSceneProxy*, FlowGridSceneProxy, (FFlowGridSceneProxy*)SceneProxy,
				int32, NumSubSteps, NumSubSteps,
				{
					FlowGridSceneProxy->Simulate_RenderThread(NumSubSteps);
				});
		}
	}
}

void UFlowGridComponent::CreatePhysicsState()
{
	UActorComponent::CreatePhysicsState();
}

void UFlowGridComponent::DestroyPhysicsState()
{
	UActorComponent::DestroyPhysicsState();
}

#if WITH_EDITOR	
void UFlowGridComponent::OnRegister()
{
	Super::OnRegister();
}

void UFlowGridComponent::OnUnregister()
{
	Super::OnUnregister();
}
#endif

void UFlowGridComponent::BeginPlay()
{
	Super::BeginPlay();
	INC_DWORD_STAT(STAT_Flow_GridCount);
}

void UFlowGridComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{	
	DEC_DWORD_STAT_BY(STAT_Flow_EmitterCount, FlowGridProperties.Emitters.Num());
	DEC_DWORD_STAT_BY(STAT_Flow_ColliderCount, FlowGridProperties.Colliders.Num());
	DEC_DWORD_STAT(STAT_Flow_GridCount);
	Super::EndPlay(EndPlayReason);
}

void UFlowGridComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport/* = ETeleportType::None*/)
{
	// We are handling the physics move below, so don't handle it at higher levels
	Super::OnUpdateTransform(EUpdateTransformFlags::SkipPhysicsUpdate, Teleport);

	// Reset simulation - will get turned on with Tick again
	FlowGridProperties.bActive = false;
	MarkRenderDynamicDataDirty();
}

void UFlowGridComponent::SendRenderDynamicData_Concurrent()
{
	Super::SendRenderDynamicData_Concurrent();
	if (SceneProxy)
	{
		// Enqueue command to send to render thread
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FSendFlowGridDynamicData,
			FFlowGridSceneProxy*, FlowGridSceneProxy, (FFlowGridSceneProxy*)SceneProxy,
			FFlowGridProperties, FlowGridProperties, FlowGridProperties,
			{
				FlowGridSceneProxy->SetDynamicData_RenderThread(FlowGridProperties);
			});
	}
}

void UFlowGridComponent::SendRenderTransform_Concurrent()
{
	Super::SendRenderTransform_Concurrent();
}

/*=============================================================================
FFlowGridSceneProxy
=============================================================================*/

FFlowGridSceneProxy::FFlowGridSceneProxy(UFlowGridComponent* Component)
	: FPrimitiveSceneProxy(Component)
	, FlowGridProperties(Component->FlowGridProperties)
	, NumScheduledSubsteps(0)
	, scenePtr(nullptr)
	, cleanupSceneFunc(nullptr)
{
	FlowData.bFlowGrid = true;
}

FFlowGridSceneProxy::~FFlowGridSceneProxy()
{
	if (scenePtr != nullptr)
	{
		check(cleanupSceneFunc != nullptr);
		cleanupSceneFunc(scenePtr);
	}
}


void FFlowGridSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	const FMatrix& LocalToWorld = GetLocalToWorld();

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			const FSceneView* View = Views[ViewIndex];

			FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

			if (FlowGridProperties.bDebugWireframe)
			{
				const FLinearColor DrawColor = FLinearColor(1.0f, 1.0f, 1.0f);
				FBox Box(LocalToWorld.GetOrigin() - FlowGridProperties.VirtualGridExtents, LocalToWorld.GetOrigin() + FlowGridProperties.VirtualGridExtents);
				DrawWireBox(PDI, Box, DrawColor, SDPG_World, 2.0f);
			}
		}
	}
}

void FFlowGridSceneProxy::CreateRenderThreadResources()
{
}

FPrimitiveViewRelevance FFlowGridSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Relevance = FPrimitiveSceneProxy::GetViewRelevance(View);
	Relevance.bDynamicRelevance = true;
	Relevance.bStaticRelevance = false;
	Relevance.bDrawRelevance = IsShown(View);
	Relevance.bShadowRelevance = false;
	Relevance.bEditorPrimitiveRelevance = false;
	return Relevance;
}

void FFlowGridSceneProxy::SetDynamicData_RenderThread(const FFlowGridProperties& InFlowGridProperties)
{
	FlowGridProperties = InFlowGridProperties;

	// if bActive was turned off, clean up the scheduled substeps
	if (!FlowGridProperties.bActive)
	{
		NumScheduledSubsteps = 0;
	}
}

void FFlowGridSceneProxy::Simulate_RenderThread(int32 NumSubSteps)
{
	NumScheduledSubsteps += NumSubSteps;
}

// NvFlow end