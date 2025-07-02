#include "normal_pass.h"
#include "renderer/camera.h"
#include "renderer/device.h"
#include "renderer/renderer.h"
#include "renderer/shader.h"

#include "scene/scene.h"
#include <DirectXMath.h>
#include <d3d11_4.h>
#include <winrt/base.h>

using namespace winrt;

namespace
{
com_ptr<ID3D11Buffer> g_cameraBuffer;
}

void ashenvale::renderer::render_pass::normal::initialize()
{
    D3D11_BUFFER_DESC cameraBufferDesc = {};
    cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    cameraBufferDesc.ByteWidth = sizeof(ashenvale::renderer::camera::camera_buffer);
    cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    renderer::device::g_device->CreateBuffer(&cameraBufferDesc, nullptr, g_cameraBuffer.put());
}

void ashenvale::renderer::render_pass::normal::execute(const render_pass_context &context)
{
    ID3D11RenderTargetView *const rtvs[] = {context.normal.rtv};
    ashenvale::renderer::device::g_context->OMSetRenderTargets(1, rtvs, context.normal.dsv);
    ashenvale::renderer::device::g_context->RSSetViewports(1, &ashenvale::renderer::g_viewportViewport);

    UINT stride = sizeof(ashenvale::scene::vertex);
    UINT offset = 0;

    scene::g_world.each([&](flecs::entity e, ashenvale::scene::transform &tc, ashenvale::scene::mesh_renderer &mrc) {
        DirectX::XMVECTOR quat_rot = DirectX::XMQuaternionRotationRollPitchYaw(
            DirectX::XMConvertToRadians(tc.rotation.x), DirectX::XMConvertToRadians(tc.rotation.y),
            DirectX::XMConvertToRadians(tc.rotation.z));

        DirectX::XMMATRIX world = DirectX::XMMatrixScaling(tc.scale.x, tc.scale.y, tc.scale.z) *
                                  DirectX::XMMatrixRotationQuaternion(quat_rot) *
                                  DirectX::XMMatrixTranslation(tc.position.x, tc.position.y, tc.position.z);

        renderer::camera::camera_buffer mvp = {};
        DirectX::XMStoreFloat4x4(&mvp.world, DirectX::XMMatrixTranspose(world));
        DirectX::XMStoreFloat4x4(&mvp.view, DirectX::XMMatrixTranspose(renderer::camera::g_viewMatrix));
        DirectX::XMStoreFloat4x4(&mvp.projection, DirectX::XMMatrixTranspose(renderer::camera::g_projectionMatrix));
        mvp.cameraPosition = renderer::camera::g_position;

        D3D11_MAPPED_SUBRESOURCE mappedResource = {};
        renderer::device::g_context->Map(g_cameraBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        memcpy(mappedResource.pData, &mvp, sizeof(mvp));
        renderer::device::g_context->Unmap(g_cameraBuffer.get(), 0);

        ID3D11Buffer *const cameraBuffer[] = {g_cameraBuffer.get()};

        renderer::device::g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
        ashenvale::renderer::device::g_context->VSSetShader(shader::g_normalShader.vertexShader.get(), nullptr, 0);
        ashenvale::renderer::device::g_context->IASetInputLayout(shader::g_normalShader.inputLayout.get());
        ashenvale::renderer::device::g_context->PSSetShader(shader::g_normalShader.pixelShader.get(), nullptr, 0);
        ashenvale::renderer::device::g_context->GSSetShader(shader::g_normalShader.geometryShader.get(), nullptr, 0);

        renderer::device::g_context->VSSetConstantBuffers(0, 1, cameraBuffer);
        renderer::device::g_context->GSSetConstantBuffers(0, 1, cameraBuffer);

        UINT stride = sizeof(scene::vertex);
        UINT offset = 0;

        size_t count = std::min(mrc.meshes.size(), mrc.materials.size());
        for (size_t i = 0; i < count; ++i)
        {
            const scene::mesh &m = mrc.meshes[i];

            ID3D11Buffer *const vertexBuffers[] = {m.vertexBuffer.get()};
            renderer::device::g_context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
            renderer::device::g_context->IASetIndexBuffer(m.indexBuffer.get(), DXGI_FORMAT_R32_UINT, 0);

            renderer::device::g_context->Draw(m.vertexCount, 0);
        }
    });
}
