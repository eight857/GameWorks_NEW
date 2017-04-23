#pragma once

#ifndef NVFLOW_ADAPTIVE
#define NVFLOW_ADAPTIVE 0
#endif

#ifndef WITH_CUDA_CONTEXT
#define WITH_CUDA_CONTEXT 0
#endif

#ifndef NVFLOW_SMP
#define NVFLOW_SMP 0
#endif

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
