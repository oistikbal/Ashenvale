#pragma once

#include <Windows.h>
#include <wrl/client.h>
#include <d3d11_4.h>
#include <dxgi1_6.h>

namespace ashenvale::renderer::device
{
	inline Microsoft::WRL::ComPtr<ID3D11Device4> g_device = nullptr;
	inline Microsoft::WRL::ComPtr<ID3D11DeviceContext4> g_context = nullptr;

	inline Microsoft::WRL::ComPtr<IDXGIFactory6> g_factory = nullptr;
	inline Microsoft::WRL::ComPtr<IDXGIOutput6> g_baseOutput = nullptr;

	inline Microsoft::WRL::ComPtr<ID3D11Texture2D> g_viewportTexture = nullptr;
	inline Microsoft::WRL::ComPtr<ID3D11RenderTargetView> g_viewportRTV = nullptr;
	inline Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> g_viewportSRV = nullptr;

	inline Microsoft::WRL::ComPtr<ID3D11Texture2D> g_viewportDepthStencil = nullptr;
	inline Microsoft::WRL::ComPtr<ID3D11DepthStencilView> g_viewportDSV = nullptr;
	inline Microsoft::WRL::ComPtr<ID3D11DepthStencilState> g_viewportState = nullptr;
}

namespace ashenvale::renderer::device
{
	bool initialize();
	void render();
	void shutdown();
	void resize_viewport(int width, int height);
}