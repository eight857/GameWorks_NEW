#include "NvFlowPCH.h"

#include "D3D11RHIPrivate.h"

#include "GameWorks/RHINvFlowD3D11.h"

#include "NvFlow.h"
#include "NvFlowContext.h"
#include "NvFlowContextD3D11.h"

class NvFlowInteropD3D11 : public NvFlowInterop
{
public:
	virtual NvFlowContext* CreateContext(IRHICommandContext& RHICmdCtx)
	{
		FRHINvFlowDeviceDesc deviceDesc = {};
		RHICmdCtx.NvFlowGetDeviceDesc(&deviceDesc);

		NvFlowContextDescD3D11 desc = {};
		desc.device = deviceDesc.device;
		desc.deviceContext = deviceDesc.deviceContext;

		return NvFlowCreateContextD3D11(NV_FLOW_VERSION, &desc);
	}

	virtual NvFlowDepthStencilView* CreateDepthStencilView(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
	{
		FRHINvFlowDepthStencilViewDesc dsvDesc = {};
		RHICmdCtx.NvFlowGetDepthStencilViewDesc(&dsvDesc);

		NvFlowDepthStencilViewDescD3D11 desc = {};
		desc.dsv = dsvDesc.dsv;
		desc.srv = dsvDesc.srv;
		desc.viewport = dsvDesc.viewport;

		return NvFlowCreateDepthStencilViewD3D11(context, &desc);
	}

	virtual NvFlowRenderTargetView* CreateRenderTargetView(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
	{
		FRHINvFlowRenderTargetViewDesc rtvDesc = {};
		RHICmdCtx.NvFlowGetRenderTargetViewDesc(&rtvDesc);

		NvFlowRenderTargetViewDescD3D11 desc = {};
		desc.rtv = rtvDesc.rtv;
		desc.viewport = rtvDesc.viewport;

		return NvFlowCreateRenderTargetViewD3D11(context, &desc);
	}

	virtual void UpdateContext(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
	{
		FRHINvFlowDeviceDesc deviceDesc = {};
		RHICmdCtx.NvFlowGetDeviceDesc(&deviceDesc);

		NvFlowContextDescD3D11 desc = {};
		desc.device = deviceDesc.device;
		desc.deviceContext = deviceDesc.deviceContext;

		NvFlowUpdateContextD3D11(context, &desc);
	}

	virtual void UpdateDepthStencilView(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowDepthStencilView* view)
	{
		FRHINvFlowDepthStencilViewDesc dsvDesc = {};
		RHICmdCtx.NvFlowGetDepthStencilViewDesc(&dsvDesc);

		NvFlowDepthStencilViewDescD3D11 desc = {};
		desc.dsv = dsvDesc.dsv;
		desc.srv = dsvDesc.srv;
		desc.viewport = dsvDesc.viewport;

		NvFlowUpdateDepthStencilViewD3D11(context, view, &desc);
	}

	virtual void UpdateRenderTargetView(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowRenderTargetView* view)
	{
		FRHINvFlowRenderTargetViewDesc rtvDesc = {};
		RHICmdCtx.NvFlowGetRenderTargetViewDesc(&rtvDesc);

		NvFlowRenderTargetViewDescD3D11 desc = {};
		desc.rtv = rtvDesc.rtv;
		desc.viewport = rtvDesc.viewport;

		NvFlowUpdateRenderTargetViewD3D11(context, view, &desc);
	}

	virtual void Push(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
	{
		NvFlowContextPush(context);
	}

	virtual void Pop(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
	{
		NvFlowContextPop(context);
	}

	virtual void CleanupFunc(IRHICommandContext& RHICmdCtx, void(*func)(void*), void* ptr)
	{
		RHICmdCtx.NvFlowCleanup.Set(func, ptr);
	}

	virtual FShaderResourceViewRHIRef ConvertSRV(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowResource* resource)
	{
		if (resource)
		{
			NvFlowResourceViewDescD3D11 viewDesc;
			NvFlowUpdateResourceViewDescD3D11(context, resource, &viewDesc);

			FRHINvFlowResourceViewDesc viewDescRHI;
			viewDescRHI.srv = viewDesc.srv;
			return RHICmdCtx.NvFlowCreateSRV(&viewDescRHI);
		}
		return FShaderResourceViewRHIRef();
	}

	virtual FShaderResourceViewRHIRef ConvertSRV(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowResourceRW* resourceRW)
	{
		if (resourceRW)
		{
			NvFlowResourceRWViewDescD3D11 viewDesc;
			NvFlowUpdateResourceRWViewDescD3D11(context, resourceRW, &viewDesc);

			FRHINvFlowResourceViewDesc viewDescRHI;
			viewDescRHI.srv = viewDesc.resourceView.srv;
			return RHICmdCtx.NvFlowCreateSRV(&viewDescRHI);
		}
		return FShaderResourceViewRHIRef();
	}

	virtual FUnorderedAccessViewRHIRef ConvertUAV(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowResourceRW* resourceRW)
	{
		if (resourceRW)
		{
			NvFlowResourceRWViewDescD3D11 viewDesc;
			NvFlowUpdateResourceRWViewDescD3D11(context, resourceRW, &viewDesc);

			FRHINvFlowResourceRWViewDesc viewDescRHI;
			viewDescRHI.uav = viewDesc.uav;
			return RHICmdCtx.NvFlowCreateUAV(&viewDescRHI);
		}
		return FUnorderedAccessViewRHIRef();
	}

};

NvFlowInterop* NvFlowCreateInteropD3D11()
{
	return new NvFlowInteropD3D11();
}
