#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include "renderer/device.h"

using Microsoft::WRL::ComPtr;

static HWND g_hwnd = nullptr;
static ComPtr<ID3D11Device> g_device;
static ComPtr<ID3D11DeviceContext> g_context;
static ComPtr<IDXGISwapChain1> g_swapChain; 
static ComPtr<ID3D11RenderTargetView> g_renderTargetView;
static ComPtr<IDXGIFactory6> g_factory;
static ComPtr<IDXGIOutput> g_baseOutput;

bool ashenvale::renderer::initialize(HWND hwnd)
{
    g_hwnd = hwnd;

    HRESULT result;
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIOutput6> adapterOutput;

    result = CreateDXGIFactory2(0, IID_PPV_ARGS(&g_factory));
    if (FAILED(result))
        return false;

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;

    result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, &featureLevel, 1,
        D3D11_SDK_VERSION, &g_device, nullptr, &g_context);

    result = g_factory->EnumAdapterByGpuPreference(
        0,
        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
        IID_PPV_ARGS(&adapter)
    );
    if (FAILED(result))
        return false;

    result = adapter->EnumOutputs(0, &g_baseOutput);
    if (FAILED(result))
        return false;

    g_baseOutput.As(&adapterOutput);

    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

    UINT numModes = 0;
    result = adapterOutput->GetDisplayModeList1(
        format,
        0,
        &numModes,
        nullptr
    );
    if (FAILED(result))
        return false;

    DXGI_MODE_DESC1* displayModeList = new DXGI_MODE_DESC1[numModes];
    result = adapterOutput->GetDisplayModeList1(
        format,
        0,
        &numModes,
        displayModeList
    );
    if (FAILED(result))
        return false;

    UINT numerator = 0, denominator = 0;
    for (UINT i = 0; i < numModes; ++i) {
        if (displayModeList[i].Width == 1280 && displayModeList[i].Height == 720) {
            numerator = displayModeList[i].RefreshRate.Numerator;
            denominator = displayModeList[i].RefreshRate.Denominator;
            break;
        }
    }

    delete[] displayModeList;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Width = 1280;
    swapChainDesc.Height = 720;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = 0;

    ComPtr<IDXGIAdapter> dxgiAdapter;
    g_device.As(&dxgiAdapter);
    ComPtr<IDXGISwapChain1> swapChain1;
    result = g_factory->CreateSwapChainForHwnd(
        g_device.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1
    );
    if (FAILED(result))
        return false;


    g_swapChain = swapChain1;

    ComPtr<ID3D11Texture2D> backBuffer;
    result = g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(result)) {
        return false;
    }

    result = g_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &g_renderTargetView);
    if (FAILED(result)) {
        return false;
    }

    g_context->OMSetRenderTargets(1, g_renderTargetView.GetAddressOf(), nullptr);

    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(swapChainDesc.Width);
    viewport.Height = static_cast<float>(swapChainDesc.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;

    g_context->RSSetViewports(1, &viewport);

    return true;
}

void ashenvale::renderer::render()
{
    const float color[4] = { 0.3f, 0.8f, 0.7f, 1.0f };
    g_context->ClearRenderTargetView(g_renderTargetView.Get(), color);

    g_swapChain->Present(1, 0);
}

void ashenvale::renderer::shutdown()
{
    g_renderTargetView.Reset();
    g_context.Reset();
    g_swapChain.Reset();
    g_device.Reset();
    g_factory.Reset();
    g_baseOutput.Reset();
}
