#include "renderer/swapchain.h"
#include "renderer/device.h"
#include "window/window.h"

using Microsoft::WRL::ComPtr;

void ashenvale::renderer::swapchain::create(int width, int height)
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = 0;

    ComPtr<IDXGISwapChain1> tempSwapchain;
    ashenvale::renderer::device::g_factory->CreateSwapChainForHwnd(
        ashenvale::renderer::device::g_device.Get(), ashenvale::window::g_hwnd, &swapChainDesc, nullptr, nullptr, tempSwapchain.GetAddressOf());

    tempSwapchain.As(&g_baseSwapchain);
    tempSwapchain.As(&g_swapChain);

    g_viewport = {};
    g_viewport.Width = static_cast<float>(width);
    g_viewport.Height = static_cast<float>(height);
    g_viewport.MinDepth = 0.0f;
    g_viewport.MaxDepth = 1.0f;
    g_viewport.TopLeftX = 0.0f;
    g_viewport.TopLeftY = 0.0f;

    ComPtr<ID3D11Texture2D> backBuffer;
    g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));

    ashenvale::renderer::device::g_device->CreateRenderTargetView1(backBuffer.Get(), nullptr, g_renderTargetView.GetAddressOf());
    g_renderTargetView.As(&g_baseRTV);
}

void ashenvale::renderer::swapchain::resize(int width, int height)
{
    if (g_swapChain == nullptr)
        return;

    g_renderTargetView.Reset();
    g_baseRTV.Reset();

    g_swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

    ComPtr<ID3D11Texture2D> backBuffer;
    g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));

    ashenvale::renderer::device::g_device->CreateRenderTargetView1(backBuffer.Get(), nullptr, g_renderTargetView.GetAddressOf());
    g_renderTargetView.As(&g_baseRTV);

    g_viewport = {};
    g_viewport.Width = static_cast<float>(width);
    g_viewport.Height = static_cast<float>(height);
    g_viewport.MinDepth = 0.0f;
    g_viewport.MaxDepth = 1.0f;
    g_viewport.TopLeftX = 0.0f;
    g_viewport.TopLeftY = 0.0f;
}