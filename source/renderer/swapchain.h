#pragma once

#include <wrl/client.h>
#include <d3d11_4.h>
#include <dxgi1_6.h>

namespace ashenvale::renderer::swapchain
{
	inline Microsoft::WRL::ComPtr<ID3D11RenderTargetView1> g_renderTargetView = nullptr;
	inline Microsoft::WRL::ComPtr<ID3D11RenderTargetView> g_baseRTV = nullptr;

	inline Microsoft::WRL::ComPtr<IDXGISwapChain4> g_swapChain = nullptr;
	inline Microsoft::WRL::ComPtr<IDXGISwapChain> g_baseSwapchain = nullptr;

	inline D3D11_VIEWPORT g_viewport;
}

namespace ashenvale::renderer::swapchain
{
	void create(int width, int height);
	void resize(int width, int height);
}