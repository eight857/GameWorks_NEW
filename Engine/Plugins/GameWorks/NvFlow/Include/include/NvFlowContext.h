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

//! NvFlowContext: A framework for fluid simulation
struct NvFlowContext;

//! import interop buffers
struct NvFlowDepthStencilView;
struct NvFlowRenderTargetView;

//! graphics API dependent descriptions
struct NvFlowContextDesc;
struct NvFlowDepthStencilViewDesc;
struct NvFlowRenderTargetViewDesc;

//! export interop buffers
struct NvFlowBuffer;
struct NvFlowTexture3D;

//! graphics API dependent descriptions
struct NvFlowBufferViewDesc;
struct NvFlowTexture3DViewDesc;