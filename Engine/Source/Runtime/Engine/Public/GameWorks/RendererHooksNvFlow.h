#pragma once

// NvFlow begin

struct RendererHooksNvFlow
{
	virtual void NvFlowUpdateScene(FRHICommandListImmediate& RHICmdList, TArray<FPrimitiveSceneInfo*>& Primitives) = 0;
	virtual bool NvFlowDoRenderPrimitive(FRHICommandList& RHICmdList, const FViewInfo& View, FPrimitiveSceneInfo* PrimitiveSceneInfo) = 0;
	virtual void NvFlowDoRenderFinish(FRHICommandListImmediate& RHICmdList, const FViewInfo& View) = 0;
};

extern ENGINE_API struct RendererHooksNvFlow* GRendererNvFlowHooks;

class FComponentVisualizersModule;

struct EditorRendererHooksNvFlow
{
	virtual void NvFlowRegisterVisualizer(FComponentVisualizersModule* module) = 0;
};

extern ENGINE_API struct EditorRendererHooksNvFlow* GEditorRendererHooksNvFlow;

// NvFlow end