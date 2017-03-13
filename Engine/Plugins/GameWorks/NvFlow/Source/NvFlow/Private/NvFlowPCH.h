#pragma once

#include "UObject/ObjectMacros.h"
#include "ConstructorHelpers.h"
#include "Engine/CollisionProfile.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNvFlow, Verbose, All);

#define NV_FLOW_SHADER_UTILS 0
#include "NvFlow.h"
#include "NvFlowContext.h"

#include "NvFlowModule.h"

#include "FlowGridAsset.h"
#include "FlowGridComponent.h"
#include "FlowEmitterComponent.h"
#include "FlowGridActor.h"
#include "FlowGridSceneProxy.h"
#include "FlowMaterial.h"
#include "FlowRenderMaterial.h"

#include "NvFlowInterop.h"
