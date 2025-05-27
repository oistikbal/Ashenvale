#pragma once

#include <d3d11_4.h>
#include <wrl/client.h>


namespace ashenvale::renderer 
{
	inline Microsoft::WRL::ComPtr<ID3D11Texture2D> g_viewportTexture = nullptr;
	inline Microsoft::WRL::ComPtr<ID3D11RenderTargetView> g_viewportRTV = nullptr;
	inline Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> g_viewportSRV = nullptr;

	inline Microsoft::WRL::ComPtr<ID3D11Texture2D> g_viewportDepthStencil = nullptr;
	inline Microsoft::WRL::ComPtr<ID3D11DepthStencilView> g_viewportDSV = nullptr;
	inline Microsoft::WRL::ComPtr<ID3D11DepthStencilState> g_viewportDepthStencilState = nullptr;
}

namespace ashenvale::renderer 
{
	void initialize();
	void render();
	void resize_viewport(int width, int height);
}