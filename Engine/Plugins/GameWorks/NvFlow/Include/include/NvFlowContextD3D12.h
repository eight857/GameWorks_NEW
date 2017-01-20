/*
 * Copyright (c) 2008-2016, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#pragma once

struct NvFlowDepthStencilViewDescD3D12
{
	D3D12_CPU_DESCRIPTOR_HANDLE dsv;
	D3D12_CPU_DESCRIPTOR_HANDLE srv;
	DXGI_FORMAT dsv_format;
	DXGI_FORMAT srv_format;
	ID3D12Resource* resource;
	D3D12_RESOURCE_STATES currentState;
	D3D12_VIEWPORT viewport;
	NvFlowUint width;
	NvFlowUint height;
};

struct NvFlowRenderTargetViewDescD3D12
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtv;
	DXGI_FORMAT rtv_format;
	ID3D12Resource* resource;
	D3D12_RESOURCE_STATES currentState;
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissor;
};

struct NvFlowContextDescD3D12
{
	ID3D12Device* device;						//!< The desired d3d12 device to use
	ID3D12CommandQueue* commandQueue;			//!< The commandQueue commandList will be submit on
	ID3D12Fence* commandQueueFence;				//!< Fence marking events on this queue
	ID3D12GraphicsCommandList* commandList;		//!< The commandlist for recording
	UINT64 lastFenceCompleted;					//!< The last fence completed on commandQueue
	UINT64 nextFenceValue;						//!< The fence value signaled after commandList is submitted
};

struct NvFlowResourceViewDescD3D12
{
	D3D12_CPU_DESCRIPTOR_HANDLE srv;
	ID3D12Resource* resource;
	D3D12_RESOURCE_STATES* currentState;
	DXGI_FORMAT format;
	NvFlowDim dim;
};

struct NvFlowResourceRWViewDescD3D12
{
	NvFlowResourceViewDescD3D12 resourceView;
	D3D12_CPU_DESCRIPTOR_HANDLE uav;
};

// --------------------------- NvFlowContext -------------------------------

/**
* Creates a graphics/compute context for Flow.
*
* @param[in] version Should be set by app to NV_FLOW_VERSION.
* @param[in] desc A graphics-API dependent structure containing data needed for a FlowContext to interoperate with the app.
*
* @return The created Flow context.
*/
NV_FLOW_API NvFlowContext* NvFlowCreateContextD3D12(NvFlowUint version, const NvFlowContextDescD3D12* desc);

/**
* Creates a Flow depth stencil view based on information provided by the application.
*
* @param[in] context The Flow context to create and use the depth stencil view.
* @param[in] desc The graphics API dependent description.
*
* @return The created Flow depth stencil view.
*/
NV_FLOW_API NvFlowDepthStencilView* NvFlowCreateDepthStencilViewD3D12(NvFlowContext* context, const NvFlowDepthStencilViewDescD3D12* desc);

/**
* Creates a Flow render target view based on information provided by the application.
*
* @param[in] context The Flow context to create and use the render target view.
* @param[in] desc The graphics API dependent description.
*
* @return The created Flow render target view.
*/
NV_FLOW_API NvFlowRenderTargetView* NvFlowCreateRenderTargetViewD3D12(NvFlowContext* context, const NvFlowRenderTargetViewDescD3D12* desc);

/**
* Updates a Flow context with information provided by the application.
*
* @param[in] context The Flow context to update.
* @param[in] desc The graphics API dependent description.
*/
NV_FLOW_API void NvFlowUpdateContextD3D12(NvFlowContext* context, const NvFlowContextDescD3D12* desc);

/**
* Gets a Flow context description from a Flow context.
*
* @param[in] context The Flow context.
* @param[out] desc The graphics API dependent description.
*/
NV_FLOW_API void NvFlowUpdateContextDescD3D12(NvFlowContext* context, NvFlowContextDescD3D12* desc);

/**
* Updates a Flow depth stencil view with information provided by the application.
*
* @param[in] context The Flow context used to create the depth stencil view.
* @param[in] view The Flow depth stencil view to update.
* @param[in] desc The graphics API dependent description.
*/
NV_FLOW_API void NvFlowUpdateDepthStencilViewD3D12(NvFlowContext* context, NvFlowDepthStencilView* view, const NvFlowDepthStencilViewDescD3D12* desc);

/**
* Updates a Flow render target view with information provided by the application.
*
* @param[in] context The Flow context used to create the render target view.
* @param[in] view The Flow render target view to update.
* @param[in] desc The graphics API dependent description.
*/
NV_FLOW_API void NvFlowUpdateRenderTargetViewD3D12(NvFlowContext* context, NvFlowRenderTargetView* view, const NvFlowRenderTargetViewDescD3D12* desc);

/**
* Updates an application visible description with internal Flow resource information.
*
* @param[in] context The Flow context that created the resource.
* @param[in] resource The Flow resource to describe.
* @param[out] desc The graphics API dependent Flow resource description.
*/
NV_FLOW_API void NvFlowUpdateResourceViewDescD3D12(NvFlowContext* context, NvFlowResource* resource, NvFlowResourceViewDescD3D12* desc);

/**
* Updates an application visible description with internal Flow resourceRW information.
*
* @param[in] context The Flow context that created the resourceRW.
* @param[in] buffer The Flow resourceRW to describe.
* @param[out] desc The graphics API dependent Flow resourceRW description.
*/
NV_FLOW_API void NvFlowUpdateResourceRWViewDescD3D12(NvFlowContext* context, NvFlowResourceRW* resourceRW, NvFlowResourceRWViewDescD3D12* desc);