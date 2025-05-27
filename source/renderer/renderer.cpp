#include <wrl/client.h>
#include <d3d11_4.h>
#include <DirectXMath.h>

#include "renderer/renderer.h"
#include "profiler/profiler.h"
#include "renderer/device.h"
#include "renderer/shader_compiler.h"
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

void ashenvale::renderer::initialize()
{
    PIX_SCOPED_EVENT("renderer.initialize")
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

    renderer::device::g_device->CreateBuffer(&vertexBufferDesc, &vertexData, &g_vertexBuffer);

    uint16_t indices[] = { 0, 1, 2 };

    D3D11_BUFFER_DESC indexBufferDesc = {};
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(indices);
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices;

    renderer::device::g_device->CreateBuffer(&indexBufferDesc, &indexData, &g_indexBuffer);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    renderer::device::g_context->IASetVertexBuffers(0, 1, g_vertexBuffer.GetAddressOf(), &stride, &offset);

    renderer::device::g_context->IASetIndexBuffer(g_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    renderer::device::g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ComPtr<ID3DBlob> errorBlob;
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;

    renderer::shader_compiler::compile(L"vs.hlsl", "main", "vs_5_0", nullptr, vsBlob.GetAddressOf(), errorBlob.GetAddressOf());

    renderer::device::g_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_vertexShader);

    renderer::device::g_context->VSSetShader(g_vertexShader.Get(), nullptr, 0);

    renderer::shader_compiler::compile(L"ps.hlsl", "main", "ps_5_0", nullptr, psBlob.GetAddressOf(), errorBlob.GetAddressOf());

    renderer::device::g_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_pixelShader);

    renderer::device::g_context->PSSetShader(g_pixelShader.Get(), nullptr, 0);

    ComPtr<ID3D11ShaderReflection> reflection;
    D3DReflect(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), IID_PPV_ARGS(&reflection));

    auto layouts = renderer::shader_compiler::get_input_layout(reflection.Get());

    renderer::device::g_device->CreateInputLayout(layouts.data(), layouts.size(), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), g_inputLayout.GetAddressOf());
    renderer::device::g_context->IASetInputLayout(g_inputLayout.Get());
}

void ashenvale::renderer::resize_viewport(int width, int height)
{
    PIX_SCOPED_EVENT("renderer.resize_viewport")
    g_viewportTexture.Reset();
    g_viewportRTV.Reset();
    g_viewportSRV.Reset();
    g_viewportDepthStencil.Reset();
    g_viewportDSV.Reset();
    g_viewportDepthStencilState.Reset();

    D3D11_TEXTURE2D_DESC colorDesc = {};
    colorDesc.Width = width;
    colorDesc.Height = height;
    colorDesc.MipLevels = 1;
    colorDesc.ArraySize = 1;
    colorDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    colorDesc.SampleDesc.Count = 1;
    colorDesc.Usage = D3D11_USAGE_DEFAULT;
    colorDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    renderer::device::g_device->CreateTexture2D(&colorDesc, nullptr, &g_viewportTexture);
    renderer::device::g_device->CreateRenderTargetView(g_viewportTexture.Get(), nullptr, &g_viewportRTV);
    renderer::device::g_device->CreateShaderResourceView(g_viewportTexture.Get(), nullptr, &g_viewportSRV);

    D3D11_TEXTURE2D_DESC depthDesc = colorDesc;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    renderer::device::g_device->CreateTexture2D(&depthDesc, nullptr, &g_viewportDepthStencil);
    renderer::device::g_device->CreateDepthStencilView(g_viewportDepthStencil.Get(), nullptr, &g_viewportDSV);

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    renderer::device::g_device->CreateDepthStencilState(&dsDesc, &g_viewportDepthStencilState);

    g_viewportViewport = {};
    g_viewportViewport.Width = static_cast<float>(width);
    g_viewportViewport.Height = static_cast<float>(height);
    g_viewportViewport.MinDepth = 0.0f;
    g_viewportViewport.MaxDepth = 1.0f;
    g_viewportViewport.TopLeftX = 0.0f;
    g_viewportViewport.TopLeftY = 0.0f;
}

void ashenvale::renderer::render()
{
    PIX_SCOPED_EVENT("renderer.render")
    const float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    renderer::device::g_context->OMSetRenderTargets(1, g_viewportRTV.GetAddressOf(), g_viewportDSV.Get());
    renderer::device::g_context->OMSetDepthStencilState(g_viewportDepthStencilState.Get(), 1);

    renderer::device::g_context->ClearRenderTargetView(g_viewportRTV.Get(), color);
    renderer::device::g_context->ClearDepthStencilView(g_viewportDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    renderer::device::g_context->RSSetViewports(1, &g_viewportViewport);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    renderer::device::g_context->IASetVertexBuffers(0, 1, g_vertexBuffer.GetAddressOf(), &stride, &offset);
    renderer::device::g_context->IASetIndexBuffer(g_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    renderer::device::g_context->IASetInputLayout(g_inputLayout.Get());
    renderer::device::g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    renderer::device::g_context->VSSetShader(g_vertexShader.Get(), nullptr, 0);
    renderer::device::g_context->PSSetShader(g_pixelShader.Get(), nullptr, 0);
    renderer::device::g_context->DrawIndexed(3, 0, 0);

    renderer::device::g_context->OMSetRenderTargets(1, ashenvale::renderer::swapchain::g_baseRTV.GetAddressOf(), nullptr);
    renderer::device::g_context->ClearRenderTargetView(ashenvale::renderer::swapchain::g_renderTargetView.Get(), color);

    renderer::device::g_context->RSSetViewports(1, &swapchain::g_viewport);
    ashenvale::editor::render();

    ashenvale::renderer::swapchain::g_swapChain->Present(1, 0);
}
