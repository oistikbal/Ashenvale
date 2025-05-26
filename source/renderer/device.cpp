#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <DirectXMath.h>

#include "window/window.h"
#include "renderer/device.h"
#include "renderer/shader_compiler.h"
#include "profiler/profiler.h"
#include "editor/editor.h"
#include "renderer/swapchain.h"


using Microsoft::WRL::ComPtr;

static ComPtr<ID3D11Buffer> g_vertexBuffer;
static ComPtr<ID3D11Buffer> g_indexBuffer;
static ComPtr<ID3D11VertexShader> g_vertexShader;
static ComPtr<ID3D11PixelShader> g_pixelShader;
static ComPtr<ID3D11InputLayout> g_inputLayout;
static D3D11_VIEWPORT g_viewportViewport;

struct Vertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

bool ashenvale::renderer::device::initialize()
{
    PIXBeginEvent(0, "renderer.initialize");
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

    DXGI_MODE_DESC1 displayModeList[512] = {};
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

    RECT clientRect;
    GetClientRect(window::g_hwnd, &clientRect);

    int renderWidth = clientRect.right - clientRect.left;
    int renderHeight = clientRect.bottom - clientRect.top;

    swapchain::create(renderWidth, renderHeight);

    Vertex triangleVertices[] = {
        { {  0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },  // Top (Red)
        { {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },  // Right (Green)
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }   // Left (Blue)
    };

    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(triangleVertices);
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = triangleVertices;

    result = g_device->CreateBuffer(&vertexBufferDesc, &vertexData, &g_vertexBuffer);
    if (FAILED(result)) {
        return false;
    }

    uint16_t indices[] = { 0, 1, 2 };

    D3D11_BUFFER_DESC indexBufferDesc = {};
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(indices);
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices;

    result = g_device->CreateBuffer(&indexBufferDesc, &indexData, &g_indexBuffer);
    if (FAILED(result)) {
        return false;
    }

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_context->IASetVertexBuffers(0, 1, g_vertexBuffer.GetAddressOf(), &stride, &offset);

    g_context->IASetIndexBuffer(g_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ComPtr<ID3DBlob> errorBlob;
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;

    result = renderer::shader_compiler::compile(L"vs.hlsl", "main", "vs_5_0", nullptr, vsBlob.GetAddressOf(), errorBlob.GetAddressOf());

    if (FAILED(result)) {
        return false;
    }

    g_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_vertexShader);

    g_context->VSSetShader(g_vertexShader.Get(), nullptr, 0);

    result = renderer::shader_compiler::compile(L"ps.hlsl", "main", "ps_5_0", nullptr, psBlob.GetAddressOf(), errorBlob.GetAddressOf());

    if (FAILED(result)) {
        return false;
    }

    g_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_pixelShader);

    g_context->PSSetShader(g_pixelShader.Get(), nullptr, 0);

    ComPtr<ID3D11ShaderReflection> reflection;
    D3DReflect(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), IID_PPV_ARGS(&reflection));

    auto layouts = renderer::shader_compiler::get_input_layout(reflection.Get());

    g_device->CreateInputLayout(layouts.data(), layouts.size(), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), g_inputLayout.GetAddressOf());
    g_context->IASetInputLayout(g_inputLayout.Get());

    resize_viewport(renderWidth, renderHeight);

    PIXEndEvent();
    return true;
}

void ashenvale::renderer::device::render()
{
    PIXBeginEvent(0, "renderer.render");
    const float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    g_context->OMSetRenderTargets(1, g_viewportRTV.GetAddressOf(), g_viewportDSV.Get());
    g_context->OMSetDepthStencilState(g_viewportState.Get(), 1);

    g_context->ClearRenderTargetView(g_viewportRTV.Get(), color);
    g_context->ClearDepthStencilView(g_viewportDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


    g_context->RSSetViewports(1, &g_viewportViewport);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    g_context->IASetVertexBuffers(0, 1, g_vertexBuffer.GetAddressOf(), &stride, &offset);
    g_context->IASetIndexBuffer(g_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    g_context->IASetInputLayout(g_inputLayout.Get());

    g_context->VSSetShader(g_vertexShader.Get(), nullptr, 0);
    g_context->PSSetShader(g_pixelShader.Get(), nullptr, 0);
    g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_context->DrawIndexed(3, 0, 0);

    g_context->OMSetRenderTargets(1, ashenvale::renderer::swapchain::g_baseRTV.GetAddressOf(), nullptr);
    g_context->ClearRenderTargetView(ashenvale::renderer::swapchain::g_renderTargetView.Get(), color);

    g_context->RSSetViewports(1, &swapchain::g_viewport);
    ashenvale::editor::render();

    ashenvale::renderer::swapchain::g_swapChain->Present(1, 0);

    PIXEndEvent();
}

void ashenvale::renderer::device::shutdown()
{
    PIXBeginEvent(0, "renderer.shutdown");

    g_context.Reset();
    g_device.Reset();
    g_factory.Reset();
    g_baseOutput.Reset();
    g_vertexBuffer.Reset();
    g_pixelShader.Reset();
    g_viewportTexture.Reset();
    g_viewportRTV.Reset();
    g_viewportSRV.Reset();
    g_viewportDepthStencil.Reset();
    g_viewportDSV.Reset();
    g_viewportState.Reset();

    PIXEndEvent();
}

void ashenvale::renderer::device::resize_viewport(int width, int height)
{
    g_viewportTexture.Reset();
    g_viewportRTV.Reset();
    g_viewportSRV.Reset();
    g_viewportDepthStencil.Reset();
    g_viewportDSV.Reset();
    g_viewportState.Reset();

    D3D11_TEXTURE2D_DESC colorDesc = {};
    colorDesc.Width = width;
    colorDesc.Height = height;
    colorDesc.MipLevels = 1;
    colorDesc.ArraySize = 1;
    colorDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    colorDesc.SampleDesc.Count = 1;
    colorDesc.Usage = D3D11_USAGE_DEFAULT;
    colorDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    g_device->CreateTexture2D(&colorDesc, nullptr, &g_viewportTexture);
    g_device->CreateRenderTargetView(g_viewportTexture.Get(), nullptr, &g_viewportRTV);
    g_device->CreateShaderResourceView(g_viewportTexture.Get(), nullptr, &g_viewportSRV);

    D3D11_TEXTURE2D_DESC depthDesc = colorDesc;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    g_device->CreateTexture2D(&depthDesc, nullptr, &g_viewportDepthStencil);
    g_device->CreateDepthStencilView(g_viewportDepthStencil.Get(), nullptr, &g_viewportDSV);

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    g_device->CreateDepthStencilState(&dsDesc, &g_viewportState);

    g_viewportViewport = {};
    g_viewportViewport.Width = static_cast<float>(width);
    g_viewportViewport.Height = static_cast<float>(height);
    g_viewportViewport.MinDepth = 0.0f;
    g_viewportViewport.MaxDepth = 1.0f;
    g_viewportViewport.TopLeftX = 0.0f;
    g_viewportViewport.TopLeftY = 0.0f;
}
