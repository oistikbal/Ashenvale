#include "renderer/swapchain.h"
#include "profiler/profiler.h"
#include "renderer.h"
#include "renderer/device.h"
#include "window/window.h"

using namespace winrt;

void ashenvale::renderer::swapchain::create(int width, int height)
{
    PIX_SCOPED_EVENT("swapchain.create")
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

    com_ptr<IDXGISwapChain1> tempSwapchain;
    ashenvale::renderer::device::g_factory->CreateSwapChainForHwnd(ashenvale::renderer::device::g_device.get(),
                                                                   ashenvale::window::g_hwnd, &swapChainDesc, nullptr,
                                                                   nullptr, tempSwapchain.put());

    tempSwapchain.as(g_baseSwapchain);
    tempSwapchain.as(g_swapChain);

    g_viewport = {};
    g_viewport.Width = static_cast<float>(width);
    g_viewport.Height = static_cast<float>(height);
    g_viewport.MinDepth = 0.0f;
    g_viewport.MaxDepth = 1.0f;
    g_viewport.TopLeftX = 0.0f;
    g_viewport.TopLeftY = 0.0f;

    com_ptr<ID3D11Texture2D> backBuffer;
    g_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.put()));

    ashenvale::renderer::device::g_device->CreateRenderTargetView1(backBuffer.get(), nullptr, g_renderTargetView.put());
    g_renderTargetView.as(g_baseRTV);
}

void ashenvale::renderer::swapchain::resize(int width, int height)
{
    PIX_SCOPED_EVENT("swapchain.resize")
    if (g_swapChain == nullptr)
        return;

    g_renderTargetView = nullptr;
    g_baseRTV = nullptr;

    g_swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

    com_ptr<ID3D11Texture2D> backBuffer;
    g_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.put()));

    ashenvale::renderer::device::g_device->CreateRenderTargetView1(backBuffer.get(), nullptr, g_renderTargetView.put());
    g_renderTargetView.as(g_baseRTV);

    g_viewport = {};
    g_viewport.Width = static_cast<float>(width);
    g_viewport.Height = static_cast<float>(height);
    g_viewport.MinDepth = 0.0f;
    g_viewport.MaxDepth = 1.0f;
    g_viewport.TopLeftX = 0.0f;
    g_viewport.TopLeftY = 0.0f;

    renderer::resize_viewport(width, height);
}