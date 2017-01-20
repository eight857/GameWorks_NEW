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

#include "NvFlowTypes.h"

#define NV_FLOW_VERSION 0

//! NvFlowContext: A framework for fluid simulation
struct NvFlowContext;

//! API type
enum NvFlowContextAPI
{
	eNvFlowContextD3D11 = 1,
	eNvFlowContextD3D12 = 2
};

//! import interop buffers
struct NvFlowDepthStencilView;
struct NvFlowRenderTargetView;

//! export interop buffers
struct NvFlowResource;
struct NvFlowResourceRW;
struct NvFlowBuffer;
struct NvFlowTexture3D;

// --------------------------- NvFlowContext -------------------------------
///@defgroup NvFlowContext
///@{

/**
* Get the API type of the current context
*
* @param[in] context The Flow context to get the type of.
*
* @return context The Flow context to be released.
*/
NV_FLOW_API NvFlowContextAPI NvFlowContextGetContextType(NvFlowContext* context);

/**
* Releases a Flow context.
*
* @param[in] context The Flow context to be released.
*/
NV_FLOW_API void NvFlowReleaseContext(NvFlowContext* context);

/**
* Releases a Flow depth stencil view.
*
* @param[in] view The Flow depth stencil view to be released.
*/
NV_FLOW_API void NvFlowReleaseDepthStencilView(NvFlowDepthStencilView* view);

/**
* Releases a Flow render target view.
*
* @param[in] view The Flow render target view to be released.
*/
NV_FLOW_API void NvFlowReleaseRenderTargetView(NvFlowRenderTargetView* view);

/**
* Pushes graphics/compute pipeline state for later restoration by NvFlowContextPop.
*
* @param[in] context The Flow context to push.
*/
NV_FLOW_API void NvFlowContextPush(NvFlowContext* context);

/**
* Restores graphics/compute pipeline state pushed by NvFlowContextPush.
*
* @param[in] context The Flow context to restore.
*/
NV_FLOW_API void NvFlowContextPop(NvFlowContext* context);

/**
* An optional callback to allow the application to control how Flow allocates CPU memory.
*
* @param[in] malloc The allocation function for Flow to use.
*/
NV_FLOW_API void NvFlowSetMallocFunc(void*(*malloc)(size_t size));

/**
* An optional callback to allow the application to control how Flow releases CPU memory.
*
* @param[in] free The free function for Flow to use.
*/
NV_FLOW_API void NvFlowSetFreeFunc(void(*free)(void* ptr));

/**
* Should be called before DLL unload, to ensure complete cleanup.
*
* @param[in] timeoutMS Wait timeout, in milliseconds
*
* @return The current number of active deferred release units.
*/
NV_FLOW_API NvFlowUint NvFlowDeferredRelease(float timeoutMS);

///@}