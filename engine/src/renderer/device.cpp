#include <d3d11.h>

#include "renderer/device.h"

static HWND g_hwnd;
static ID3D11Device* g_device;
static ID3D11DeviceContext* g_context;
static IDXGISwapChain* g_swapChain;
static ID3D11RenderTargetView* g_renderTargetView;

bool ashenvale::renderer::initialize(HWND hwnd)
{
    g_hwnd = hwnd;

    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
    {
        return false;
    }

    DXGI_SWAP_CHAIN_DESC scDesc = {};
    scDesc.BufferCount = 1;
    scDesc.BufferDesc.Width = 1280;
    scDesc.BufferDesc.Height = 720;
    scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferDesc.RefreshRate.Numerator = 60;
    scDesc.BufferDesc.RefreshRate.Denominator = 1;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2;
    scDesc.OutputWindow = hwnd;
    scDesc.SampleDesc.Count = 1;
    scDesc.Windowed = TRUE;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0,
        D3D11_SDK_VERSION, &scDesc, &g_swapChain, &g_device, nullptr, &g_context)))
    {
        return false;
    }

    ID3D11Texture2D* backBuffer = nullptr;
    if (FAILED(g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))))
    {
        return false;
    }

    if (FAILED(g_device->CreateRenderTargetView(backBuffer, nullptr, &g_renderTargetView)))
    {
        backBuffer->Release();
        return false;
    }
    backBuffer->Release();

    g_context->OMSetRenderTargets(1, &g_renderTargetView, nullptr);

    D3D11_VIEWPORT viewport = {};
    viewport.Width = 1280;
    viewport.Height = 720;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    g_context->RSSetViewports(1, &viewport);


	return true;
}

void ashenvale::renderer::render()
{
    const float color[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    g_context->ClearRenderTargetView(g_renderTargetView, color);

    g_swapChain->Present(1, 0);
}

void ashenvale::renderer::shutdown()
{
    if (g_renderTargetView) g_renderTargetView->Release();
    if (g_context) g_context->Release();
    if (g_swapChain) g_swapChain->Release();
    if (g_device) g_device->Release();
}
