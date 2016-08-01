#pragma once

// NvFlow begin

struct RendererHooksNvFlow
{
	virtual void NvFlowUpdateScene(FRHICommandListImmediate& RHICmdList, TArray<FPrimitiveSceneInfo*>& Primitives) = 0;
	virtual void NvFlowDoRender(FRHICommandListImmediate& RHICmdList, const FViewInfo& View) = 0;
};

extern ENGINE_API struct RendererHooksNvFlow* GRendererNvFlowHooks;

// NvFlow end