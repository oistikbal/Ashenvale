#include "debug_wireframe_pass.h"

#include "renderer/camera.h"
#include "renderer/device.h"
#include "renderer/renderer.h"
#include "renderer/shader.h"
#include "scene/scene.h"
#include <d3d11_4.h>
#include <winrt/base.h>

using namespace winrt;

namespace
{

com_ptr<ID3D11Buffer> g_cameraBuffer;
com_ptr<ID3D11RasterizerState> g_rasterizer;
com_ptr<ID3D11DepthStencilState> g_wireframeDepthState;
} // namespace

void ashenvale::renderer::render_pass::debug_wireframe::initialize()
{

    D3D11_BUFFER_DESC cameraBufferDesc = {};
    cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    cameraBufferDesc.ByteWidth = sizeof(camera::mvp_buffer);
    cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    renderer::device::g_device->CreateBuffer(&cameraBufferDesc, nullptr, g_cameraBuffer.put());

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthClipEnable = true;
    rasterDesc.DepthBias = -1000;
    rasterDesc.SlopeScaledDepthBias = 0.0f;
    rasterDesc.DepthBiasClamp = 0.0f;
    renderer::device::g_device->CreateRasterizerState(&rasterDesc, g_rasterizer.put());

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    renderer::device::g_device->CreateDepthStencilState(&depthStencilDesc, g_wireframeDepthState.put());
}

void ashenvale::renderer::render_pass::debug_wireframe::execute(const render_pass_context &context)
{
    reset_pipeline();

    if (context.debug_wireframe.clear)
    {
        // TODO: Update Depth And Stencil Values from context.
        const float color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        ashenvale::renderer::device::g_context->ClearRenderTargetView(context.geometry.rtv, color);
        ashenvale::renderer::device::g_context->ClearDepthStencilView(
            context.geometry.dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0.0);
    }

    ID3D11RenderTargetView *const rtvs[] = {context.geometry.rtv};
    ashenvale::renderer::device::g_context->OMSetRenderTargets(1, rtvs, context.geometry.dsv);
    ashenvale::renderer::device::g_context->RSSetViewports(1, &ashenvale::renderer::g_viewportViewport);

    UINT stride = sizeof(ashenvale::scene::vertex);
    UINT offset = 0;

    for (auto &node : ashenvale::scene::g_nodes)
    {
        scene::update_world_matrix(node);
        ashenvale::renderer::camera::mvp_buffer mvp = {};
        DirectX::XMStoreFloat4x4(&mvp.world, DirectX::XMMatrixTranspose(node.worldMatrix));
        DirectX::XMStoreFloat4x4(&mvp.view, DirectX::XMMatrixTranspose(ashenvale::renderer::camera::g_viewMatrix));
        DirectX::XMStoreFloat4x4(&mvp.projection,
                                 DirectX::XMMatrixTranspose(ashenvale::renderer::camera::g_projectionMatrix));

        D3D11_MAPPED_SUBRESOURCE mappedResource = {};

        ashenvale::renderer::device::g_context->Map(g_cameraBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                                                    &mappedResource);
        memcpy(mappedResource.pData, &mvp, sizeof(ashenvale::renderer::camera::mvp_buffer));
        ashenvale::renderer::device::g_context->Unmap(g_cameraBuffer.get(), 0);

        ID3D11Buffer *const cameraBuffer[] = {g_cameraBuffer.get()};

        ashenvale::renderer::device::g_context->VSSetConstantBuffers(0, 1, cameraBuffer);
        for (const auto &renderable : node.renderables)
        {
            const ashenvale::scene::mesh &m = ashenvale::scene::g_meshes[renderable.meshIndex];
            const ashenvale::scene::material &mat = ashenvale::scene::g_materials[renderable.materialIndex];

            ashenvale::scene::material_bind(mat, ashenvale::renderer::device::g_context.get());

            ID3D11Buffer *const vertexBuffers[] = {m.vertexBuffer.get()};
            ashenvale::renderer::device::g_context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
            ashenvale::renderer::device::g_context->IASetIndexBuffer(m.indexBuffer.get(), DXGI_FORMAT_R32_UINT, 0);
            ashenvale::renderer::device::g_context->RSSetState(g_rasterizer.get());
            if (!context.debug_wireframe.clear)
            {
                ashenvale::renderer::device::g_context->PSSetShader(shader::g_wireframeShader.pixelShader.get(),
                                                                    nullptr, 0);
            }
            ashenvale::renderer::device::g_context->DrawIndexed(m.indexCount, 0, 0);
        }
    }
}