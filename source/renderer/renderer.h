#pragma once

#include <d3d11_4.h>
#include <winrt/base.h>

namespace ashenvale::renderer 
{
	inline winrt::com_ptr<ID3D11Texture2D> g_viewportTexture = nullptr;
	inline winrt::com_ptr<ID3D11RenderTargetView> g_viewportRTV = nullptr;
	inline winrt::com_ptr<ID3D11ShaderResourceView> g_viewportSRV = nullptr;
    inline winrt::com_ptr<ID3D11UnorderedAccessView> g_viewportUAV = nullptr;

	inline winrt::com_ptr<ID3D11Texture2D> g_viewportDepthStencil = nullptr;
	inline winrt::com_ptr<ID3D11DepthStencilView> g_viewportDSV = nullptr;
	inline winrt::com_ptr<ID3D11DepthStencilState> g_viewportDepthStencilState = nullptr;
	inline winrt::com_ptr<ID3D11DepthStencilState> g_viewportDepthStencilStateDisabled = nullptr;
	inline winrt::com_ptr<ID3D11ShaderResourceView> g_viewportDepthSRV = nullptr;

	inline D3D11_VIEWPORT g_viewportViewport;
}

namespace ashenvale::renderer 
{
	void initialize();
	void render();
	void resize_viewport(int width, int height);
}