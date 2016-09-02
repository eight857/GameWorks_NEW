#include "NvFlowPCH.h"

/*=============================================================================
	FlowGridComponent.cpp: UFlowGridComponent methods.
=============================================================================*/

// NvFlow begin

#include "NvFlow.h"

#include "EnginePrivate.h"
#include "PhysicsEngine/PhysXSupport.h"
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
	, FlowGridAssetCurrent(&FlowGridAsset)
	, FlowGridAssetOverride(nullptr)
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

	// set critical property defaults
	FlowGridProperties.bActive = false;
	FlowGridProperties.bMultiAdapterEnabled = false;
	FlowGridProperties.SubstepSize = 0.0f;
	FlowGridProperties.VirtualGridExtents = FVector(0.f);

	// initialize desc/param defaults
	NvFlowGridDescDefaults(&FlowGridProperties.GridDesc);
	NvFlowGridParamsDefaults(&FlowGridProperties.GridParams);
}

UFlowGridAsset* UFlowGridComponent::CreateOverrideAsset()
{
	// duplicate asset
	auto asset = DuplicateObject<UFlowGridAsset>(FlowGridAsset, this);
	return asset;
}

void UFlowGridComponent::SetOverrideAsset(class UFlowGridAsset* asset)
{
	FlowGridAssetOverride = asset;
	if(asset) FlowGridAssetCurrent = &FlowGridAssetOverride;
	else FlowGridAssetCurrent = &FlowGridAsset;
}

FBoxSphereBounds UFlowGridComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds NewBounds(ForceInit);

	auto& FlowGridAssetRef = (*FlowGridAssetCurrent);
	if (FlowGridAssetRef)
	{
		NewBounds.Origin = FVector(0.0f);
		NewBounds.BoxExtent = FVector(FlowGridAssetRef->GetVirtualGridExtent());
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
	void FillColorMap(FFlowGridRenderParams& RenderParams, UFlowGridAsset& FlowGridAsset)
	{
		SCOPE_CYCLE_COUNTER(STAT_Flow_UpdateColorMap);

		//Alloc color map size to default specified by the flow library. NvFlowRendering.cpp assumes that for now.
		if (RenderParams.ColorMap.Num() == 0)
		{
			NvFlowColorMapDesc FlowColorMapDesc;
			NvFlowColorMapDescDefaults(&FlowColorMapDesc);
			RenderParams.ColorMap.SetNum(FlowColorMapDesc.resolution);
		}

		float xmin = FlowGridAsset.ColorMapMinX;
		float xmax = FlowGridAsset.ColorMapMaxX;
		RenderParams.ColorMapMinX = xmin;
		RenderParams.ColorMapMaxX = xmax;

		for (int32 i = 0; i < RenderParams.ColorMap.Num(); i++)
		{
			float t = float(i) / (RenderParams.ColorMap.Num() - 1);

			float s = (xmax - xmin) * t + xmin;

			RenderParams.ColorMap[i] = FlowGridAsset.ColorMap ? FlowGridAsset.ColorMap->GetLinearColorValue(s) : FLinearColor(0.f, 0.f, 0.f, 1.f);
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

	DEC_DWORD_STAT_BY(STAT_Flow_EmitterCount, FlowGridProperties.GridEmitParams.Num());
	DEC_DWORD_STAT_BY(STAT_Flow_ColliderCount, FlowGridProperties.GridCollideParams.Num());
	FlowGridProperties.GridEmitParams.SetNum(0);
	FlowGridProperties.GridCollideParams.SetNum(0);
	FlowGridProperties.GridEmitShapeDescs.SetNum(0);
	FlowGridProperties.GridCollideShapeDescs.SetNum(0);

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

	auto& FlowGridAssetRef = (*FlowGridAssetCurrent);

	// do PhysX quer
	Overlaps.Reset();
	GetWorld()->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity, FlowGridAssetRef->ObjectType, Shape, FCollisionQueryParams(NAME_None, false), FCollisionResponseParams(FlowGridAssetRef->ResponseToChannels));

	for (int32 i = 0; i < Overlaps.Num(); ++i)
	{
		const FOverlapResult& hit = Overlaps[i];

		const UPrimitiveComponent* PrimComp = hit.Component.Get();
		if (!PrimComp)
			continue;

		ECollisionResponse Response = PrimComp->GetCollisionResponseToChannel(FlowGridAssetRef->ObjectType);

		// try to grab any attached component
		auto owner = PrimComp->GetOwner();
		auto rootComponent = owner ? owner->GetRootComponent() : nullptr;
		if (rootComponent)
		{
			auto& children = rootComponent->GetAttachChildren();
			for (int32 j = 0; j < children.Num(); j++)
			{
				//OverlapMultiple returns ECollisionResponse::ECR_Overlap types, which we want to ignore
				ECollisionResponse ResponseChild = children[j]->GetCollisionResponseToChannel(FlowGridAssetRef->ObjectType);
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

			auto PxGeometryType = PhysXShape->getGeometryType();
			bool IsSupported =
				(PxGeometryType == PxGeometryType::eSPHERE) ||
				(PxGeometryType == PxGeometryType::eBOX) ||
				(PxGeometryType == PxGeometryType::eCAPSULE) ||
				(PxGeometryType == PxGeometryType::eCONVEXMESH);
			bool IsEmitter = FlowEmitterComponent != nullptr;
			bool IsCollider = Response == ECollisionResponse::ECR_Block;

			// optimization: kick out early if couple rate is zero
			if (IsEmitter)
			{
				if (FlowEmitterComponent->CoupleRate <= 0.f)
				{
					IsEmitter = false;
				}
			}

			if (IsSupported && (IsEmitter || IsCollider))
			{
				PxTransform WorldTransform = ActorTransform*ShapeTransform;
				FVector UnitToActualScale(1.f, 1.f, 1.f);
				FVector LocalToWorldScale(1.f, 1.f, 1.f);
				FVector BoundsScale(1.f, 1.f, 1.f);
				NvFlowShapeType FlowShapeType = eNvFlowShapeTypeSphere;
				float FlowShapeDistScale = 1.f;
				uint32 NumShapeDescs = 1u;

				int32 EmitShapeStartIndex = FlowGridProperties.GridEmitShapeDescs.Num();
				int32 CollideShapeStartIndex = FlowGridProperties.GridCollideShapeDescs.Num();

				bool IsEnabledArray[2] = { IsEmitter, IsCollider };
				TArray<NvFlowShapeDesc>* ShapeDescsArray[2] = { 
					&FlowGridProperties.GridEmitShapeDescs, 
					&FlowGridProperties.GridCollideShapeDescs 
				};
				for (uint32 passID = 0u; passID < 2u; passID++)
				{
					if (!IsEnabledArray[passID])
					{
						continue;
					}

					TArray<NvFlowShapeDesc>& ShapeDescs = *ShapeDescsArray[passID];

					// compute number of NvFlowShapeDesc
					if (PxGeometryType == PxGeometryType::eCONVEXMESH)
					{
						PxConvexMeshGeometry ConvexGeometry;
						PhysXShape->getConvexMeshGeometry(ConvexGeometry);
						NumShapeDescs = ConvexGeometry.convexMesh->getNbPolygons();
					}

					// allocate shape descs
					int32 Index = ShapeDescs.AddUninitialized(NumShapeDescs);
					NvFlowShapeDesc* ShapeDescsPtr = &ShapeDescs[Index];

					if (PxGeometryType == PxGeometryType::eSPHERE)
					{
						FlowShapeType = eNvFlowShapeTypeSphere;
						PxSphereGeometry SphereGeometry;
						PhysXShape->getSphereGeometry(SphereGeometry);
						ShapeDescsPtr[0].sphere.radius = NvFlow::sdfRadius;
						UnitToActualScale = FVector(SphereGeometry.radius * (1.f / NvFlow::sdfRadius) * NvFlow::scaleInv);
					}
					else if (PxGeometryType == PxGeometryType::eBOX)
					{
						FlowShapeType = eNvFlowShapeTypeBox;
						PxBoxGeometry BoxGeometry;
						PhysXShape->getBoxGeometry(BoxGeometry);
						ShapeDescsPtr[0].box.halfSize.x = NvFlow::sdfRadius;
						ShapeDescsPtr[0].box.halfSize.y = NvFlow::sdfRadius;
						ShapeDescsPtr[0].box.halfSize.z = NvFlow::sdfRadius;
						UnitToActualScale = P2UVector(BoxGeometry.halfExtents) * (1.f / NvFlow::sdfRadius) * NvFlow::scaleInv;
						// distortion correction, makes LocalToWorld uniform scale
						FVector aspectRatio = UnitToActualScale;
						float aspectRatioMin = FMath::Min(aspectRatio.X, FMath::Min(aspectRatio.Y, aspectRatio.Z));
						aspectRatio.X /= aspectRatioMin;
						aspectRatio.Y /= aspectRatioMin;
						aspectRatio.Z /= aspectRatioMin;
						LocalToWorldScale = FVector(1.f) / aspectRatio;
						ShapeDescsPtr[0].box.halfSize.x *= aspectRatio.X;
						ShapeDescsPtr[0].box.halfSize.y *= aspectRatio.Y;
						ShapeDescsPtr[0].box.halfSize.z *= aspectRatio.Z;
					}
					else if (PxGeometryType == PxGeometryType::eCAPSULE)
					{
						FlowShapeType = eNvFlowShapeTypeCapsule;
						PxCapsuleGeometry CapsuleGeometry;
						PhysXShape->getCapsuleGeometry(CapsuleGeometry);
						ShapeDescsPtr[0].capsule.radius = NvFlow::sdfRadius;
						ShapeDescsPtr[0].capsule.length = NvFlow::sdfRadius * (2.f * CapsuleGeometry.halfHeight / CapsuleGeometry.radius);
						UnitToActualScale = FVector(CapsuleGeometry.radius * (1.f / NvFlow::sdfRadius) * NvFlow::scaleInv);

						// extends bounds on x axis
						BoundsScale.X = (0.5f * ShapeDescsPtr[0].capsule.length + 1.f);
					}
					else if (PxGeometryType == PxGeometryType::eCONVEXMESH)
					{
						FlowShapeType = eNvFlowShapeTypePlane;
						PxHullPolygon polygon;
						PxConvexMeshGeometry ConvexGeometry;
						PhysXShape->getConvexMeshGeometry(ConvexGeometry);
						auto localBounds = ConvexGeometry.convexMesh->getLocalBounds();
						auto meshScale = ConvexGeometry.scale;

						for (uint32 i = 0u; i < NumShapeDescs; i++)
						{
							ConvexGeometry.convexMesh->getPolygonData(i, polygon);
							ShapeDescsPtr[i].plane.normal.x = +polygon.mPlane[0];
							ShapeDescsPtr[i].plane.normal.y = +polygon.mPlane[1];
							ShapeDescsPtr[i].plane.normal.z = +polygon.mPlane[2];
							ShapeDescsPtr[i].plane.distance = -polygon.mPlane[3];
						}

						auto LocalMin = P2UVector(localBounds.minimum);
						auto LocalMax = P2UVector(localBounds.maximum);
						auto MeshScale = P2UVector(meshScale.scale);
						auto MeshRotation = P2UQuat(meshScale.rotation);
						const float Radius = (MeshScale * 0.5f * (LocalMax - LocalMin)).GetAbsMax();
						UnitToActualScale = FVector(Radius * (1.f / NvFlow::sdfRadius) * NvFlow::scaleInv);

						FlowShapeDistScale = NvFlow::scaleInv;

						// scale bounds
						BoundsScale = (MeshScale * 0.5f * (LocalMax - LocalMin));
						float norm = BoundsScale.GetAbsMax();
						BoundsScale /= norm;

						// scale local to world, scaleInv cancels out because planes are in UE4 space
						LocalToWorldScale = MeshScale * NvFlow::scaleInv / UnitToActualScale;
					}
				}

				if (IsEmitter)
				{
					// update transforms
					FTransform Transform = P2UTransform(WorldTransform);
					if (!FlowEmitterComponent->bHasPreviousTransform)
					{
						FlowEmitterComponent->PreviousTransform = Transform;
						FlowEmitterComponent->bHasPreviousTransform = true;
					}
					FTransform PreviousTransform = FlowEmitterComponent->PreviousTransform;
					FlowEmitterComponent->PreviousTransform = Transform;

					// physics
					FVector physicsLinearVelocity = Body->OwnerComponent->GetPhysicsLinearVelocity();
					FVector physicsAngularVelocity = Body->OwnerComponent->GetPhysicsAngularVelocity();
					FVector physicsCenterOfMass = Body->OwnerComponent->GetCenterOfMass();

					FVector CollisionLinearVelocity = Transform.InverseTransformVector(physicsLinearVelocity);
					FVector CollisionAngularVelocity = Transform.InverseTransformVector(physicsAngularVelocity);
					FVector CollisionCenterOfRotationOffset = physicsCenterOfMass;

					FVector LinearVelocity = FlowEmitterComponent->LinearVelocity + CollisionLinearVelocity * FlowEmitterComponent->BlendInPhysicalVelocity;
					FVector AngularVelocity = FlowEmitterComponent->AngularVelocity + CollisionAngularVelocity * FlowEmitterComponent->BlendInPhysicalVelocity;
					FVector CenterOfRotationOffset = CollisionCenterOfRotationOffset * FlowEmitterComponent->BlendInPhysicalVelocity;

					FVector CollisionScaledVelocityLinear = CollisionLinearVelocity * NvFlow::scaleInv;
					FVector CollisionScaledVelocityAngular = CollisionAngularVelocity * NvFlow::angularScale;
					FVector CollisionScaledCenterOfMass = CollisionCenterOfRotationOffset * NvFlow::scaleInv;

					FVector ScaledVelocityLinear = LinearVelocity * NvFlow::scaleInv;
					FVector ScaledVelocityAngular = AngularVelocity * NvFlow::angularScale;
					FVector ScaledCenterOfMass = CenterOfRotationOffset * NvFlow::scaleInv;

					// substep invariant params
					NvFlowGridEmitParams emitParams;
					NvFlowGridEmitParamsDefaults(&emitParams);

					// emit values
					emitParams.velocityLinear  = *(NvFlowFloat3*)(&ScaledVelocityLinear.X);
					emitParams.velocityAngular = *(NvFlowFloat3*)(&ScaledVelocityAngular.X);
					emitParams.fuel = FlowEmitterComponent->Fuel;
					emitParams.fuelReleaseTemp = FlowEmitterComponent->FuelReleaseTemp;
					emitParams.fuelRelease = FlowEmitterComponent->FuelRelease;
					emitParams.density = FlowEmitterComponent->Density;
					emitParams.temperature = FlowEmitterComponent->Temperature;
					emitParams.allocationPredict = FlowEmitterComponent->AllocationPredict;
					emitParams.allocationScale = {
						FlowEmitterComponent->AllocationScale,
						FlowEmitterComponent->AllocationScale,
						FlowEmitterComponent->AllocationScale
					};

					// couple rates
					float coupleRate = FlowEmitterComponent->CoupleRate;
					emitParams.fuelCoupleRate        = coupleRate * FlowEmitterComponent->FuelMask;
					emitParams.temperatureCoupleRate = coupleRate * FlowEmitterComponent->TemperatureMask;
					emitParams.densityCoupleRate     = coupleRate * FlowEmitterComponent->DensityMask;
					float velocityCoupleRate         = coupleRate * FlowEmitterComponent->VelocityMask;
					emitParams.velocityCoupleRate = { velocityCoupleRate, velocityCoupleRate, velocityCoupleRate };

					// max/min active dist
					float CollisionFactor = FlowEmitterComponent->CollisionFactor;
					float EmitterInflate = FlowEmitterComponent->EmitterInflate;
					emitParams.maxActiveDist = EmitterInflate;
					emitParams.minActiveDist = -1.f + CollisionFactor;

					// set shape type, shape base and range, distance scale
					emitParams.shapeType = FlowShapeType;
					emitParams.shapeRangeOffset = EmitShapeStartIndex;
					emitParams.shapeRangeSize = NumShapeDescs;
					emitParams.shapeDistScale = FlowShapeDistScale;

					// substep
					float NumSubsteps = FlowEmitterComponent->NumSubsteps;
					float emitterSubstepDt = FlowGridProperties.SubstepSize / NumSubsteps;

					// transforms
					FTransform BeginTransform = PreviousTransform;
					BeginTransform.SetLocation(PreviousTransform.GetLocation() * NvFlow::scaleInv);
					BeginTransform.SetScale3D(BeginTransform.GetScale3D() * UnitToActualScale);

					FTransform EndTransform = Transform;
					EndTransform.SetLocation(Transform.GetLocation() * NvFlow::scaleInv);
					EndTransform.SetScale3D(EndTransform.GetScale3D() * UnitToActualScale);

					// optimization: use NvFlow emitter substep for stationary enough objects
					bool isStationary = BeginTransform.Equals(EndTransform);
					uint32 iterations = isStationary ? 1u : NumSubsteps;
					emitParams.numSubSteps = isStationary ? NumSubsteps : 1u;

					// substepping
					for (uint32 i = 0u; i < iterations; i++)
					{
						// Note: substep and numSubsteps are for future simulation substep support
						const float substep = 0.f;
						const float numSubsteps = 1.f;

						// Generate interpolated transform
						FTransform BlendedTransform;
						float alpha = float(substep*NumSubsteps + i + 1) / (NumSubsteps*numSubsteps);
						BlendedTransform.Blend(BeginTransform, EndTransform, alpha);

						// compute centerOfMass in emitter local space
						FVector centerOfMass = BlendedTransform.InverseTransformPosition(ScaledCenterOfMass);
						emitParams.centerOfMass = *(NvFlowFloat3*)(&centerOfMass.X);

						// establish bounds and localToWorld
						FTransform Bounds = BlendedTransform;
						FTransform LocalToWorld = BlendedTransform;
						Bounds.SetScale3D(Bounds.GetScale3D() * BoundsScale);
						LocalToWorld.SetScale3D(LocalToWorld.GetScale3D() * LocalToWorldScale);

						// scale bounds as a function of emitter inflate
						{
							const float k = (EmitterInflate + 1.f);
							Bounds.SetScale3D(Bounds.GetScale3D() * k);
						}

						emitParams.bounds = *(NvFlowFloat4x4*)(&Bounds.ToMatrixWithScale().M[0][0]);
						emitParams.localToWorld = *(NvFlowFloat4x4*)(&LocalToWorld.ToMatrixWithScale().M[0][0]);

						// set timestep based on subtepping mode
						emitParams.deltaTime = isStationary ? FlowGridProperties.SubstepSize : emitterSubstepDt;

						// push parameters
						FlowGridProperties.GridEmitParams.Push(emitParams);

						// collision factor support
						if (CollisionFactor > 0.f)
						{
							NvFlowGridEmitParams collideParams = emitParams;
							collideParams.allocationScale = { 0.f, 0.f, 0.f };

							collideParams.slipFactor = 0.9f;
							collideParams.slipThickness = 0.1f;

							collideParams.velocityLinear = *(NvFlowFloat3*)(&CollisionScaledVelocityLinear.X);
							collideParams.velocityAngular = *(NvFlowFloat3*)(&CollisionScaledVelocityAngular.X);
							float collideVelCR = 100.f * FlowEmitterComponent->VelocityMask;
							collideParams.velocityCoupleRate = { collideVelCR, collideVelCR, collideVelCR };

							collideParams.fuel = 0.f;
							collideParams.fuelCoupleRate = 100.f * FlowEmitterComponent->FuelMask;

							collideParams.density = 0.f;
							collideParams.densityCoupleRate = 100.f * FlowEmitterComponent->DensityMask;

							collideParams.temperature = 0.f;
							collideParams.temperatureCoupleRate = 100.0f * FlowEmitterComponent->TemperatureMask;

							collideParams.maxActiveDist = -1.f + CollisionFactor - collideParams.slipThickness;
							collideParams.minActiveDist = -1.f;

							FlowGridProperties.GridEmitParams.Push(collideParams);
						}
					}
				}

				if (IsCollider)
				{
					// update transforms
					FTransform Transform = P2UTransform(WorldTransform);

					// physics
					FVector physicsLinearVelocity = Body->OwnerComponent->GetPhysicsLinearVelocity();
					FVector physicsAngularVelocity = Body->OwnerComponent->GetPhysicsAngularVelocity();
					FVector physicsCenterOfMass = Body->OwnerComponent->GetCenterOfMass();

					FVector CollisionLinearVelocity = Transform.InverseTransformVector(physicsLinearVelocity);
					FVector CollisionAngularVelocity = Transform.InverseTransformVector(physicsAngularVelocity);
					FVector CollisionCenterOfRotationOffset = physicsCenterOfMass;

					FVector CollisionScaledVelocityLinear = CollisionLinearVelocity * NvFlow::scaleInv;
					FVector CollisionScaledVelocityAngular = CollisionAngularVelocity * NvFlow::angularScale;
					FVector CollisionScaledCenterOfMass = CollisionCenterOfRotationOffset * NvFlow::scaleInv;

					// parameters
					NvFlowGridEmitParams emitParams;
					NvFlowGridEmitParamsDefaults(&emitParams);

					// emit values
					emitParams.velocityLinear = *(NvFlowFloat3*)(&CollisionScaledVelocityLinear.X);
					emitParams.velocityAngular = *(NvFlowFloat3*)(&CollisionScaledVelocityAngular.X);
					emitParams.fuel = 0.f;
					emitParams.density = 0.f;
					emitParams.temperature = 0.f;
					emitParams.allocationScale = { 0.f, 0.f, 0.f };

					// couple rates
					float coupleRate = 100.f;
					emitParams.fuelCoupleRate = coupleRate;
					emitParams.temperatureCoupleRate = coupleRate;
					emitParams.densityCoupleRate = coupleRate;
					float velocityCoupleRate = coupleRate;
					emitParams.velocityCoupleRate = { velocityCoupleRate, velocityCoupleRate, velocityCoupleRate };

					// set shape type, shape base and range, distance scale
					emitParams.shapeType = FlowShapeType;
					emitParams.shapeRangeOffset = CollideShapeStartIndex;
					emitParams.shapeRangeSize = NumShapeDescs;
					emitParams.shapeDistScale = FlowShapeDistScale;

					// scaled transform
					FTransform ScaledTransform = Transform;
					ScaledTransform.SetLocation(Transform.GetLocation() * NvFlow::scaleInv);
					ScaledTransform.SetScale3D(ScaledTransform.GetScale3D() * UnitToActualScale);

					// compute centerOfMass in emitter local space
					FVector centerOfMass = ScaledTransform.InverseTransformPosition(CollisionScaledCenterOfMass);
					emitParams.centerOfMass = *(NvFlowFloat3*)(&centerOfMass.X);

					// establish bounds and localToWorld
					FTransform Bounds = ScaledTransform;
					FTransform LocalToWorld = ScaledTransform;
					Bounds.SetScale3D(Bounds.GetScale3D() * BoundsScale);
					LocalToWorld.SetScale3D(LocalToWorld.GetScale3D() * LocalToWorldScale);

					emitParams.bounds = *(NvFlowFloat4x4*)(&Bounds.ToMatrixWithScale().M[0][0]);
					emitParams.localToWorld = *(NvFlowFloat4x4*)(&LocalToWorld.ToMatrixWithScale().M[0][0]);

					// step size
					emitParams.deltaTime = FlowGridProperties.SubstepSize;

					// push parameters
					FlowGridProperties.GridCollideParams.Push(emitParams);
				}
			}
		}
	}

	INC_DWORD_STAT_BY(STAT_Flow_EmitterCount, FlowGridProperties.GridEmitParams.Num());
	INC_DWORD_STAT_BY(STAT_Flow_ColliderCount, FlowGridProperties.GridCollideParams.Num());

	SCENE_UNLOCK_READ(SyncScene);
}


void UFlowGridComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SCOPE_CYCLE_COUNTER(STAT_Flow_Tick);

	auto& FlowGridAssetRef = (*FlowGridAssetCurrent);

	if (FlowGridAssetRef && SceneProxy)
	{
		// derive parameters from asset
		FVector NewHalfSize = NvFlow::scaleInv * FVector(FlowGridAssetRef->GetVirtualGridDimension() * 0.5f * FlowGridAssetRef->GridCellSize);
		FIntVector NewVirtualDim = FIntVector(FlowGridAssetRef->GetVirtualGridDimension());
		bool OldMultiAdapterEnabled = FlowGridProperties.bMultiAdapterEnabled;
		bool NewMultiAdapterEnabled = FlowGridAssetRef->bMultiAdapterEnabled;

		// grab default desc
		NvFlowGridDesc defaultGridDesc = {};
		NvFlowGridDescDefaults(&defaultGridDesc);

		//NvFlowGridDesc
		NvFlowGridDesc newGridDesc = FlowGridProperties.GridDesc;
		newGridDesc.halfSize = { NewHalfSize.X, NewHalfSize.Y, NewHalfSize.Z };
		newGridDesc.virtualDim = { uint32(NewVirtualDim.X), uint32(NewVirtualDim.Y), uint32(NewVirtualDim.Z) };
		newGridDesc.residentScale = defaultGridDesc.residentScale * FlowGridAssetRef->MemoryLimitScale;

		if (FlowGridProperties.bActive && 
			(newGridDesc.halfSize.x != FlowGridProperties.GridDesc.halfSize.x ||
			 newGridDesc.halfSize.y != FlowGridProperties.GridDesc.halfSize.y ||
			 newGridDesc.halfSize.z != FlowGridProperties.GridDesc.halfSize.z ||
			 newGridDesc.virtualDim.x != FlowGridProperties.GridDesc.virtualDim.x ||
			 newGridDesc.virtualDim.y != FlowGridProperties.GridDesc.virtualDim.y ||
			 newGridDesc.virtualDim.z != FlowGridProperties.GridDesc.virtualDim.z ||
			 newGridDesc.residentScale != FlowGridProperties.GridDesc.residentScale ||
			 NewMultiAdapterEnabled != OldMultiAdapterEnabled))
		{
			// rebuild required
			FlowGridProperties.bActive = false;
			MarkRenderDynamicDataDirty();
			return;
		}

		// Commit any changes
		FlowGridProperties.GridDesc = newGridDesc;
		FlowGridProperties.bMultiAdapterEnabled = NewMultiAdapterEnabled;

		//Properties that can be changed without rebuilding grid
		FlowGridProperties.VirtualGridExtents = FVector(FlowGridAssetRef->GetVirtualGridExtent());
		FlowGridProperties.SubstepSize = TimeStepper.FixedDt;

		//NvFlowGridParams
		auto& GridParams = FlowGridProperties.GridParams;
		NvFlowGridParamsDefaults(&GridParams);
		GridParams.velocityWeight = FlowGridAssetRef->VelocityWeight;
		GridParams.densityWeight = FlowGridAssetRef->DensityWeight;
		GridParams.tempWeight = FlowGridAssetRef->TempWeight;
		GridParams.fuelWeight = FlowGridAssetRef->FuelWeight;
		GridParams.velocityThreshold = FlowGridAssetRef->VelocityThreshold;
		GridParams.densityThreshold = FlowGridAssetRef->DensityThreshold;
		GridParams.tempThreshold = FlowGridAssetRef->TempThreshold;
		GridParams.fuelThreshold = FlowGridAssetRef->FuelThreshold;
		GridParams.importanceThreshold = FlowGridAssetRef->ImportanceThreshold;

		FVector ScaledGravity(FlowGridAssetRef->Gravity * NvFlow::scaleInv);
		GridParams.gravity = *(NvFlowFloat3*)(&ScaledGravity);
		GridParams.velocityDamping = FlowGridAssetRef->VelocityDamping;
		GridParams.densityDamping = FlowGridAssetRef->DensityDamping;
		GridParams.velocityFade = FlowGridAssetRef->VelocityFade;
		GridParams.densityFade = FlowGridAssetRef->DensityFade;
		GridParams.velocityMacCormackBlendFactor = FlowGridAssetRef->VelocityMacCormackBlendFactor;
		GridParams.densityMacCormackBlendFactor = FlowGridAssetRef->DensityMacCormackBlendFactor;
		GridParams.vorticityStrength = FlowGridAssetRef->VorticityStrength;
		GridParams.combustion.ignitionTemp = FlowGridAssetRef->IgnitionTemperature;
		GridParams.combustion.coolingRate = FlowGridAssetRef->CoolingRate;
		
		//NvFlowVolumeRenderParams
		FlowGridProperties.RenderParams.RenderingAlphaScale = FlowGridAssetRef->RenderingAlphaScale;
		FlowGridProperties.RenderParams.bAdaptiveScreenPercentage = FlowGridAssetRef->bAdaptiveScreenPercentage;
		FlowGridProperties.RenderParams.AdaptiveTargetFrameTime = FlowGridAssetRef->AdaptiveTargetFrameTime;
		FlowGridProperties.RenderParams.MaxScreenPercentage = FlowGridAssetRef->MaxScreenPercentage;
		FlowGridProperties.RenderParams.MinScreenPercentage = FlowGridAssetRef->MinScreenPercentage;
		
		if (UFlowGridAsset::sGlobalDebugDraw)
		{
			FlowGridProperties.RenderParams.bDebugWireframe = true;
			FlowGridProperties.RenderParams.RenderingMode = UFlowGridAsset::sGlobalRenderingMode;
		}
		else
		{
			FlowGridProperties.RenderParams.bDebugWireframe = FlowGridAssetRef->bDebugWireframe;
			FlowGridProperties.RenderParams.RenderingMode = FlowGridAssetRef->RenderingMode;
		}

		//ColorMap
		FillColorMap(FlowGridProperties.RenderParams, *FlowGridAssetRef);

		//EmitShapes & CollisionShapes
		UpdateShapes();

		//set active, since we are ticking
		FlowGridProperties.bActive = true;

		//push all flow properties to proxy
		MarkRenderDynamicDataDirty();

		TimeStepper.FixedDt = 1.f / FlowGridAssetRef->SimulationRate;

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
	DEC_DWORD_STAT_BY(STAT_Flow_EmitterCount, FlowGridProperties.GridEmitParams.Num());
	DEC_DWORD_STAT_BY(STAT_Flow_ColliderCount, FlowGridProperties.GridCollideParams.Num());
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

			if (FlowGridProperties.RenderParams.bDebugWireframe)
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
	Relevance.bNormalTranslucencyRelevance = true;
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