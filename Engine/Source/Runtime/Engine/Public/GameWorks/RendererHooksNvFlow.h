#pragma once

// NvFlow begin

struct RendererHooksNvFlow
{
	virtual void NvFlowUpdateScene(FRHICommandListImmediate& RHICmdList, TArray<FPrimitiveSceneInfo*>& Primitives) = 0;
	virtual void NvFlowDoRenderBegin(FRHICommandListImmediate& RHICmdList, const FViewInfo& View) = 0;
	virtual void NvFlowDoRenderPrimitive(FRHICommandList& RHICmdList, const FViewInfo& View, FPrimitiveSceneInfo* PrimitiveSceneInfo) = 0;
	virtual void NvFlowDoRenderEnd(FRHICommandListImmediate& RHICmdList, const FViewInfo& View) = 0;
};

extern ENGINE_API struct RendererHooksNvFlow* GRendererNvFlowHooks;

// NvFlow end