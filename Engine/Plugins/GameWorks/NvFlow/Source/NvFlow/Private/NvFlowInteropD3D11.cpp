#include "NvFlowPCH.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#include <d3d11.h>
#include "HideWindowsPlatformTypes.h"
#endif

#include "GameWorks/RHINvFlowD3D11.h"

#include "NvFlow.h"
#include "NvFlowContext.h"
#include "NvFlowContextD3D11.h"

class NvFlowInteropD3D11 : public NvFlowInterop
{
public:
	virtual NvFlowContext* CreateContext(IRHICommandContext& RHICmdCtx)
	{
		FRHINvFlowDeviceDescD3D11 deviceDesc = {};
		RHICmdCtx.NvFlowGetDeviceDesc(&deviceDesc);

		NvFlowContextDescD3D11 desc = {};
		desc.device = deviceDesc.device;
		desc.deviceContext = deviceDesc.deviceContext;

		return NvFlowCreateContextD3D11(NV_FLOW_VERSION, &desc);
	}

	virtual NvFlowDepthStencilView* CreateDepthStencilView(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
	{
		FRHINvFlowDepthStencilViewDescD3D11 dsvDesc = {};
		RHICmdCtx.NvFlowGetDepthStencilViewDesc(&dsvDesc);

		NvFlowDepthStencilViewDescD3D11 desc = {};
		desc.dsv = dsvDesc.dsv;
		desc.srv = dsvDesc.srv;
		desc.viewport = dsvDesc.viewport;

		return NvFlowCreateDepthStencilViewD3D11(context, &desc);
	}

	virtual NvFlowRenderTargetView* CreateRenderTargetView(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
	{
		FRHINvFlowRenderTargetViewDescD3D11 rtvDesc = {};
		RHICmdCtx.NvFlowGetRenderTargetViewDesc(&rtvDesc);

		NvFlowRenderTargetViewDescD3D11 desc = {};
		desc.rtv = rtvDesc.rtv;
		desc.viewport = rtvDesc.viewport;

		return NvFlowCreateRenderTargetViewD3D11(context, &desc);
	}

	virtual void UpdateContext(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
	{
		FRHINvFlowDeviceDescD3D11 deviceDesc = {};
		RHICmdCtx.NvFlowGetDeviceDesc(&deviceDesc);

		NvFlowContextDescD3D11 desc = {};
		desc.device = deviceDesc.device;
		desc.deviceContext = deviceDesc.deviceContext;

		NvFlowUpdateContextD3D11(context, &desc);
	}

	virtual void UpdateDepthStencilView(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowDepthStencilView* view)
	{
		FRHINvFlowDepthStencilViewDescD3D11 dsvDesc = {};
		RHICmdCtx.NvFlowGetDepthStencilViewDesc(&dsvDesc);

		NvFlowDepthStencilViewDescD3D11 desc = {};
		desc.dsv = dsvDesc.dsv;
		desc.srv = dsvDesc.srv;
		desc.viewport = dsvDesc.viewport;

		NvFlowUpdateDepthStencilViewD3D11(context, view, &desc);
	}

	virtual void UpdateRenderTargetView(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowRenderTargetView* view)
	{
		FRHINvFlowRenderTargetViewDescD3D11 rtvDesc = {};
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

			FRHINvFlowResourceViewDescD3D11 viewDescRHI;
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

			FRHINvFlowResourceViewDescD3D11 viewDescRHI;
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

			FRHINvFlowResourceRWViewDescD3D11 viewDescRHI;
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
