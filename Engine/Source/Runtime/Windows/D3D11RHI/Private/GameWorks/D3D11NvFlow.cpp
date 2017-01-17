// NvFlow begin

/*=============================================================================
D3D11NvFlow.cpp: D3D device RHI NvFlow interop implementation.
=============================================================================*/

#include "D3D11RHIPrivate.h"

#include "GameWorks/RHINvFlowD3D11.h"

inline D3D11_VIEWPORT NvFlowGetViewport(ID3D11DeviceContext* context)
{
	unsigned int numViewPorts = 1u;
	D3D11_VIEWPORT viewport;
	context->RSGetViewports(&numViewPorts, &viewport);
	return viewport;
}

void FD3D11DynamicRHI::NvFlowGetDeviceDesc(FRHINvFlowDeviceDesc* desc)
{
	desc->device = Direct3DDevice;
	desc->deviceContext = Direct3DDeviceIMContext;
}

void FD3D11DynamicRHI::NvFlowGetDepthStencilViewDesc(FRHINvFlowDepthStencilViewDesc* desc)
{
	desc->dsv = CurrentDepthStencilTarget;
	desc->srv = CurrentDepthTexture->GetShaderResourceView();
	desc->viewport = NvFlowGetViewport(Direct3DDeviceIMContext);
}

void FD3D11DynamicRHI::NvFlowGetRenderTargetViewDesc(FRHINvFlowRenderTargetViewDesc* desc)
{
	desc->rtv = CurrentRenderTargets[0];
	desc->viewport = NvFlowGetViewport(Direct3DDeviceIMContext);
}

namespace
{
	class FEmptyResource : public FRHIResource, public FD3D11BaseShaderResource
	{
	public:
		FEmptyResource()
		{
		}

		// IRefCountedObject interface.
		virtual uint32 AddRef() const
		{
			return FRHIResource::AddRef();
		}
		virtual uint32 Release() const
		{
			return FRHIResource::Release();
		}
		virtual uint32 GetRefCount() const
		{
			return FRHIResource::GetRefCount();
		}
	};
}

FShaderResourceViewRHIRef FD3D11DynamicRHI::NvFlowCreateSRV(const FRHINvFlowResourceViewDesc* desc)
{
	TRefCountPtr<ID3D11ShaderResourceView> View = desc->srv;
	TRefCountPtr<FD3D11BaseShaderResource> Resource = new FEmptyResource();

	return new FD3D11ShaderResourceView(View, Resource);
}

FUnorderedAccessViewRHIRef FD3D11DynamicRHI::NvFlowCreateUAV(const FRHINvFlowResourceRWViewDesc* desc)
{
	TRefCountPtr<ID3D11UnorderedAccessView> View = desc->uav;
	TRefCountPtr<FD3D11BaseShaderResource> Resource = new FEmptyResource();
	Resource->SetCurrentGPUAccess(EResourceTransitionAccess::EWritable);

	return new FD3D11UnorderedAccessView(View, Resource);
}

// NvFlow end