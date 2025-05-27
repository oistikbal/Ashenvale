#include <d3d11_4.h>
#include <dxgi1_6.h>

#include "window/window.h"
#include "renderer/device.h"
#include "renderer/renderer.h"
#include "renderer/shader_compiler.h"
#include "profiler/profiler.h"
#include "editor/editor.h"
#include "renderer/swapchain.h"


using Microsoft::WRL::ComPtr;

bool ashenvale::renderer::device::initialize()
{
    PIX_SCOPED_EVENT("device.initialize");
    HRESULT result;
    ComPtr<ID3D11Device> tempDevice;
    ComPtr<ID3D11DeviceContext> tempContext;

    result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&g_factory));

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;

    result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, &featureLevel, 1,
        D3D11_SDK_VERSION, &tempDevice, nullptr, &tempContext);

    result = tempDevice.As(&g_device);

    result = tempContext.As(&g_context);

    ComPtr<IDXGIAdapter1> adapter;
    result = g_factory->EnumAdapterByGpuPreference(
        0,
        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
        IID_PPV_ARGS(&adapter)
    );

    ComPtr<IDXGIOutput> adapterOutput;
    result = adapter->EnumOutputs(0, &adapterOutput);

    result = adapterOutput.As(&g_baseOutput);

    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

    UINT numModes = 0;
    result = g_baseOutput->GetDisplayModeList1(
        format,
        0,
        &numModes,
        nullptr
    );

    DXGI_MODE_DESC1 displayModeList[512] = {};
    result = g_baseOutput->GetDisplayModeList1(
        format,
        0,
        &numModes,
        displayModeList
    );


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

    RECT clientRect;
    GetClientRect(window::g_hwnd, &clientRect);

    int renderWidth = clientRect.right - clientRect.left;
    int renderHeight = clientRect.bottom - clientRect.top;

    swapchain::create(renderWidth, renderHeight);
    renderer::initialize();

    resize_viewport(renderWidth, renderHeight);

    return true;
}