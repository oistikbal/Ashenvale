#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include "window/window.h"
#include "renderer/device.h"

using Microsoft::WRL::ComPtr;

bool ashenvale::renderer::initialize()
{
    HRESULT result;
    ComPtr<ID3D11Device> tempDevice;
    ComPtr<ID3D11DeviceContext> tempContext;

    result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&g_factory));
    if (FAILED(result))
        return false;

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;

    result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, &featureLevel, 1,
        D3D11_SDK_VERSION, &tempDevice, nullptr, &tempContext);

    result = tempDevice.As(&g_device);
    if (FAILED(result)) {
        return false;
    }
    result = tempContext.As(&g_context);
    if (FAILED(result)) {
        return false;
    }

    ComPtr<IDXGIAdapter1> adapter;
    result = g_factory->EnumAdapterByGpuPreference(
        0,
        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
        IID_PPV_ARGS(&adapter)
    );
    if (FAILED(result))
        return false;

    ComPtr<IDXGIOutput> adapterOutput;
    result = adapter->EnumOutputs(0, &adapterOutput);
    if (FAILED(result))
        return false;

    result = adapterOutput.As(&g_baseOutput);

    if (FAILED(result)) {
        return false;
    }

    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

    UINT numModes = 0;
    result = g_baseOutput->GetDisplayModeList1(
        format,
        0,
        &numModes,
        nullptr
    );
    if (FAILED(result))
        return false;

    DXGI_MODE_DESC1* displayModeList = new DXGI_MODE_DESC1[numModes];
    result = g_baseOutput->GetDisplayModeList1(
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
    if (numerator == 0 || denominator == 0) {
        numerator = 60;
        denominator = 1;
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


    ComPtr<IDXGISwapChain1> tempSwapChain;
    result = g_factory->CreateSwapChainForHwnd(
        g_device.Get(), ashenvale::window::g_hwnd, &swapChainDesc, nullptr, nullptr, &tempSwapChain
    );
    if (FAILED(result)) {
        return false;
    }

    result = tempSwapChain.As(&g_swapChain);
    if (FAILED(result)) {
        return false;
    }

    ComPtr<ID3D11Texture2D> backBuffer;
    result = g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(result)) {
        return false;
    }

    result = g_device->CreateRenderTargetView1(backBuffer.Get(), nullptr, &g_renderTargetView);
    if (FAILED(result)) {
        return false;
    }


    ComPtr<ID3D11RenderTargetView> baseRTV;
    g_renderTargetView.As(&baseRTV);
    g_context->OMSetRenderTargets(1, baseRTV.GetAddressOf(), nullptr);

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
