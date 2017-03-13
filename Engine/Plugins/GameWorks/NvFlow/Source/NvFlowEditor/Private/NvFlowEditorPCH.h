#pragma once

DECLARE_LOG_CATEGORY_EXTERN(LogNvFlowEditor, Verbose, All);

#define NV_FLOW_SHADER_UTILS 0
#include "NvFlow.h"
#include "NvFlowContext.h"

#include "NvFlowEditorModule.h"

#include "FlowGridAsset.h"
#include "FlowMaterial.h"
#include "FlowRenderMaterial.h"
#include "FlowGridComponent.h"
#include "FlowGridActor.h"
#include "ActorFactoryFlowGridActor.h"
#include "FlowGridAssetFactory.h"
#include "FlowMaterialFactory.h"
#include "FlowRenderMaterialFactory.h"
#include "AssetTypeActions_FlowGridAsset.h"
#include "AssetTypeActions_FlowMaterial.h"
#include "AssetTypeActions_FlowRenderMaterial.h"

#include "FlowGridComponentVisualizer.h"
