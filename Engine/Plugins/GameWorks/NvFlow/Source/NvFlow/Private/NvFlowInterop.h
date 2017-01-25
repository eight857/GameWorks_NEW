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
class NvFlowInterop
{
public:
	virtual NvFlowContext* CreateContext(IRHICommandContext& RHICmdCtx) = 0;
	virtual NvFlowDepthStencilView* CreateDepthStencilView(IRHICommandContext& RHICmdCtx, NvFlowContext* context) = 0;
	virtual NvFlowRenderTargetView* CreateRenderTargetView(IRHICommandContext& RHICmdCtx, NvFlowContext* context) = 0;
	virtual void UpdateContext(IRHICommandContext& RHICmdCtx, NvFlowContext* context) = 0;
	virtual void UpdateDepthStencilView(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowDepthStencilView* view) = 0;
	virtual void UpdateRenderTargetView(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowRenderTargetView* view) = 0;
	virtual void Push(IRHICommandContext& RHICmdCtx, NvFlowContext* context) = 0;
	virtual void Pop(IRHICommandContext& RHICmdCtx, NvFlowContext* context) = 0;
	virtual void CleanupFunc(IRHICommandContext& RHICmdCtx, void(*func)(void*), void* ptr) = 0;

	virtual FShaderResourceViewRHIRef ConvertSRV(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowResource* resource) = 0;
	virtual FUnorderedAccessViewRHIRef ConvertUAV(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowResourceRW* resourceRW, FShaderResourceViewRHIRef* pRHIRefSRV = nullptr) = 0;
};

NvFlowInterop* NvFlowCreateInteropD3D11();
NvFlowInterop* NvFlowCreateInteropD3D12();

inline void NvFlowReleaseInterop(NvFlowInterop* flowInterop)
{
	delete flowInterop;
}

#endif // WITH_NVFLOW