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

class FD3D12ResourceNvFlow : public FRHIResource
{
public:
	FD3D12Resource Resource;
	FD3D12ResourceLocation ResourceLocation;
	D3D12_RESOURCE_STATES* ResourceState;

	FD3D12ResourceNvFlow(
		FD3D12Device* InParent,
		GPUNodeMask VisibleNodes,
		ID3D12Resource* InResource,
		D3D12_RESOURCE_STATES InitialState,
		D3D12_RESOURCE_DESC const& InDesc,
		D3D12_RESOURCE_STATES* InResourceState
	)
		: Resource(InParent, VisibleNodes, InResource, InitialState, InDesc)
		, ResourceLocation(InParent)
		, ResourceState(InResourceState)
	{
		ResourceLocation.SetResource(&Resource);
		Resource.bIsNvFlowResource = true;
		Resource.NvFlowResourceState = ResourceState;
	}
};

class FD3D12ShaderResourceViewNvFlow : public FD3D12ShaderResourceView
{
	TRefCountPtr<FD3D12ResourceNvFlow> ResourceRef;

public:
	FD3D12ShaderResourceViewNvFlow(FD3D12Device* InParent, D3D12_SHADER_RESOURCE_VIEW_DESC* InSRVDesc,
		const TRefCountPtr<FD3D12ResourceNvFlow>& InResourceRef)
		: FD3D12ShaderResourceView(InParent, InSRVDesc, &InResourceRef->ResourceLocation)
		, ResourceRef(InResourceRef)
	{
	}
};

FShaderResourceViewRHIRef FD3D12CommandContext::NvFlowCreateSRV(const FRHINvFlowResourceViewDesc* desc)
{
	const FRHINvFlowResourceViewDescD3D12* descD3D12 = static_cast<const FRHINvFlowResourceViewDescD3D12*>(desc);

	TRefCountPtr<FD3D12ResourceNvFlow> ResourceRef = new FD3D12ResourceNvFlow(
		GetParentDevice(), GetParentDevice()->GetNodeMask(), descD3D12->resource,
		*descD3D12->currentState, descD3D12->resource->GetDesc(), descD3D12->currentState);

	auto localSrvDesc = descD3D12->srvDesc;
	return new FD3D12ShaderResourceViewNvFlow(GetParentDevice(), &localSrvDesc, ResourceRef);
}

class FD3D12UnorderedAccessViewNvFlow : public FD3D12UnorderedAccessView
{
	TRefCountPtr<FD3D12ResourceNvFlow> ResourceRef;

public:
	FD3D12UnorderedAccessViewNvFlow(FD3D12Device* InParent, D3D12_UNORDERED_ACCESS_VIEW_DESC* InUAVDesc,
		const TRefCountPtr<FD3D12ResourceNvFlow>& InResourceRef)
		: FD3D12UnorderedAccessView(InParent, InUAVDesc, &InResourceRef->ResourceLocation)
		, ResourceRef(InResourceRef)
	{
	}
};

FUnorderedAccessViewRHIRef FD3D12CommandContext::NvFlowCreateUAV(const FRHINvFlowResourceRWViewDesc* desc, FShaderResourceViewRHIRef* pRHIRefSRV)
{
	const FRHINvFlowResourceRWViewDescD3D12* descD3D12 = static_cast<const FRHINvFlowResourceRWViewDescD3D12*>(desc);

	TRefCountPtr<FD3D12ResourceNvFlow> ResourceRef = new FD3D12ResourceNvFlow(
		GetParentDevice(), GetParentDevice()->GetNodeMask(), descD3D12->resourceView.resource,
		*descD3D12->resourceView.currentState, descD3D12->resourceView.resource->GetDesc(), descD3D12->resourceView.currentState);

	if (pRHIRefSRV)
	{
		auto localSrvDesc = descD3D12->resourceView.srvDesc;
		*pRHIRefSRV = new FD3D12ShaderResourceViewNvFlow(GetParentDevice(), &localSrvDesc, ResourceRef);
	}

	auto localUavDesc = descD3D12->uavDesc;
	return new FD3D12UnorderedAccessViewNvFlow(GetParentDevice(), &localUavDesc, ResourceRef);
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