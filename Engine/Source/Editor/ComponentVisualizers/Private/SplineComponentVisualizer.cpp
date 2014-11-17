// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ComponentVisualizersPrivatePCH.h"
#include "SplineComponentVisualizer.h"
#include "ScopedTransaction.h"

IMPLEMENT_HIT_PROXY(HSplineVisProxy, HComponentVisProxy);
IMPLEMENT_HIT_PROXY(HSplineKeyProxy, HSplineVisProxy);

#define LOCTEXT_NAMESPACE "SplineComponentVisualizer"


/** Define commands for the spline component visualizer */
class FSplineComponentVisualizerCommands : public TCommands<FSplineComponentVisualizerCommands>
{
public:
	FSplineComponentVisualizerCommands() : TCommands <FSplineComponentVisualizerCommands>
	(
		"SplineComponentVisualizer",	// Context name for fast lookup
		LOCTEXT("SplineComponentVisualizer", "Spline Component Visualizer"),	// Localized context name for displaying
		NAME_None,	// Parent
		FEditorStyle::GetStyleSetName()
	)
	{
	}

	virtual void RegisterCommands() override
	{
		UI_COMMAND(DeleteKey, "Delete Key", "Delete the currently selected key.", EUserInterfaceActionType::Button, FInputGesture(EKeys::Delete));
		UI_COMMAND(DuplicateKey, "Duplicate Key", "Duplicate the currently selected key.", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(ResetToUnclampedTangent, "Unclamped Tangent", "Reset the tangent for this key to its default unclamped value.", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(ResetToClampedTangent, "Clamped Tangent", "Reset the tangent for this key to its default clamped value.", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(SetKeyToCurve, "Curve", "Set spline key to Curve type", EUserInterfaceActionType::RadioButton, FInputGesture());
		UI_COMMAND(SetKeyToLinear, "Linear", "Set spline key to Linear type", EUserInterfaceActionType::RadioButton, FInputGesture());
		UI_COMMAND(SetKeyToConstant, "Constant", "Set spline key to Constant type", EUserInterfaceActionType::RadioButton, FInputGesture());
	}

public:
	/** Delete key */
	TSharedPtr<FUICommandInfo> DeleteKey;

	/** Duplicate key */
	TSharedPtr<FUICommandInfo> DuplicateKey;

	/** Reset to unclamped tangent */
	TSharedPtr<FUICommandInfo> ResetToUnclampedTangent;

	/** Reset to clamped tangent */
	TSharedPtr<FUICommandInfo> ResetToClampedTangent;

	/** Set spline key to Curve type */
	TSharedPtr<FUICommandInfo> SetKeyToCurve;

	/** Set spline key to Linear type */
	TSharedPtr<FUICommandInfo> SetKeyToLinear;

	/** Set spline key to Constant type */
	TSharedPtr<FUICommandInfo> SetKeyToConstant;
};



FSplineComponentVisualizer::FSplineComponentVisualizer()
	: FComponentVisualizer()
	, SelectedKeyIndex(INDEX_NONE)
	, bAllowDuplication(true)
{
	FSplineComponentVisualizerCommands::Register();

	SplineComponentVisualizerActions = MakeShareable(new FUICommandList);
}

void FSplineComponentVisualizer::OnRegister()
{
	const auto& Commands = FSplineComponentVisualizerCommands::Get();

	SplineComponentVisualizerActions->MapAction(
		Commands.DeleteKey,
		FExecuteAction::CreateSP(this, &FSplineComponentVisualizer::OnDeleteKey),
		FCanExecuteAction::CreateSP(this, &FSplineComponentVisualizer::IsSelectionValid));

	SplineComponentVisualizerActions->MapAction(
		Commands.DuplicateKey,
		FExecuteAction::CreateSP(this, &FSplineComponentVisualizer::OnDuplicateKey),
		FCanExecuteAction::CreateSP(this, &FSplineComponentVisualizer::IsSelectionValid));

	SplineComponentVisualizerActions->MapAction(
		Commands.ResetToUnclampedTangent,
		FExecuteAction::CreateSP(this, &FSplineComponentVisualizer::OnResetToAutomaticTangent, CIM_CurveAuto),
		FCanExecuteAction::CreateSP(this, &FSplineComponentVisualizer::CanResetToAutomaticTangent, CIM_CurveAuto));

	SplineComponentVisualizerActions->MapAction(
		Commands.ResetToClampedTangent,
		FExecuteAction::CreateSP(this, &FSplineComponentVisualizer::OnResetToAutomaticTangent, CIM_CurveAutoClamped),
		FCanExecuteAction::CreateSP(this, &FSplineComponentVisualizer::CanResetToAutomaticTangent, CIM_CurveAutoClamped));

	SplineComponentVisualizerActions->MapAction(
		Commands.SetKeyToCurve,
		FExecuteAction::CreateSP(this, &FSplineComponentVisualizer::OnSetKeyType, CIM_CurveAuto),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FSplineComponentVisualizer::IsKeyTypeSet, CIM_CurveAuto));

	SplineComponentVisualizerActions->MapAction(
		Commands.SetKeyToLinear,
		FExecuteAction::CreateSP(this, &FSplineComponentVisualizer::OnSetKeyType, CIM_Linear),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FSplineComponentVisualizer::IsKeyTypeSet, CIM_Linear));

	SplineComponentVisualizerActions->MapAction(
		Commands.SetKeyToConstant,
		FExecuteAction::CreateSP(this, &FSplineComponentVisualizer::OnSetKeyType, CIM_Constant),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FSplineComponentVisualizer::IsKeyTypeSet, CIM_Constant));
}

FSplineComponentVisualizer::~FSplineComponentVisualizer()
{
	FSplineComponentVisualizerCommands::Unregister();
}

void FSplineComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (const USplineComponent* SplineComp = Cast<const USplineComponent>(Component))
	{
		if (!SplineComp->bAllowSplineEditingPerInstance)
		{
			return;
		}

		const FColor DrawColor(255, 255, 255);
		const FColor SelectedColor(255, 0, 0);
		const float GrabHandleSize = 12.0f;

		const FInterpCurveVector& SplineInfo = SplineComp->SplineInfo;

		/*
		// Debug draw keys on the SplineReparamTable
		const FInterpCurveFloat& SplineReparamTable = SplineComp->SplineReparamTable;
		for (const auto& Point : SplineReparamTable.Points)
		{
			const float Value = Point.OutVal;
			const FVector KeyPos = SplineComp->ComponentToWorld.TransformPosition(SplineInfo.Eval(Value, FVector::ZeroVector));
			PDI->DrawPoint(KeyPos, FColor(255, 0, 255), 10.f, SDPG_World);
		}
		*/

		FVector OldKeyPos(0);
		float OldKeyTime = 0.f;
		for (int32 KeyIdx = 0; KeyIdx<SplineInfo.Points.Num(); KeyIdx++)
		{
			float NewKeyTime = SplineInfo.Points[KeyIdx].InVal;
			FVector NewKeyPos = SplineComp->ComponentToWorld.TransformPosition( SplineInfo.Eval(NewKeyTime, FVector::ZeroVector) );

			USplineComponent* EditedSplineComp = GetEditedSplineComponent();
			const FColor KeyColor = (SplineComp == EditedSplineComp && KeyIdx == SelectedKeyIndex) ? SelectedColor : DrawColor;

			// Draw the keypoint
			PDI->SetHitProxy(new HSplineKeyProxy(Component, KeyIdx));
			PDI->DrawPoint(NewKeyPos, KeyColor, GrabHandleSize, SDPG_Foreground);
			PDI->SetHitProxy(NULL);

			// If not the first keypoint, draw a line to the previous keypoint.
			if (KeyIdx>0)
			{
				// For constant interpolation - don't draw ticks - just draw dotted line.
				if (SplineInfo.Points[KeyIdx - 1].InterpMode == CIM_Constant)
				{
					DrawDashedLine(PDI, OldKeyPos, NewKeyPos, DrawColor, 20, SDPG_World);
				}
				else
				{
					int32 NumSteps = FMath::CeilToInt((NewKeyTime - OldKeyTime) / 0.02f);
					float DrawSubstep = (NewKeyTime - OldKeyTime) / NumSteps;

					// Find position on first keyframe.
					float OldTime = OldKeyTime;
					FVector OldPos = OldKeyPos;

					// Then draw a line for each substep.
					for (int32 StepIdx = 1; StepIdx<NumSteps + 1; StepIdx++)
					{
						float NewTime = OldKeyTime + StepIdx*DrawSubstep;
						FVector NewPos = SplineComp->ComponentToWorld.TransformPosition( SplineInfo.Eval(NewTime, FVector::ZeroVector) );

						PDI->DrawLine(OldPos, NewPos, DrawColor, SDPG_Foreground);

						OldTime = NewTime;
						OldPos = NewPos;
					}
				}
			}

			OldKeyTime = NewKeyTime;
			OldKeyPos = NewKeyPos;
		}
	}
}

bool FSplineComponentVisualizer::VisProxyHandleClick(HComponentVisProxy* VisProxy)
{
	bool bEditing = false;

	if(VisProxy && VisProxy->Component.IsValid())
	{
		const USplineComponent* SplineComp = CastChecked<const USplineComponent>(VisProxy->Component.Get());

		SplineCompPropName = GetComponentPropertyName(SplineComp);
		if(SplineCompPropName != NAME_None)
		{
			SplineOwningActor = SplineComp->GetOwner();

			if (VisProxy->IsA(HSplineKeyProxy::StaticGetType()))
			{
				HSplineKeyProxy* KeyProxy = (HSplineKeyProxy*)VisProxy;
				SelectedKeyIndex = KeyProxy->KeyIndex;
				bEditing = true;
			}
		}
		else
		{
			SplineOwningActor = NULL;
		}
	}

	return bEditing;
}

USplineComponent* FSplineComponentVisualizer::GetEditedSplineComponent() const
{
	return Cast<USplineComponent>(GetComponentFromPropertyName(SplineOwningActor.Get(), SplineCompPropName));
}


bool FSplineComponentVisualizer::GetWidgetLocation(FVector& OutLocation) const
{
	USplineComponent* SplineComp = GetEditedSplineComponent();
	if (SplineComp && SelectedKeyIndex != INDEX_NONE)
	{
		if (SelectedKeyIndex < SplineComp->SplineInfo.Points.Num())
		{
			OutLocation = SplineComp->ComponentToWorld.TransformPosition( SplineComp->SplineInfo.Points[SelectedKeyIndex].OutVal );
			return true;
		}
	}

	return false;
}


bool FSplineComponentVisualizer::HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale)
{
	USplineComponent* SplineComp = GetEditedSplineComponent();
	if( SplineComp && 
		SelectedKeyIndex != INDEX_NONE &&
		SelectedKeyIndex < SplineComp->SplineInfo.Points.Num() )
	{
		if (ViewportClient->IsAltPressed() && bAllowDuplication)
		{
			OnDuplicateKey();
			// Don't duplicate again until we release LMB
			bAllowDuplication = false;
		}

		FInterpCurvePoint<FVector>& EditedPoint = SplineComp->SplineInfo.Points[SelectedKeyIndex];

		if (!DeltaTranslate.IsZero())
		{
			// Find key position in world space
			FVector CurrentWorldPos = SplineComp->ComponentToWorld.TransformPosition(EditedPoint.OutVal);
			// Move in world space
			FVector NewWorldPos = CurrentWorldPos + DeltaTranslate;
			// Convert back to local space
			EditedPoint.OutVal = SplineComp->ComponentToWorld.InverseTransformPosition(NewWorldPos);
		}

		if (!DeltaRotate.IsZero())
		{
			// Set point tangent as user controlled
			EditedPoint.InterpMode = CIM_CurveUser;

			// Rotate tangent according to delta rotation
			const FVector NewTangent = DeltaRotate.RotateVector(EditedPoint.LeaveTangent);
			EditedPoint.LeaveTangent = NewTangent;
			EditedPoint.ArriveTangent = NewTangent;
		}

		if (!DeltaScale.IsZero())
		{
			// Set point tangent as user controlled
			EditedPoint.InterpMode = CIM_CurveUser;

			// Break tangent into direction and length so we can change its scale (the 'tension')
			// independently of its direction.
			FVector Direction;
			float Length;
			EditedPoint.LeaveTangent.ToDirectionAndLength(Direction, Length);

			// Figure out which component has changed, and use it
			float DeltaScaleValue = (DeltaScale.X != 0.0f) ? DeltaScale.X : ((DeltaScale.Y != 0.0f) ? DeltaScale.Y : DeltaScale.Z);

			// Change scale, avoiding singularity by never allowing a scale of 0, hence preserving direction.
			Length += DeltaScaleValue * 10.0f;
			if (Length == 0.0f)
			{
				Length = SMALL_NUMBER;
			}

			const FVector NewTangent = Direction * Length;
			EditedPoint.LeaveTangent = NewTangent;
			EditedPoint.ArriveTangent = NewTangent;
		}

		NotifyComponentModified();

		return true;
	}

	return false;
}

bool FSplineComponentVisualizer::HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	bool bHandled = false;

	if (Key == EKeys::LeftMouseButton && Event == IE_Released)
	{
		// Reset duplication flag on LMB release
		bAllowDuplication = true;
	}

	if (Event == IE_Pressed)
	{
		bHandled = SplineComponentVisualizerActions->ProcessCommandBindings(Key, FSlateApplication::Get().GetModifierKeys(), false);
	}

	return bHandled;
}


void FSplineComponentVisualizer::EndEditing()
{
	SplineOwningActor = NULL;
	SplineCompPropName = NAME_None;
	SelectedKeyIndex = INDEX_NONE;
}


void FSplineComponentVisualizer::OnDuplicateKey()
{
	const FScopedTransaction Transaction(LOCTEXT("DuplicateKey", "Duplicate Spline Key"));
	USplineComponent* SplineComp = GetEditedSplineComponent();

	if (AActor* Owner = SplineComp->GetOwner())
	{
		Owner->Modify();
	}

	FInterpCurvePoint<FVector> KeyToDupe = SplineComp->SplineInfo.Points[SelectedKeyIndex];
	KeyToDupe.InterpMode = CIM_CurveAuto;
	int32 NewKeyIndex = SplineComp->SplineInfo.Points.Insert(KeyToDupe, SelectedKeyIndex);
	// move selection to 'next' key
	SelectedKeyIndex++;

	// Update Input value for all keys
	SplineComp->RefreshSplineInputs();

	NotifyComponentModified();
}


void FSplineComponentVisualizer::OnDeleteKey()
{
	const FScopedTransaction Transaction(LOCTEXT("DeleteKey", "Delete Spline Key"));
	USplineComponent* SplineComp = GetEditedSplineComponent();

	if (AActor* Owner = SplineComp->GetOwner())
	{
		Owner->Modify();
	}

	SplineComp->SplineInfo.Points.RemoveAt(SelectedKeyIndex);
	SplineComp->RefreshSplineInputs(); // update input value for each key

	SelectedKeyIndex = INDEX_NONE; // deselect any keys

	NotifyComponentModified();
}


bool FSplineComponentVisualizer::IsSelectionValid() const
{
	USplineComponent* SplineComp = GetEditedSplineComponent();
	return (SplineComp &&
		SelectedKeyIndex != INDEX_NONE &&
		SelectedKeyIndex < SplineComp->SplineInfo.Points.Num());
}


void FSplineComponentVisualizer::OnResetToAutomaticTangent(EInterpCurveMode Mode)
{
	USplineComponent* SplineComp = GetEditedSplineComponent();

	if (SplineComp->SplineInfo.Points[SelectedKeyIndex].InterpMode != Mode)
	{
		const FScopedTransaction Transaction(LOCTEXT("ResetToAutomaticTangent", "Reset to Automatic Tangent"));
		if (AActor* Owner = SplineComp->GetOwner())
		{
			Owner->Modify();
		}

		SplineComp->SplineInfo.Points[SelectedKeyIndex].InterpMode = Mode;
		NotifyComponentModified();
	}
}


bool FSplineComponentVisualizer::CanResetToAutomaticTangent(EInterpCurveMode Mode) const
{
	USplineComponent* SplineComp = GetEditedSplineComponent();
	return (SplineComp &&
		SelectedKeyIndex != INDEX_NONE &&
		SelectedKeyIndex < SplineComp->SplineInfo.Points.Num() &&
		SplineComp->SplineInfo.Points[SelectedKeyIndex].IsCurveKey() &&
		SplineComp->SplineInfo.Points[SelectedKeyIndex].InterpMode != Mode);
}


void FSplineComponentVisualizer::OnSetKeyType(EInterpCurveMode Mode)
{
	USplineComponent* SplineComp = GetEditedSplineComponent();

	if (SplineComp->SplineInfo.Points[SelectedKeyIndex].InterpMode != Mode)
	{
		const FScopedTransaction Transaction(LOCTEXT("ResetToAutomaticTangent", "Reset to Automatic Tangent"));
		if (AActor* Owner = SplineComp->GetOwner())
		{
			Owner->Modify();
		}

		SplineComp->SplineInfo.Points[SelectedKeyIndex].InterpMode = Mode;
		NotifyComponentModified();
	}
}


bool FSplineComponentVisualizer::IsKeyTypeSet(EInterpCurveMode Mode) const
{
	if (IsSelectionValid())
	{
		USplineComponent* SplineComp = GetEditedSplineComponent();

		const auto& SelectedPoint = SplineComp->SplineInfo.Points[SelectedKeyIndex];
		return ((Mode == CIM_CurveAuto && SelectedPoint.IsCurveKey()) || SelectedPoint.InterpMode == Mode);
	}

	return false;
}


TSharedPtr<SWidget> FSplineComponentVisualizer::GenerateContextMenu() const
{
	FMenuBuilder MenuBuilder(true, SplineComponentVisualizerActions);
	{
		MenuBuilder.BeginSection("SplineKeyEdit");
		{
			MenuBuilder.AddMenuEntry(FSplineComponentVisualizerCommands::Get().DeleteKey);
			MenuBuilder.AddMenuEntry(FSplineComponentVisualizerCommands::Get().DuplicateKey);

			MenuBuilder.AddSubMenu(
				LOCTEXT("KeyType", "Key Type"),
				LOCTEXT("KeyTypeTooltip", "Define the type of the spline key."),
				FNewMenuDelegate::CreateSP(this, &FSplineComponentVisualizer::GenerateSplinePointTypeSubMenu));

			// Only add the Automatic Tangents submenu if the key is a curve type
			USplineComponent* SplineComp = GetEditedSplineComponent();
			if (SplineComp &&
				SelectedKeyIndex != INDEX_NONE &&
				SelectedKeyIndex < SplineComp->SplineInfo.Points.Num() &&
				SplineComp->SplineInfo.Points[SelectedKeyIndex].IsCurveKey())
			{
				MenuBuilder.AddSubMenu(
					LOCTEXT("ResetToAutomaticTangent", "Reset to Automatic Tangent"),
					LOCTEXT("ResetToAutomaticTangentTooltip", "Reset the point tangent to an automatically generated value."),
					FNewMenuDelegate::CreateSP(this, &FSplineComponentVisualizer::GenerateTangentTypeSubMenu));
			}
		}
		MenuBuilder.EndSection();
	}

	TSharedPtr<SWidget> MenuWidget = MenuBuilder.MakeWidget();
	return MenuWidget;
}


void FSplineComponentVisualizer::GenerateSplinePointTypeSubMenu(FMenuBuilder& MenuBuilder) const
{
	MenuBuilder.AddMenuEntry(FSplineComponentVisualizerCommands::Get().SetKeyToCurve);
	MenuBuilder.AddMenuEntry(FSplineComponentVisualizerCommands::Get().SetKeyToLinear);
	MenuBuilder.AddMenuEntry(FSplineComponentVisualizerCommands::Get().SetKeyToConstant);
}


void FSplineComponentVisualizer::GenerateTangentTypeSubMenu(FMenuBuilder& MenuBuilder) const
{
	MenuBuilder.AddMenuEntry(FSplineComponentVisualizerCommands::Get().ResetToUnclampedTangent);
	MenuBuilder.AddMenuEntry(FSplineComponentVisualizerCommands::Get().ResetToClampedTangent);
}


void FSplineComponentVisualizer::NotifyComponentModified()
{
	USplineComponent* SplineComp = GetEditedSplineComponent();

	SplineComp->UpdateSpline();

	// Notify that the spline info has been modified
	UProperty* SplineInfoProperty = FindField<UProperty>(USplineComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(USplineComponent, SplineInfo));
	FPropertyChangedEvent PropertyChangedEvent(SplineInfoProperty);
	SplineComp->PostEditChangeProperty(PropertyChangedEvent);

	// Notify of change so any CS is re-run
	if (SplineOwningActor.IsValid())
	{
		SplineOwningActor.Get()->PostEditMove(true);
	}

	GEditor->RedrawLevelEditingViewports(true);
}

#undef LOCTEXT_NAMESPACE