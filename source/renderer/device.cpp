#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <DirectXMath.h>

#include "window/window.h"
#include "renderer/device.h"
#include "renderer/shader_compiler.h"
#include "profiler/profiler.h"
#include "editor/editor.h"



using Microsoft::WRL::ComPtr;

static ComPtr<ID3D11Buffer> g_vertexBuffer;
static ComPtr<ID3D11Buffer> g_indexBuffer;
static ComPtr<ID3D11VertexShader> g_vertexShader;
static ComPtr<ID3D11PixelShader> g_pixelShader;
static ComPtr<ID3D11InputLayout> g_inputLayout;
static ComPtr<ID3D11Texture2D> g_depthStencilBuffer;
static ComPtr<ID3D11DepthStencilView> g_depthStencilView;
static ComPtr<ID3D11RasterizerState> g_rasterState;
static ComPtr<ID3D11DepthStencilState> g_depthState;

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

    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = 1280;
    depthDesc.Height = 720;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;


    g_device->CreateTexture2D(&depthDesc, nullptr, g_depthStencilBuffer.GetAddressOf());
    if (FAILED(result)) {
        return false;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = depthDesc.Format;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    result = g_device->CreateDepthStencilView(g_depthStencilBuffer.Get(), &dsvDesc, g_depthStencilView.GetAddressOf());
    if (FAILED(result)) {
        return false;
    }

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_BACK;
    rasterDesc.FrontCounterClockwise = FALSE;

    g_device->CreateRasterizerState(&rasterDesc, g_rasterState.GetAddressOf());
    g_context->RSSetState(g_rasterState.Get());

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

    g_device->CreateDepthStencilState(&dsDesc, g_depthState.GetAddressOf());
    g_context->OMSetDepthStencilState(g_depthState.Get(), 1);

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

    PIXEndEvent();
    return true;
}

void ashenvale::renderer::device::render()
{
    PIXBeginEvent(0, "renderer.render");
    static ComPtr<ID3D11RenderTargetView> rtv;
    renderer::device::g_renderTargetView.As(&rtv);

    g_context->OMSetRenderTargets(1, rtv.GetAddressOf(), g_depthStencilView.Get());
    g_context->ClearDepthStencilView(g_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


    const float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    g_context->ClearRenderTargetView(g_renderTargetView.Get(), color);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    g_context->IASetVertexBuffers(0, 1, g_vertexBuffer.GetAddressOf(), &stride, &offset);
    g_context->IASetIndexBuffer(g_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    g_context->IASetInputLayout(g_inputLayout.Get());

    g_context->VSSetShader(g_vertexShader.Get(), nullptr, 0);
    g_context->PSSetShader(g_pixelShader.Get(), nullptr, 0);
    g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_context->DrawIndexed(3, 0, 0);

    ashenvale::editor::render();

    g_swapChain->Present(1, 0);

    PIXEndEvent();
}

void ashenvale::renderer::device::shutdown()
{
    PIXBeginEvent(0, "renderer.shutdown");

    g_renderTargetView.Reset();
    g_context.Reset();
    g_swapChain.Reset();
    g_device.Reset();
    g_factory.Reset();
    g_baseOutput.Reset();
    g_depthStencilView.Reset();
    g_depthStencilBuffer.Reset();
    g_vertexBuffer.Reset();
    g_pixelShader.Reset();

    PIXEndEvent();
}
