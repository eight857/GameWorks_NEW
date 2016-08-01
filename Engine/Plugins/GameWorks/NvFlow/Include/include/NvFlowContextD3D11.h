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

struct NvFlowDepthStencilViewDesc
{
	ID3D11DepthStencilView* dsv;
	ID3D11ShaderResourceView* srv;
	D3D11_VIEWPORT viewport;
};

struct NvFlowRenderTargetViewDesc
{
	ID3D11RenderTargetView* rtv;
	D3D11_VIEWPORT viewport;
};

struct NvFlowContextDesc
{
	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;
};

struct NvFlowBufferViewDesc
{
	ID3D11ShaderResourceView* srv;
	ID3D11UnorderedAccessView* uav;
};

struct NvFlowTexture3DViewDesc
{
	ID3D11ShaderResourceView* srv;
	ID3D11UnorderedAccessView* uav;
};