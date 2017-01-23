#pragma once

// NvFlow begin
struct FRHINvFlowDeviceDescD3D12 : FRHINvFlowDeviceDesc
{
	ID3D12Device* device;						//!< The desired d3d12 device to use
	ID3D12CommandQueue* commandQueue;			//!< The commandQueue commandList will be submit on
	ID3D12Fence* commandQueueFence;				//!< Fence marking events on this queue
	ID3D12GraphicsCommandList* commandList;		//!< The commandlist for recording
	UINT64 lastFenceCompleted;					//!< The last fence completed on commandQueue
	UINT64 nextFenceValue;						//!< The fence value signaled after commandList is submitted
};

struct FRHINvFlowDepthStencilViewDescD3D12 : FRHINvFlowDepthStencilViewDesc
{
	D3D12_CPU_DESCRIPTOR_HANDLE dsv;
	D3D12_CPU_DESCRIPTOR_HANDLE srv;
	DXGI_FORMAT dsv_format;
	DXGI_FORMAT srv_format;
	ID3D12Resource* resource;
	D3D12_RESOURCE_STATES currentState;
	D3D12_VIEWPORT viewport;
	UINT32 width;
	UINT32 height;
};

struct FRHINvFlowRenderTargetViewDescD3D12 : FRHINvFlowRenderTargetViewDesc
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtv;
	DXGI_FORMAT rtv_format;
	ID3D12Resource* resource;
	D3D12_RESOURCE_STATES currentState;
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissor;
};

struct FRHINvFlowResourceViewDescD3D12 : FRHINvFlowResourceViewDesc
{
	D3D12_CPU_DESCRIPTOR_HANDLE srv;
};

struct FRHINvFlowResourceRWViewDescD3D12 : FRHINvFlowResourceRWViewDesc
{
	D3D12_CPU_DESCRIPTOR_HANDLE uav;
};
// NvFlow end