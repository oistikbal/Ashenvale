#pragma once

#include <Windows.h>
#include <wrl/client.h>
#include <d3d11_4.h>
#include <dxgi1_6.h>

using Microsoft::WRL::ComPtr;

namespace ashenvale::renderer::device
{
	inline ComPtr<ID3D11Device4> g_device = nullptr;
	inline ComPtr<ID3D11DeviceContext4> g_context = nullptr;
	inline ComPtr<ID3D11RenderTargetView1> g_renderTargetView = nullptr;

	inline ComPtr<IDXGISwapChain4> g_swapChain = nullptr;
	inline ComPtr<IDXGIFactory6> g_factory = nullptr;
	inline ComPtr<IDXGIOutput6> g_baseOutput = nullptr;
}

namespace ashenvale::renderer::device
{
	bool initialize();
	void render();
	void shutdown();
}