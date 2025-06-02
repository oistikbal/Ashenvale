#include "render_pass.h"
#include "render_graph.h"
#include "renderer.h"
#include "shader_compiler.h"
#include "device.h"
#include "camera.h"
#include "editor/editor.h"
#include "swapchain.h"

using namespace winrt;


namespace
{
    com_ptr<ID3D11Buffer> g_vertexBuffer;
    com_ptr<ID3D11Buffer> g_indexBuffer;
    com_ptr<ID3D11VertexShader> g_vertexShader;
    com_ptr<ID3D11VertexShader> g_quadVS;
    com_ptr<ID3D11PixelShader> g_pixelShader;
    com_ptr<ID3D11PixelShader> g_quadPS;
    com_ptr<ID3D11InputLayout> g_inputLayout;
    com_ptr<ID3D11InputLayout> g_quadInputLayout;
    com_ptr<ID3D11Buffer> g_cameraBuffer;
    com_ptr<ID3D11SamplerState> g_linearSampler;
    com_ptr<ID3D11RasterizerState> g_rasterizer;
    com_ptr<ID3D11RasterizerState> g_debugDepthRasterize;

    ashenvale::renderer::render_pass::render_pass_info g_clearPassInfo = {};
    ashenvale::renderer::render_pass::render_pass_info g_geometryPassInfo = {};
    ashenvale::renderer::render_pass::render_pass_info g_editorPassInfo = {};
    ashenvale::renderer::render_pass::render_pass_info g_presentPassInfo = {};

    struct Vertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

	void clear_pass_execute(const ashenvale::renderer::render_pass::render_pass_context& context)
	{
		const float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        ID3D11RenderTargetView* const viewportRtvs[] = { ashenvale::renderer::g_viewportRTV.get() };
        ashenvale::renderer::device::g_context->OMSetRenderTargets(1, viewportRtvs, ashenvale::renderer::g_viewportDSV.get());
        ashenvale::renderer::device::g_context->OMSetDepthStencilState(ashenvale::renderer::g_viewportDepthStencilState.get(), 1);

        ashenvale::renderer::device::g_context->ClearRenderTargetView(ashenvale::renderer::g_viewportRTV.get(), color);
        ashenvale::renderer::device::g_context->ClearDepthStencilView(ashenvale::renderer::g_viewportDSV.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        ashenvale::renderer::device::g_context->RSSetViewports(1, &ashenvale::renderer::g_viewportViewport);
        ashenvale::renderer::device::g_context->RSSetState(g_rasterizer.get());

	}

    void geometry_pass_execute(const ashenvale::renderer::render_pass::render_pass_context& context)
    {
        UINT stride = sizeof(Vertex);
        UINT offset = 0;

        ID3D11Buffer* const vertexBuffers[] = { g_vertexBuffer.get() };
        ashenvale::renderer::device::g_context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
        ashenvale::renderer::device::g_context->IASetIndexBuffer(g_indexBuffer.get(), DXGI_FORMAT_R16_UINT, 0);
        ashenvale::renderer::device::g_context->IASetInputLayout(g_inputLayout.get());
        ashenvale::renderer::device::g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ashenvale::renderer::device::g_context->VSSetShader(g_vertexShader.get(), nullptr, 0);
        ashenvale::renderer::device::g_context->PSSetShader(g_pixelShader.get(), nullptr, 0);

        DirectX::XMFLOAT4X4 viewProjection = {};
        DirectX::XMStoreFloat4x4(&viewProjection, DirectX::XMMatrixTranspose(ashenvale::renderer::camera::g_viewProjectionMatrix));

        D3D11_MAPPED_SUBRESOURCE mappedResource = {};

        ashenvale::renderer::device::g_context->Map(g_cameraBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        memcpy(mappedResource.pData, &viewProjection, sizeof(DirectX::XMFLOAT4X4));
        ashenvale::renderer::device::g_context->Unmap(g_cameraBuffer.get(), 0);

        ID3D11Buffer* const pixelBuffers[] = { g_cameraBuffer.get() };
        ashenvale::renderer::device::g_context->VSSetConstantBuffers(0, 1, pixelBuffers);

        ashenvale::renderer::device::g_context->DrawIndexed(3, 0, 0);

        ID3D11Buffer* nullBuffer = nullptr;
        ashenvale::renderer::device::g_context->VSSetConstantBuffers(0, 1, &nullBuffer);
    }

    void editor_pass_execute(const ashenvale::renderer::render_pass::render_pass_context& context)
    {
        const float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        ashenvale::renderer::device::g_context->ClearRenderTargetView(ashenvale::renderer::swapchain::g_renderTargetView.get(), color);

        ID3D11RenderTargetView* const rtvs[] = { ashenvale::renderer::swapchain::g_renderTargetView.get() };
        ashenvale::renderer::device::g_context->OMSetRenderTargets(1, rtvs, nullptr);
        ashenvale::renderer::device::g_context->RSSetViewports(1, &ashenvale::renderer::swapchain::g_viewport);
        ashenvale::editor::render();
    }

    void present_pass_execute(const ashenvale::renderer::render_pass::render_pass_context& context)
    {
        ashenvale::renderer::swapchain::g_swapChain->Present(1, 0);
    }

    void debug_depth_pass_execute(const ashenvale::renderer::render_pass::render_pass_context& context)
    {
        const float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        ID3D11RenderTargetView* const viewportRtvs[] = { ashenvale::renderer::g_viewportRTV.get() };
        ashenvale::renderer::device::g_context->OMSetRenderTargets(1, viewportRtvs, nullptr);
        ashenvale::renderer::device::g_context->ClearRenderTargetView(ashenvale::renderer::g_viewportRTV.get(), color);
        ashenvale::renderer::device::g_context->OMSetDepthStencilState(ashenvale::renderer::g_viewportDepthStencilStateDisabled.get(), 1);
        ashenvale::renderer::device::g_context->RSSetViewports(1, &ashenvale::renderer::g_viewportViewport);
        ashenvale::renderer::device::g_context->RSSetState(g_debugDepthRasterize.get());

        ashenvale::renderer::device::g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        ID3D11Buffer* nullBuffer = nullptr;
        UINT zero = 0;
        ashenvale::renderer::device::g_context->IASetVertexBuffers(0, 1, &nullBuffer, &zero, &zero);
        ashenvale::renderer::device::g_context->IASetInputLayout(nullptr);

        ashenvale::renderer::device::g_context->VSSetShader(g_quadVS.get(), nullptr, 0);
        ashenvale::renderer::device::g_context->PSSetShader(g_quadPS.get(), nullptr, 0);

        ID3D11SamplerState* const samplers[] = { g_linearSampler.get() };
        ashenvale::renderer::device::g_context->PSSetSamplers(0, 1, samplers);
        ID3D11ShaderResourceView* const srvs[] = { ashenvale::renderer::g_viewportDepthSRV.get() };
        ashenvale::renderer::device::g_context->PSSetShaderResources(0, 1, srvs);


        ashenvale::renderer::device::g_context->Draw(4, 0);

        ID3D11ShaderResourceView* nullSRV = nullptr;
        ashenvale::renderer::device::g_context->PSSetShaderResources(0, 1, &nullSRV);
        ID3D11SamplerState* nullSampler = nullptr;
        ashenvale::renderer::device::g_context->PSSetSamplers(0, 1, &nullSampler);
    }
}

void ashenvale::renderer::render_pass::initialize()
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

    ////////////

    renderer::shader_compiler::compile(L"quad_vs.hlsl", "main", "vs_5_0", nullptr, vsBlob.put(), errorBlob.put());

    renderer::device::g_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, g_quadVS.put());

    renderer::shader_compiler::compile(L"quad_ps.hlsl", "main", "ps_5_0", nullptr, psBlob.put(), errorBlob.put());

    renderer::device::g_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, g_quadPS.put());

    renderer::device::g_device->CreateInputLayout(nullptr, 0, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), g_quadInputLayout.put());


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
    renderer::device::g_device->CreateSamplerState(&sampDesc, g_linearSampler.put());

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_BACK;
    rasterDesc.DepthClipEnable = true;
    renderer::device::g_device->CreateRasterizerState(&rasterDesc, g_rasterizer.put());

    rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthClipEnable = false;
    renderer::device::g_device->CreateRasterizerState(&rasterDesc, g_debugDepthRasterize.put());

    ///////////////////////

    g_clearPassInfo.execute = clear_pass_execute;
    g_clearPassInfo.context.clear = clear_pass_context{};

    g_geometryPassInfo.execute = geometry_pass_execute;
    g_geometryPassInfo.context.geometry = geometry_pass_context{};

    g_editorPassInfo.execute = editor_pass_execute;
    g_editorPassInfo.context.editor = editor_pass_context{};

    g_presentPassInfo.execute = present_pass_execute;
    g_presentPassInfo.context.present = present_pass_context{};

    render_graph::g_forwardPathPassInfos[0] = g_clearPassInfo;
    render_graph::g_forwardPathPassInfos[1] = g_geometryPassInfo;

    render_graph::g_editorPassInfo = g_editorPassInfo;
    render_graph::g_presentPassInfo = g_presentPassInfo;

    render_graph::g_debugDepthPassInfo.execute = debug_depth_pass_execute;
    render_graph::g_debugDepthPassInfo.context.debug_depth = debug_depth_pass_context{};
}