#pragma once

#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <winrt/base.h>

namespace ashenvale::renderer::swapchain
{
inline winrt::com_ptr<ID3D11RenderTargetView1> g_renderTargetView = nullptr;
inline winrt::com_ptr<ID3D11RenderTargetView> g_baseRTV = nullptr;

inline winrt::com_ptr<IDXGISwapChain4> g_swapChain = nullptr;
inline winrt::com_ptr<IDXGISwapChain> g_baseSwapchain = nullptr;

inline D3D11_VIEWPORT g_viewport;
} // namespace ashenvale::renderer::swapchain

namespace ashenvale::renderer::swapchain
{
void initialize(int width, int height, DXGI_FORMAT format);
void resize(int width, int height);
} // namespace ashenvale::renderer::swapchain