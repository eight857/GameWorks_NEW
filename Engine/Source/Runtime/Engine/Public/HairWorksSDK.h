// @third party code - BEGIN HairWorks
#pragma once

#include <Nv/HairWorks/NvHairSdk.h>

struct ID3D11DeviceContext;
struct ID3D11ShaderResourceView;
struct ID3D11Device;

namespace HairWorks{
	class ID3DHelper
	{
	public:
		virtual ID3D11DeviceContext* GetDeviceContext(const IRHICommandContext&) = 0;
		virtual ID3D11ShaderResourceView* GetShaderResourceView(FRHITexture2D*) = 0;
		virtual void CommitShaderResources(IRHICommandContext&) = 0;
	};

	ENGINE_API NvHair::Sdk* GetSDK();
	ENGINE_API const NvHair::ConversionSettings& GetAssetConversionSettings();
	ENGINE_API ID3DHelper& GetD3DHelper();
	ENGINE_API void Initialize(ID3D11Device& D3DDevice, ID3DHelper& D3DHelper);
	ENGINE_API void ShutDown();
}
// @third party code - END HairWorks
