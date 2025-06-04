#include "debug_wireframe_pass.h"

#include <d3d11_4.h>
#include <winrt/base.h>
#include "renderer/device.h"
#include "renderer/camera.h"
#include "renderer/shader_compiler.h"
#include "renderer/renderer.h"

using namespace winrt;

struct Vertex {
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 color;
};

namespace
{
	ashenvale::renderer::render_pass::render_pass_pso g_pso;

    com_ptr<ID3D11Buffer> g_vertexBuffer;
    com_ptr<ID3D11Buffer> g_indexBuffer;
    com_ptr<ID3D11VertexShader> g_vertexShader;
    com_ptr<ID3D11PixelShader> g_pixelShader;
    com_ptr<ID3D11InputLayout> g_inputLayout;
    com_ptr<ID3D11Buffer> g_cameraBuffer;
    com_ptr<ID3D11SamplerState> g_sampler;
    com_ptr<ID3D11RasterizerState> g_rasterizer;
    com_ptr<ID3D11DepthStencilState> g_depthStencilState;
}

void ashenvale::renderer::render_pass::debug_wireframe::initialize()
{
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

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    renderer::device::g_device->CreateSamplerState(&sampDesc, g_sampler.put());

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterDesc.CullMode = D3D11_CULL_BACK;
    rasterDesc.DepthClipEnable = true;
    renderer::device::g_device->CreateRasterizerState(&rasterDesc, g_rasterizer.put());

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    renderer::device::g_device->CreateDepthStencilState(&dsDesc, g_depthStencilState.put());

    g_pso = {};
    g_pso.bs = nullptr;
    g_pso.dss = g_depthStencilState.get();
    g_pso.inputLayout = g_inputLayout.get();
    g_pso.ps = g_pixelShader.get();
    g_pso.vs = g_vertexShader.get();
    g_pso.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    g_pso.rs = g_rasterizer.get();
}

void ashenvale::renderer::render_pass::debug_wireframe::execute(const render_pass_context& context)
{
	bind_pso(g_pso);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    if(context.debug_wireframe.clear)
    {
        // TODO: Update Depth And Stencil Values from context.
        const float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        ashenvale::renderer::device::g_context->ClearRenderTargetView(context.geometry.rtv, color);
        ashenvale::renderer::device::g_context->ClearDepthStencilView(context.geometry.dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0.0);
    }

    ID3D11RenderTargetView* const rtvs[] = { context.geometry.rtv };
    ashenvale::renderer::device::g_context->OMSetRenderTargets(1, rtvs, context.geometry.dsv);
    ashenvale::renderer::device::g_context->RSSetViewports(1, &ashenvale::renderer::g_viewportViewport);

    ID3D11Buffer* const vertexBuffers[] = { g_vertexBuffer.get() };
    ashenvale::renderer::device::g_context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
    ashenvale::renderer::device::g_context->IASetIndexBuffer(g_indexBuffer.get(), DXGI_FORMAT_R16_UINT, 0);

    DirectX::XMFLOAT4X4 viewProjection = {};
    DirectX::XMStoreFloat4x4(&viewProjection, DirectX::XMMatrixTranspose(ashenvale::renderer::camera::g_viewProjectionMatrix));

    D3D11_MAPPED_SUBRESOURCE mappedResource = {};

    ashenvale::renderer::device::g_context->Map(g_cameraBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    memcpy(mappedResource.pData, &viewProjection, sizeof(DirectX::XMFLOAT4X4));
    ashenvale::renderer::device::g_context->Unmap(g_cameraBuffer.get(), 0);

    ID3D11Buffer* const pixelBuffers[] = { g_cameraBuffer.get() };
    ashenvale::renderer::device::g_context->VSSetConstantBuffers(0, 1, pixelBuffers);

    ashenvale::renderer::device::g_context->DrawIndexed(3, 0, 0);
}