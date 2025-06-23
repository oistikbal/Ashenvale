#include <d3d11_4.h>
#include <dxgi1_6.h>

#include "editor/editor.h"
#include "profiler/profiler.h"
#include "renderer/device.h"
#include "renderer/renderer.h"
#include "renderer/swapchain.h"
#include "window/window.h"
#include "shader.h"
#include "scene/scene.h"

using namespace winrt;

bool ashenvale::renderer::device::initialize()
{
    PIX_SCOPED_EVENT("device.initialize");
    HRESULT result;
    com_ptr<ID3D11Device> tempDevice;
    com_ptr<ID3D11DeviceContext> tempContext;

    CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&g_factory));

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;

    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, &featureLevel, 1,
                      D3D11_SDK_VERSION, tempDevice.put(), nullptr, tempContext.put());

    tempDevice.as(g_device);

    tempContext.as(g_context);

    g_factory->MakeWindowAssociation(window::g_hwnd, DXGI_MWA_NO_ALT_ENTER);

    com_ptr<IDXGIAdapter1> adapter;
    g_factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));

    com_ptr<IDXGIOutput> adapterOutput;
    adapter->EnumOutputs(0, adapterOutput.put());

    adapterOutput.as(g_baseOutput);

    DXGI_MODE_DESC1 targetMode = {};
    targetMode.Format = DXGI_FORMAT_R10G10B10A2_UNORM;

    DXGI_MODE_DESC1 closeMatch = {};

    g_baseOutput->FindClosestMatchingMode1(&targetMode, &closeMatch, g_device.get());

    RECT clientRect;
    GetClientRect(window::g_hwnd, &clientRect);

    int renderWidth = clientRect.right - clientRect.left;
    int renderHeight = clientRect.bottom - clientRect.top;

    swapchain::initialize(renderWidth, renderHeight, closeMatch.Format);
    shader::initialize();
    renderer::initialize();
    scene::initialize();

    resize_viewport(renderWidth, renderHeight);

    return true;
}