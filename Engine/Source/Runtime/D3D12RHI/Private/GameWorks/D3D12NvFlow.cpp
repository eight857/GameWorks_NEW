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
	descD3D12->commandQueueFence = GetCommandListManager().GetFence().GetFenceCore()->GetFence();
	descD3D12->commandList = CommandListHandle.GraphicsCommandList();
	descD3D12->lastFenceCompleted = GetCommandListManager().GetFence().GetLastCompletedFence();
	descD3D12->nextFenceValue = GetCommandListManager().GetFence().GetCurrentFence();
}

void FD3D12CommandContext::NvFlowGetDepthStencilViewDesc(FRHINvFlowDepthStencilViewDesc* desc)
{
	FRHINvFlowDepthStencilViewDescD3D12* descD3D12 = static_cast<FRHINvFlowDepthStencilViewDescD3D12*>(desc);

	descD3D12->dsvHandle = CurrentDepthStencilTarget->GetView();
	descD3D12->dsvDesc   = CurrentDepthStencilTarget->GetDesc();
	descD3D12->srvHandle = CurrentDepthTexture->GetShaderResourceView()->GetView();
	descD3D12->srvDesc   = CurrentDepthTexture->GetShaderResourceView()->GetDesc();
	descD3D12->resource = CurrentDepthStencilTarget->GetResource()->GetResource();
	descD3D12->currentState = CurrentDepthStencilTarget->GetResource()->GetResourceState()->GetSubresourceState(0);

	StateCache.GetViewport(&descD3D12->viewport);
}

void FD3D12CommandContext::NvFlowGetRenderTargetViewDesc(FRHINvFlowRenderTargetViewDesc* desc)
{
	FRHINvFlowRenderTargetViewDescD3D12* descD3D12 = static_cast<FRHINvFlowRenderTargetViewDescD3D12*>(desc);

	descD3D12->rtvHandle = CurrentRenderTargets[0]->GetView();
	descD3D12->rtvDesc   = CurrentRenderTargets[0]->GetDesc();
	descD3D12->resource  = CurrentRenderTargets[0]->GetResource()->GetResource();
	descD3D12->currentState = CurrentRenderTargets[0]->GetResource()->GetResourceState()->GetSubresourceState(0);

	StateCache.GetViewport(&descD3D12->viewport);
	StateCache.GetScissorRect(&descD3D12->scissor);
}


class FD3D12NvFlowResourceRW : public FRHINvFlowResourceRW
{
public:
	FD3D12Resource Resource;
	FD3D12ResourceLocation ResourceLocation;
	D3D12_RESOURCE_STATES* ResourceState;

	FD3D12NvFlowResourceRW(
		FD3D12Device* InParent,
		GPUNodeMask VisibleNodes,
		ID3D12Resource* InResource,
		D3D12_RESOURCE_DESC const& InDesc,
		D3D12_RESOURCE_STATES* InResourceState
	)
		: Resource(InParent, VisibleNodes, InResource, *InResourceState, InDesc)
		, ResourceLocation(InParent)
		, ResourceState(InResourceState)
	{
		ResourceLocation.SetResource(&Resource);
	}
};

class FD3D12ShaderResourceViewNvFlow : public FD3D12ShaderResourceView
{
	TRefCountPtr<FD3D12NvFlowResourceRW> NvFlowResourceRWRef;

public:
	FD3D12ShaderResourceViewNvFlow(FD3D12Device* InParent, D3D12_SHADER_RESOURCE_VIEW_DESC* InSRVDesc, FD3D12NvFlowResourceRW* InNvFlowResourceRW)
		: FD3D12ShaderResourceView(InParent, InSRVDesc, &InNvFlowResourceRW->ResourceLocation)
		, NvFlowResourceRWRef(InNvFlowResourceRW)
	{
	}
};

FShaderResourceViewRHIRef FD3D12CommandContext::NvFlowCreateSRV(const FRHINvFlowResourceViewDesc* desc)
{
	const FRHINvFlowResourceViewDescD3D12* descD3D12 = static_cast<const FRHINvFlowResourceViewDescD3D12*>(desc);

	FD3D12NvFlowResourceRW* NvFlowResource = new FD3D12NvFlowResourceRW(
		GetParentDevice(), GetParentDevice()->GetNodeMask(), descD3D12->resource,
		descD3D12->resource->GetDesc(), descD3D12->currentState);

	// initialize CommandList resource state to avoid getting to PendingBarrierResources CommandList
	CResourceState& ResourceState = CommandListHandle.GetResourceState(&NvFlowResource->Resource);
	check(ResourceState.CheckResourceState(D3D12_RESOURCE_STATE_TBD));
	ResourceState.SetResourceState(*descD3D12->currentState);

	auto localSrvDesc = descD3D12->srvDesc;
	FD3D12ShaderResourceViewNvFlow* pSRV = new FD3D12ShaderResourceViewNvFlow(GetParentDevice(), &localSrvDesc, NvFlowResource);

	// make resource transition to SRV state
	const D3D12_RESOURCE_STATES TargetState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	FD3D12DynamicRHI::TransitionResource(CommandListHandle, pSRV, TargetState);
	*descD3D12->currentState = TargetState;

	return pSRV;
}


class FD3D12UnorderedAccessViewNvFlow : public FD3D12UnorderedAccessView
{
	TRefCountPtr<FD3D12NvFlowResourceRW> NvFlowResourceRWRef;

public:
	FD3D12UnorderedAccessViewNvFlow(FD3D12Device* InParent, D3D12_UNORDERED_ACCESS_VIEW_DESC* InUAVDesc, FD3D12NvFlowResourceRW* InNvFlowResourceRW)
		: FD3D12UnorderedAccessView(InParent, InUAVDesc, &InNvFlowResourceRW->ResourceLocation)
		, NvFlowResourceRWRef(InNvFlowResourceRW)
	{
	}
};

FRHINvFlowResourceRW* FD3D12CommandContext::NvFlowCreateResourceRW(const FRHINvFlowResourceRWViewDesc* desc, FShaderResourceViewRHIRef* pRHIRefSRV, FUnorderedAccessViewRHIRef* pRHIRefUAV)
{
	const FRHINvFlowResourceRWViewDescD3D12* descD3D12 = static_cast<const FRHINvFlowResourceRWViewDescD3D12*>(desc);

	FD3D12NvFlowResourceRW* NvFlowResourceRW = new FD3D12NvFlowResourceRW(
		GetParentDevice(), GetParentDevice()->GetNodeMask(), descD3D12->resourceView.resource,
		descD3D12->resourceView.resource->GetDesc(), descD3D12->resourceView.currentState);

	// initialize CommandList resource state to avoid getting to PendingBarrierResources CommandList
	CResourceState& ResourceState = CommandListHandle.GetResourceState(&NvFlowResourceRW->Resource);
	check(ResourceState.CheckResourceState(D3D12_RESOURCE_STATE_TBD));
	ResourceState.SetResourceState(*descD3D12->resourceView.currentState);

	if (pRHIRefSRV)
	{
		auto localSrvDesc = descD3D12->resourceView.srvDesc;
		FD3D12ShaderResourceViewNvFlow* pSRV = new FD3D12ShaderResourceViewNvFlow(GetParentDevice(), &localSrvDesc, NvFlowResourceRW);
		check(pSRV->GetViewSubresourceSubset().IsWholeResource());
		*pRHIRefSRV = pSRV;
	}
	if (pRHIRefUAV)
	{
		auto localUavDesc = descD3D12->uavDesc;
		FD3D12UnorderedAccessViewNvFlow* pUAV = new FD3D12UnorderedAccessViewNvFlow(GetParentDevice(), &localUavDesc, NvFlowResourceRW);
		check(pUAV->GetViewSubresourceSubset().IsWholeResource());
		*pRHIRefUAV = pUAV;
	}

	NvFlowResourceRW->AddRef();
	return NvFlowResourceRW;
}

void FD3D12CommandContext::NvFlowReleaseResourceRW(FRHINvFlowResourceRW* InNvFlowResourceRW)
{
	FD3D12NvFlowResourceRW* NvFlowResourceRW = static_cast<FD3D12NvFlowResourceRW*>(InNvFlowResourceRW);

	// pass updated resource state back to NvFlow!
	CResourceState& LastResourceState = CommandListHandle.GetResourceState(&NvFlowResourceRW->Resource);
	check(!LastResourceState.CheckResourceState(D3D12_RESOURCE_STATE_TBD));
	check(LastResourceState.AreAllSubresourcesSame());
	*NvFlowResourceRW->ResourceState = LastResourceState.GetSubresourceState(0);

	NvFlowResourceRW->Release();
}

void FD3D12CommandContext::NvFlowRestoreState()
{
	CommandListHandle->SetDescriptorHeaps(DescriptorHeaps.Num(), DescriptorHeaps.GetData());
	StateCache.ForceSetGraphicsRootSignature();
	StateCache.ForceSetComputeRootSignature();

	StateCache.GetDescriptorCache()->NotifyCurrentCommandList(CommandListHandle);

	StateCache.RestoreState();
}

// NvFlow end