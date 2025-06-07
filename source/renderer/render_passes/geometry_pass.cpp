#include "geometry_pass.h"

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
ashenvale::renderer::render_pass::render_pass_pso g_pso;
com_ptr<ID3D11Buffer> g_cameraBuffer;
} // namespace

void ashenvale::renderer::render_pass::geometry::initialize()
{
    D3D11_BUFFER_DESC cameraBufferDesc = {};
    cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    cameraBufferDesc.ByteWidth = sizeof(ashenvale::renderer::camera::mvp_buffer);
    cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    renderer::device::g_device->CreateBuffer(&cameraBufferDesc, nullptr, g_cameraBuffer.put());

    g_pso = {};
}

void ashenvale::renderer::render_pass::geometry::execute(const render_pass_context &context)
{
    bind_pso(g_pso);

    ID3D11RenderTargetView *const rtvs[] = {context.geometry.rtv};
    ashenvale::renderer::device::g_context->OMSetRenderTargets(1, rtvs, context.geometry.dsv);
    ashenvale::renderer::device::g_context->RSSetViewports(1, &ashenvale::renderer::g_viewportViewport);


    ashenvale::renderer::camera::mvp_buffer mvp = {};
    DirectX::XMStoreFloat4x4(&mvp.world, DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity()));
    DirectX::XMStoreFloat4x4(&mvp.view, DirectX::XMMatrixTranspose(ashenvale::renderer::camera::g_viewMatrix));
    DirectX::XMStoreFloat4x4(&mvp.projection, DirectX::XMMatrixTranspose(ashenvale::renderer::camera::g_projectionMatrix));

    D3D11_MAPPED_SUBRESOURCE mappedResource = {};

    ashenvale::renderer::device::g_context->Map(g_cameraBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    memcpy(mappedResource.pData, &mvp, sizeof(ashenvale::renderer::camera::mvp_buffer));
    ashenvale::renderer::device::g_context->Unmap(g_cameraBuffer.get(), 0);

    ID3D11Buffer *const cameraBuffer[] = {g_cameraBuffer.get()};

    UINT stride = sizeof(ashenvale::scene::vertex);
    UINT offset = 0;

    ashenvale::renderer::device::g_context->VSSetConstantBuffers(0, 1, cameraBuffer);
    for (const auto &renderable : ashenvale::scene::g_renderables)
    {
        const ashenvale::scene::mesh &m = ashenvale::scene::g_meshes[renderable.meshIndex];
        const ashenvale::scene::material &mat = ashenvale::scene::g_materials[renderable.materialIndex];

        ashenvale::scene::material_bind(mat, ashenvale::renderer::device::g_context.get());

        ID3D11Buffer *const vertexBuffers[] = {m.vertexBuffer.get()};
        ashenvale::renderer::device::g_context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
        ashenvale::renderer::device::g_context->IASetIndexBuffer(m.indexBuffer.get(), DXGI_FORMAT_R32_UINT, 0);
        ashenvale::renderer::device::g_context->DrawIndexed(m.indexCount, 0, 0);
    }
}
