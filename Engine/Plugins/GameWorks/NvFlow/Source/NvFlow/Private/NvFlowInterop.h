#pragma once

// NvFlow begin
#if WITH_NVFLOW
struct NvFlowContext;
struct NvFlowDepthStencilView;
struct NvFlowRenderTargetView;
#endif // WITH_NVFLOW
// NvFlow end

// NvFlow begin
#if WITH_NVFLOW
NvFlowContext* NvFlowInteropCreateContext(IRHICommandContext& RHICmdCtx);
NvFlowDepthStencilView* NvFlowInteropCreateDepthStencilView(IRHICommandContext& RHICmdCtx, NvFlowContext* context);
NvFlowRenderTargetView* NvFlowInteropCreateRenderTargetView(IRHICommandContext& RHICmdCtx, NvFlowContext* context);
void NvFlowInteropUpdateContext(IRHICommandContext& RHICmdCtx, NvFlowContext* context);
void NvFlowInteropUpdateDepthStencilView(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowDepthStencilView* view);
void NvFlowInteropUpdateRenderTargetView(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowRenderTargetView* view);
void NvFlowInteropPush(IRHICommandContext& RHICmdCtx, NvFlowContext* context);
void NvFlowInteropPop(IRHICommandContext& RHICmdCtx, NvFlowContext* context);
void NvFlowCleanupFunc(IRHICommandContext& RHICmdCtx, void(*func)(void*), void* ptr);
#endif // WITH_NVFLOW