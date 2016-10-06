#include "NvFlowPCH.h"

#include "D3D11RHIPrivate.h"

#include "GameWorks/RHINvFlowD3D11.h"

#include "NvFlow.h"
#include "NvFlowContext.h"
#include "NvFlowContextD3D11.h"

NvFlowContext* NvFlowInteropCreateContext(IRHICommandContext& RHICmdCtx)
{
	FRHINvFlowDeviceDesc deviceDesc = {};
	RHICmdCtx.NvFlowGetDeviceDesc(&deviceDesc);

	NvFlowContextDesc desc = {};
	desc.device = deviceDesc.device;
	desc.deviceContext = deviceDesc.deviceContext;

	return NvFlowCreateContext(NV_FLOW_VERSION, &desc);
}

NvFlowDepthStencilView* NvFlowInteropCreateDepthStencilView(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
{
	FRHINvFlowDepthStencilViewDesc dsvDesc = {};
	RHICmdCtx.NvFlowGetDepthStencilViewDesc(&dsvDesc);

	NvFlowDepthStencilViewDesc desc = {};
	desc.dsv = dsvDesc.dsv;
	desc.srv = dsvDesc.srv;
	desc.viewport = dsvDesc.viewport;

	return NvFlowCreateDepthStencilView(context, &desc);
}

NvFlowRenderTargetView* NvFlowInteropCreateRenderTargetView(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
{
	FRHINvFlowRenderTargetViewDesc rtvDesc = {};
	RHICmdCtx.NvFlowGetRenderTargetViewDesc(&rtvDesc);

	NvFlowRenderTargetViewDesc desc = {};
	desc.rtv = rtvDesc.rtv;
	desc.viewport = rtvDesc.viewport;

	return NvFlowCreateRenderTargetView(context, &desc);
}

void NvFlowInteropUpdateContext(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
{
	FRHINvFlowDeviceDesc deviceDesc = {};
	RHICmdCtx.NvFlowGetDeviceDesc(&deviceDesc);

	NvFlowContextDesc desc = {};
	desc.device = deviceDesc.device;
	desc.deviceContext = deviceDesc.deviceContext;

	NvFlowUpdateContext(context, &desc);
}

void NvFlowInteropUpdateDepthStencilView(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowDepthStencilView* view)
{
	FRHINvFlowDepthStencilViewDesc dsvDesc = {};
	RHICmdCtx.NvFlowGetDepthStencilViewDesc(&dsvDesc);

	NvFlowDepthStencilViewDesc desc = {};
	desc.dsv = dsvDesc.dsv;
	desc.srv = dsvDesc.srv;
	desc.viewport = dsvDesc.viewport;

	NvFlowUpdateDepthStencilView(context, view, &desc);
}

void NvFlowInteropUpdateRenderTargetView(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowRenderTargetView* view)
{
	FRHINvFlowRenderTargetViewDesc rtvDesc = {};
	RHICmdCtx.NvFlowGetRenderTargetViewDesc(&rtvDesc);

	NvFlowRenderTargetViewDesc desc = {};
	desc.rtv = rtvDesc.rtv;
	desc.viewport = rtvDesc.viewport;

	NvFlowUpdateRenderTargetView(context, view, &desc);
}

void NvFlowInteropPush(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
{
	NvFlowContextPush(context);
}

void NvFlowInteropPop(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
{
	NvFlowContextPop(context);
}

void NvFlowCleanupFunc(IRHICommandContext& RHICmdCtx, void(*func)(void*), void* ptr)
{
	RHICmdCtx.NvFlowCleanup.Set(func, ptr);
}