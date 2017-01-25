#include "NvFlowPCH.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#include <d3d12.h>
#include "HideWindowsPlatformTypes.h"
#endif

#include "GameWorks/RHINvFlowD3D12.h"

#include "NvFlow.h"
#include "NvFlowContext.h"
#include "NvFlowContextD3D12.h"

class NvFlowInteropD3D12 : public NvFlowInterop
{
public:
	virtual NvFlowContext* CreateContext(IRHICommandContext& RHICmdCtx)
	{
		FRHINvFlowDeviceDescD3D12 deviceDesc = {};
		RHICmdCtx.NvFlowGetDeviceDesc(&deviceDesc);

		NvFlowContextDescD3D12 desc = {};
		desc.device = deviceDesc.device;
		desc.commandQueue = deviceDesc.commandQueue;
		desc.commandQueueFence = deviceDesc.commandQueueFence;
		desc.commandList = deviceDesc.commandList;
		desc.lastFenceCompleted = deviceDesc.lastFenceCompleted;
		desc.nextFenceValue = deviceDesc.nextFenceValue;

		return NvFlowCreateContextD3D12(NV_FLOW_VERSION, &desc);
	}

	virtual NvFlowDepthStencilView* CreateDepthStencilView(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
	{
		FRHINvFlowDepthStencilViewDescD3D12 dsvDesc = {};
		RHICmdCtx.NvFlowGetDepthStencilViewDesc(&dsvDesc);

		NvFlowDepthStencilViewDescD3D12 desc = {};
		desc.dsvHandle = dsvDesc.dsvHandle;
		desc.dsvDesc   = dsvDesc.dsvDesc;
		desc.srvHandle = dsvDesc.srvHandle;
		desc.srvDesc   = dsvDesc.srvDesc;
		desc.resource = dsvDesc.resource;
		desc.currentState = dsvDesc.currentState;
		desc.viewport = dsvDesc.viewport;

		return NvFlowCreateDepthStencilViewD3D12(context, &desc);
	}

	virtual NvFlowRenderTargetView* CreateRenderTargetView(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
	{
		FRHINvFlowRenderTargetViewDescD3D12 rtvDesc = {};
		RHICmdCtx.NvFlowGetRenderTargetViewDesc(&rtvDesc);

		NvFlowRenderTargetViewDescD3D12 desc = {};
		desc.rtvHandle = rtvDesc.rtvHandle;
		desc.rtvDesc   = rtvDesc.rtvDesc;
		desc.resource = rtvDesc.resource;
		desc.currentState = rtvDesc.currentState;
		desc.viewport = rtvDesc.viewport;
		desc.scissor = rtvDesc.scissor;

		return NvFlowCreateRenderTargetViewD3D12(context, &desc);
	}

	virtual void UpdateContext(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
	{
		FRHINvFlowDeviceDescD3D12 deviceDesc = {};
		RHICmdCtx.NvFlowGetDeviceDesc(&deviceDesc);

		NvFlowContextDescD3D12 desc = {};
		desc.device = deviceDesc.device;
		desc.commandQueue = deviceDesc.commandQueue;
		desc.commandQueueFence = deviceDesc.commandQueueFence;
		desc.commandList = deviceDesc.commandList;
		desc.lastFenceCompleted = deviceDesc.lastFenceCompleted;
		desc.nextFenceValue = deviceDesc.nextFenceValue;

		NvFlowUpdateContextD3D12(context, &desc);
	}

	virtual void UpdateDepthStencilView(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowDepthStencilView* view)
	{
		FRHINvFlowDepthStencilViewDescD3D12 dsvDesc = {};
		RHICmdCtx.NvFlowGetDepthStencilViewDesc(&dsvDesc);

		NvFlowDepthStencilViewDescD3D12 desc = {};
		desc.dsvHandle = dsvDesc.dsvHandle;
		desc.dsvDesc   = dsvDesc.dsvDesc;
		desc.srvHandle = dsvDesc.srvHandle;
		desc.srvDesc   = dsvDesc.srvDesc;
		desc.resource = dsvDesc.resource;
		desc.currentState = dsvDesc.currentState;
		desc.viewport = dsvDesc.viewport;

		NvFlowUpdateDepthStencilViewD3D12(context, view, &desc);
	}

	virtual void UpdateRenderTargetView(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowRenderTargetView* view)
	{
		FRHINvFlowRenderTargetViewDescD3D12 rtvDesc = {};
		RHICmdCtx.NvFlowGetRenderTargetViewDesc(&rtvDesc);

		NvFlowRenderTargetViewDescD3D12 desc = {};
		desc.rtvHandle = rtvDesc.rtvHandle;
		desc.rtvDesc   = rtvDesc.rtvDesc;
		desc.resource = rtvDesc.resource;
		desc.currentState = rtvDesc.currentState;
		desc.viewport = rtvDesc.viewport;
		desc.scissor = rtvDesc.scissor;

		NvFlowUpdateRenderTargetViewD3D12(context, view, &desc);
	}

	virtual void Push(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
	{
		NvFlowContextPush(context);
	}

	virtual void Pop(IRHICommandContext& RHICmdCtx, NvFlowContext* context)
	{
		NvFlowContextPop(context);

		RHICmdCtx.NvFlowRestoreState();
	}

	virtual void CleanupFunc(IRHICommandContext& RHICmdCtx, void(*func)(void*), void* ptr)
	{
		RHICmdCtx.NvFlowCleanup.Set(func, ptr);
	}

	virtual FShaderResourceViewRHIRef ConvertSRV(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowResource* resource)
	{
		if (resource)
		{
			NvFlowResourceViewDescD3D12 viewDesc;
			NvFlowUpdateResourceViewDescD3D12(context, resource, &viewDesc);

			FRHINvFlowResourceViewDescD3D12 viewDescRHI;
			viewDescRHI.srvHandle = viewDesc.srvHandle;
			viewDescRHI.srvDesc = viewDesc.srvDesc;
			viewDescRHI.resource = viewDesc.resource;
			viewDescRHI.currentState = viewDesc.currentState;
			return RHICmdCtx.NvFlowCreateSRV(&viewDescRHI);
		}
		return FShaderResourceViewRHIRef();
	}

	virtual FUnorderedAccessViewRHIRef ConvertUAV(IRHICommandContext& RHICmdCtx, NvFlowContext* context, NvFlowResourceRW* resourceRW, FShaderResourceViewRHIRef* pRHIRefSRV)
	{
		if (resourceRW)
		{
			NvFlowResourceRWViewDescD3D12 viewDesc;
			NvFlowUpdateResourceRWViewDescD3D12(context, resourceRW, &viewDesc);

			FRHINvFlowResourceRWViewDescD3D12 viewDescRHI;
			viewDescRHI.resourceView.srvHandle = viewDesc.resourceView.srvHandle;
			viewDescRHI.resourceView.srvDesc = viewDesc.resourceView.srvDesc;
			viewDescRHI.resourceView.resource = viewDesc.resourceView.resource;
			viewDescRHI.resourceView.currentState = viewDesc.resourceView.currentState;
			viewDescRHI.uavHandle = viewDesc.uavHandle;
			viewDescRHI.uavDesc = viewDesc.uavDesc;
			return RHICmdCtx.NvFlowCreateUAV(&viewDescRHI, pRHIRefSRV);
		}
		return FUnorderedAccessViewRHIRef();
	}

};

NvFlowInterop* NvFlowCreateInteropD3D12()
{
	return new NvFlowInteropD3D12();
}
