#pragma once

// NvFlow begin
struct FRHINvFlowDeviceDesc
{
	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;
};

struct FRHINvFlowDepthStencilViewDesc
{
	ID3D11DepthStencilView* dsv;
	ID3D11ShaderResourceView* srv;
	D3D11_VIEWPORT viewport;
};

struct FRHINvFlowRenderTargetViewDesc
{
	ID3D11RenderTargetView* rtv;
	D3D11_VIEWPORT viewport;
};
// NvFlow end