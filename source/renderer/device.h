#pragma once

#include <Windows.h>
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <winrt/base.h>


namespace ashenvale::renderer::device
{
	inline winrt::com_ptr<ID3D11Device4> g_device = nullptr;
	inline winrt::com_ptr<ID3D11DeviceContext4> g_context = nullptr;
	inline winrt::com_ptr<ID3D11InfoQueue> g_infoQueue = nullptr;

	inline winrt::com_ptr<IDXGIFactory6> g_factory = nullptr;
	inline winrt::com_ptr<IDXGIOutput6> g_baseOutput = nullptr;
}

namespace ashenvale::renderer::device
{
	bool initialize();
}