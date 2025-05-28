#include <d3d11_4.h>
#include <DirectXMath.h>

#include "renderer/renderer.h"
#include "profiler/profiler.h"
#include "renderer/device.h"
#include "renderer/shader_compiler.h"
#include "editor/editor.h"
#include "renderer/swapchain.h"
#include "camera.h"


using namespace winrt;

static com_ptr<ID3D11Buffer> g_vertexBuffer;
static com_ptr<ID3D11Buffer> g_indexBuffer;
static com_ptr<ID3D11VertexShader> g_vertexShader;
static com_ptr<ID3D11PixelShader> g_pixelShader;
static com_ptr<ID3D11InputLayout> g_inputLayout;
static com_ptr<ID3D11Buffer> g_cameraBuffer;
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

    renderer::device::g_device->CreateBuffer(&vertexBufferDesc, &vertexData, g_vertexBuffer.put());

    uint16_t indices[] = { 0, 1, 2 };

    D3D11_BUFFER_DESC indexBufferDesc = {};
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(indices);
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices;

    renderer::device::g_device->CreateBuffer(&indexBufferDesc, &indexData, g_indexBuffer.put());

    com_ptr<ID3DBlob> errorBlob;
    com_ptr<ID3DBlob> vsBlob;
    com_ptr<ID3DBlob> psBlob;

    renderer::shader_compiler::compile(L"vs.hlsl", "main", "vs_5_0", nullptr, vsBlob.put(), errorBlob.put());

    renderer::device::g_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, g_vertexShader.put());

    renderer::shader_compiler::compile(L"ps.hlsl", "main", "ps_5_0", nullptr, psBlob.put(), errorBlob.put());

    renderer::device::g_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, g_pixelShader.put());

    com_ptr<ID3D11ShaderReflection> reflection;
    D3DReflect(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), IID_PPV_ARGS(reflection.put()));

    auto layouts = renderer::shader_compiler::get_input_layout(reflection.get());

    renderer::device::g_device->CreateInputLayout(layouts.data(), layouts.size(), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), g_inputLayout.put());

    D3D11_BUFFER_DESC cameraBufferDesc = {};
    cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    cameraBufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4X4);
    cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    renderer::device::g_device->CreateBuffer(&cameraBufferDesc, nullptr, g_cameraBuffer.put());

    renderer::camera::initialize();
}

void ashenvale::renderer::resize_viewport(int width, int height)
{
    PIX_SCOPED_EVENT("renderer.resize_viewport")
    g_viewportTexture = nullptr;
    g_viewportRTV = nullptr;
    g_viewportSRV = nullptr;
    g_viewportDepthStencil = nullptr;
    g_viewportDSV = nullptr;
    g_viewportDepthStencilState = nullptr;

    D3D11_TEXTURE2D_DESC colorDesc = {};
    colorDesc.Width = width;
    colorDesc.Height = height;
    colorDesc.MipLevels = 1;
    colorDesc.ArraySize = 1;
    colorDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    colorDesc.SampleDesc.Count = 1;
    colorDesc.Usage = D3D11_USAGE_DEFAULT;
    colorDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    renderer::device::g_device->CreateTexture2D(&colorDesc, nullptr, g_viewportTexture.put());
    renderer::device::g_device->CreateRenderTargetView(g_viewportTexture.get(), nullptr, g_viewportRTV.put());
    renderer::device::g_device->CreateShaderResourceView(g_viewportTexture.get(), nullptr, g_viewportSRV.put());

    D3D11_TEXTURE2D_DESC depthDesc = colorDesc;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    renderer::device::g_device->CreateTexture2D(&depthDesc, nullptr, g_viewportDepthStencil.put());
    renderer::device::g_device->CreateDepthStencilView(g_viewportDepthStencil.get(), nullptr, g_viewportDSV.put());

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    renderer::device::g_device->CreateDepthStencilState(&dsDesc, g_viewportDepthStencilState.put());

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
    renderer::camera::update(90.0f, g_viewportViewport.Width / g_viewportViewport.Height, 0.1f, 1000.0f);

    ID3D11RenderTargetView* const viewportRtvs[] = { g_viewportRTV.get()};
    renderer::device::g_context->OMSetRenderTargets(1, viewportRtvs, g_viewportDSV.get());
    renderer::device::g_context->OMSetDepthStencilState(g_viewportDepthStencilState.get(), 1);

    renderer::device::g_context->ClearRenderTargetView(g_viewportRTV.get(), color);
    renderer::device::g_context->ClearDepthStencilView(g_viewportDSV.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    renderer::device::g_context->RSSetViewports(1, &g_viewportViewport);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    ID3D11Buffer* const vertexBuffers[] = { g_vertexBuffer.get() };
    renderer::device::g_context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
    renderer::device::g_context->IASetIndexBuffer(g_indexBuffer.get(), DXGI_FORMAT_R16_UINT, 0);
    renderer::device::g_context->IASetInputLayout(g_inputLayout.get());
    renderer::device::g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    renderer::device::g_context->VSSetShader(g_vertexShader.get(), nullptr, 0);
    renderer::device::g_context->PSSetShader(g_pixelShader.get(), nullptr, 0);

    DirectX::XMFLOAT4X4 viewProjection = {};
    DirectX::XMStoreFloat4x4(&viewProjection, DirectX::XMMatrixTranspose(ashenvale::renderer::camera::g_viewProjectionMatrix));

    D3D11_MAPPED_SUBRESOURCE mappedResource = {};

    renderer::device::g_context->Map( g_cameraBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    memcpy(mappedResource.pData, &viewProjection, sizeof(DirectX::XMFLOAT4X4));
    renderer::device::g_context->Unmap(g_cameraBuffer.get(), 0);

    ID3D11Buffer* const pixelBuffers[] = { g_cameraBuffer.get() };
    renderer::device::g_context->VSSetConstantBuffers(0, 1, pixelBuffers);

    renderer::device::g_context->DrawIndexed(3, 0, 0);

    ID3D11RenderTargetView* const rtvs[] = { ashenvale::renderer::swapchain::g_renderTargetView.get() };
    renderer::device::g_context->OMSetRenderTargets(1, rtvs, nullptr);
    renderer::device::g_context->ClearRenderTargetView(ashenvale::renderer::swapchain::g_renderTargetView.get(), color);

    renderer::device::g_context->RSSetViewports(1, &swapchain::g_viewport);
    ashenvale::editor::render();

    ashenvale::renderer::swapchain::g_swapChain->Present(1, 0);
}
