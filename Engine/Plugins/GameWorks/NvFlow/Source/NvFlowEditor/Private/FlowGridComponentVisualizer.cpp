#include "NvFlowEditorPCH.h"

// NvFlow begin

#include "FlowGridComponentVisualizer.h"

void FFlowGridComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const UFlowGridComponent* FlowGridComp = Cast<const UFlowGridComponent>(Component);
	if (FlowGridComp != NULL && FlowGridComp->FlowGridAsset != NULL)
	{
		FTransform TM = FlowGridComp->ComponentToWorld;
		FVector BoxExtents(FlowGridComp->FlowGridAsset->GetVirtualGridExtent());
		FBox Box = FBox(TM.GetTranslation() - BoxExtents, TM.GetTranslation() + BoxExtents);
		DrawWireBox(PDI, Box, FColor(200, 255, 255), SDPG_World, 2.0f);
	}
}
// NvFlow end