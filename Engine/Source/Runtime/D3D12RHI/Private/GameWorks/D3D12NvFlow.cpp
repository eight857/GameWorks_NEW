// NvFlow begin

/*=============================================================================
D3D12NvFlow.cpp: D3D device RHI NvFlow interop implementation.
=============================================================================*/

#include "D3D12RHIPrivate.h"

#include "GameWorks/RHINvFlowD3D12.h"

void FD3D12CommandContext::NvFlowGetDeviceDesc(FRHINvFlowDeviceDesc* desc)
{
	FRHINvFlowDeviceDescD3D12* descD3D12 = static_cast<FRHINvFlowDeviceDescD3D12*>(desc);

	descD3D12->device = GetParentDevice()->GetDevice();
	descD3D12->commandQueue = GetCommandListManager().GetD3DCommandQueue();
	descD3D12->commandQueueFence = GetCommandListManager().GetFence().GetFenceCode()->GetFence();
	descD3D12->commandList = CommandListHandle.GraphicsCommandList();
	//TODO: check that these values are correct!
	descD3D12->lastFenceCompleted = GetCommandListManager().GetFence().GetLastCompletedFence();
	descD3D12->nextFenceValue = GetCommandListManager().GetFence().GetCurrentFence();
}

void FD3D12CommandContext::NvFlowGetDepthStencilViewDesc(FRHINvFlowDepthStencilViewDesc* desc)
{
	FRHINvFlowDepthStencilViewDescD3D12* descD3D12 = static_cast<FRHINvFlowDepthStencilViewDescD3D12*>(desc);

	descD3D12->dsv = CurrentDepthStencilTarget->GetView();
	descD3D12->srv = CurrentDepthTexture->GetShaderResourceView()->GetView();
	descD3D12->dsv_format = CurrentDepthStencilTarget->GetDesc().Format;
	descD3D12->srv_format = CurrentDepthTexture->GetShaderResourceView()->GetDesc().Format;
	descD3D12->resource = CurrentDepthStencilTarget->GetResource()->GetResource();
	descD3D12->currentState = CurrentDepthStencilTarget->GetResource()->GetResourceState()->GetSubresourceState(0);
	descD3D12->width = CurrentDepthStencilTarget->GetResource()->GetDesc().Width;
	descD3D12->height = CurrentDepthStencilTarget->GetResource()->GetDesc().Height;

	StateCache.GetViewport(&descD3D12->viewport);
}

void FD3D12CommandContext::NvFlowGetRenderTargetViewDesc(FRHINvFlowRenderTargetViewDesc* desc)
{
	FRHINvFlowRenderTargetViewDescD3D12* descD3D12 = static_cast<FRHINvFlowRenderTargetViewDescD3D12*>(desc);

	descD3D12->rtv = CurrentRenderTargets[0]->GetView();
	descD3D12->rtv_format = CurrentRenderTargets[0]->GetDesc().Format;
	descD3D12->resource = CurrentRenderTargets[0]->GetResource()->GetResource();
	descD3D12->currentState = CurrentRenderTargets[0]->GetResource()->GetResourceState()->GetSubresourceState(0);

	StateCache.GetViewport(&descD3D12->viewport);
	StateCache.GetScissorRect(&descD3D12->scissor);
}

FShaderResourceViewRHIRef FD3D12CommandContext::NvFlowCreateSRV(const FRHINvFlowResourceViewDesc* desc)
{
	const FRHINvFlowResourceViewDescD3D12* descD3D12 = static_cast<const FRHINvFlowResourceViewDescD3D12*>(desc);

	//TODO: implement
	return nullptr;
}

FUnorderedAccessViewRHIRef FD3D12CommandContext::NvFlowCreateUAV(const FRHINvFlowResourceRWViewDesc* desc)
{
	const FRHINvFlowResourceRWViewDescD3D12* descD3D12 = static_cast<const FRHINvFlowResourceRWViewDescD3D12*>(desc);

	//TODO: implement
	return nullptr;
}

// NvFlow end