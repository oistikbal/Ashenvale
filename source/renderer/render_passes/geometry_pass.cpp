#include "geometry_pass.h"

#include "editor/inspector.h"
#include "renderer/camera.h"
#include "renderer/device.h"
#include "renderer/renderer.h"
#include "renderer/shader.h"
#include "scene/scene.h"
#include "scene/skydome.h"
#include "tonemap_pass.h"
#include <d3d11_4.h>
#include <winrt/base.h>

using namespace winrt;

namespace
{
com_ptr<ID3D11Buffer> g_cameraBuffer;
com_ptr<ID3D11Buffer> g_lightBuffer;
com_ptr<ID3D11Buffer> g_lightMetaBuffer;
com_ptr<ID3D11Texture2D> g_shadowMapArray;
com_ptr<ID3D11ShaderResourceView> g_lightSrv;
com_ptr<ID3D11ShaderResourceView> g_shadowMapArraySrv;

constexpr uint32_t g_max_light_count = 64;
} // namespace

void ashenvale::renderer::render_pass::geometry::initialize()
{
    D3D11_BUFFER_DESC cameraBufferDesc = {};
    cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    cameraBufferDesc.ByteWidth = sizeof(ashenvale::renderer::camera::camera_buffer);
    cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    renderer::device::g_device->CreateBuffer(&cameraBufferDesc, nullptr, g_cameraBuffer.put());

    D3D11_BUFFER_DESC lightMetaBufferDesc = {};
    lightMetaBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    lightMetaBufferDesc.ByteWidth = sizeof(ashenvale::scene::light_meta);
    lightMetaBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    lightMetaBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    renderer::device::g_device->CreateBuffer(&lightMetaBufferDesc, nullptr, g_lightMetaBuffer.put());

    D3D11_BUFFER_DESC lightBufferDesc = {};
    lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    lightBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    lightBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    lightBufferDesc.StructureByteStride = sizeof(ashenvale::scene::light_buffer);
    lightBufferDesc.ByteWidth = sizeof(ashenvale::scene::light_buffer) * g_max_light_count;

    renderer::device::g_device->CreateBuffer(&lightBufferDesc, nullptr, g_lightBuffer.put());

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.ElementOffset = 0;
    srvDesc.Buffer.NumElements = g_max_light_count;

    renderer::device::g_device->CreateShaderResourceView(g_lightBuffer.get(), nullptr, g_lightSrv.put());

    D3D11_TEXTURE2D_DESC texArrayDesc = {};
    texArrayDesc.Width = 1024;
    texArrayDesc.Height = 1024;
    texArrayDesc.MipLevels = 1;
    texArrayDesc.ArraySize = 64;
    texArrayDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    texArrayDesc.SampleDesc.Count = 1;
    texArrayDesc.Usage = D3D11_USAGE_DEFAULT;
    texArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
    texArrayDesc.MiscFlags = 0;

    renderer::device::g_device->CreateTexture2D(&texArrayDesc, nullptr, g_shadowMapArray.put());

    D3D11_SHADER_RESOURCE_VIEW_DESC shadowSrvDesc = {};
    shadowSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    shadowSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    shadowSrvDesc.Texture2DArray.MostDetailedMip = 0;
    shadowSrvDesc.Texture2DArray.MipLevels = 1;
    shadowSrvDesc.Texture2DArray.FirstArraySlice = 0;
    shadowSrvDesc.Texture2DArray.ArraySize = 64;

    renderer::device::g_device->CreateShaderResourceView(g_shadowMapArray.get(), &shadowSrvDesc,
                                                         g_shadowMapArraySrv.put());
}

void ashenvale::renderer::render_pass::geometry::execute(const render_pass_context &context)
{
    reset_pipeline();

    UINT stride = sizeof(ashenvale::scene::vertex);
    UINT offset = 0;

    std::vector<scene::light_buffer> gpuLights;

    size_t lightIndex = 0;
    scene::g_world.each([&](flecs::entity le, const ashenvale::scene::light &l,
                            const ashenvale::scene::transform &ltc) {
        ashenvale::scene::light_buffer gpu;
        gpu.position = ltc.position;
        gpu.color = l.color;
        gpu.intensity = l.intensity;
        gpu.light_type = static_cast<uint32_t>(l.type);
        gpu.linear = 0.7f;
        gpu.quadratic = 1.8f;
        gpu.range = l.range;
        gpu.spot_inner_cone_angle = l.spot_inner_cone_angle;
        gpu.spot_outer_cone_angle = l.spot_outer_cone_angle;
        gpuLights.push_back(gpu);

        D3D11_VIEWPORT viewport = {};
        viewport.Width = 1024;
        viewport.Height = 1024;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;

        ashenvale::renderer::device::g_context->OMSetRenderTargets(0, nullptr, l.shadowDepthView.get());
        ashenvale::renderer::device::g_context->RSSetViewports(1, &viewport);

        ashenvale::renderer::device::g_context->ClearDepthStencilView(l.shadowDepthView.get(), D3D11_CLEAR_DEPTH, 1.0f,
                                                                      0);

        DirectX::XMMATRIX viewMatrix{};
        DirectX::XMMATRIX projectionMatrix{};
        DirectX::XMMATRIX viewProjectionMatrix{};

        DirectX::XMVECTOR ltc_quat_rot = DirectX::XMQuaternionRotationRollPitchYaw(
            DirectX::XMConvertToRadians(ltc.rotation.x), DirectX::XMConvertToRadians(ltc.rotation.y),
            DirectX::XMConvertToRadians(ltc.rotation.z));

        DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationQuaternion(ltc_quat_rot);

        DirectX::XMVECTOR forward = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0, 0, 1, 0), rotationMatrix);

        DirectX::XMVECTOR eye = DirectX::XMLoadFloat3(&ltc.position);
        DirectX::XMVECTOR up = DirectX::XMVectorSet(0, 1, 0, 0);

        viewMatrix = DirectX::XMMatrixLookToLH(eye, forward, up);

        if (l.type == scene::light::light_type::directional)
        {

            float near_plane = 0.1f, far_plane = 100.0f;
            projectionMatrix = DirectX::XMMatrixOrthographicLH(10.0f, 10.0f, near_plane, far_plane);
            viewProjectionMatrix = XMMatrixMultiply(viewMatrix, projectionMatrix);
        }
        else
        {
            float near_plane = 0.1f, far_plane = 100.0f;
            projectionMatrix =
                DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, viewport.Width / viewport.Height, 0.1f, 1000.0f);
            viewProjectionMatrix = XMMatrixMultiply(viewMatrix, projectionMatrix);
        }

        scene::g_world.each(
            [&](flecs::entity e, ashenvale::scene::transform &tc, ashenvale::scene::mesh_renderer &mrc) {
                DirectX::XMVECTOR quat_rot = DirectX::XMQuaternionRotationRollPitchYaw(
                    DirectX::XMConvertToRadians(tc.rotation.x), DirectX::XMConvertToRadians(tc.rotation.y),
                    DirectX::XMConvertToRadians(tc.rotation.z));

                DirectX::XMMATRIX world = DirectX::XMMatrixScaling(tc.scale.x, tc.scale.y, tc.scale.z) *
                                          DirectX::XMMatrixRotationQuaternion(quat_rot) *
                                          DirectX::XMMatrixTranslation(tc.position.x, tc.position.y, tc.position.z);

                renderer::camera::camera_buffer mvp = {};
                DirectX::XMStoreFloat4x4(&mvp.world, DirectX::XMMatrixTranspose(world));
                DirectX::XMStoreFloat4x4(&mvp.view, DirectX::XMMatrixTranspose(viewMatrix));
                DirectX::XMStoreFloat4x4(&mvp.projection, DirectX::XMMatrixTranspose(viewProjectionMatrix));
                mvp.cameraPosition = ltc.position;

                D3D11_MAPPED_SUBRESOURCE mappedResource = {};
                renderer::device::g_context->Map(g_cameraBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
                memcpy(mappedResource.pData, &mvp, sizeof(mvp));
                renderer::device::g_context->Unmap(g_cameraBuffer.get(), 0);

                ID3D11Buffer *const cameraBuffer[] = {g_cameraBuffer.get()};
                renderer::device::g_context->VSSetConstantBuffers(0, 1, cameraBuffer);

                UINT stride = sizeof(scene::vertex);
                UINT offset = 0;

                size_t count = std::min(mrc.meshes.size(), mrc.materials.size());
                for (size_t i = 0; i < count; ++i)
                {
                    const scene::mesh &m = mrc.meshes[i];

                    ID3D11Buffer *const vertexBuffers[] = {m.vertexBuffer.get()};
                    renderer::device::g_context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
                    renderer::device::g_context->IASetIndexBuffer(m.indexBuffer.get(), DXGI_FORMAT_R32_UINT, 0);
                    renderer::device::g_context->VSSetShader(renderer::shader::g_shadowShader.vertexShader.get(),
                                                             nullptr, 0);
                    renderer::device::g_context->IASetInputLayout(renderer::shader::g_shadowShader.inputLayout.get());
                    renderer::device::g_context->PSSetShader(nullptr, nullptr, 0);
                    renderer::device::g_context->DrawIndexed(m.indexCount, 0, 0);
                }
            });

        ID3D11Resource *resource = nullptr;
        l.shadowSrv->GetResource(&resource);

        renderer::device::g_context->CopySubresourceRegion(g_shadowMapArray.get(), lightIndex, 0, 0, 0, resource, 0,
                                                           nullptr);

        ++lightIndex;
    });

    ////////////////////////////////////

    ID3D11RenderTargetView *const rtvs[] = {context.geometry.rtv};
    ashenvale::renderer::device::g_context->OMSetRenderTargets(1, rtvs, context.geometry.dsv);
    ashenvale::renderer::device::g_context->RSSetViewports(1, &ashenvale::renderer::g_viewportViewport);

    scene::skydome::render();

    D3D11_MAPPED_SUBRESOURCE mapped;
    renderer::device::g_context->Map(g_lightBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, gpuLights.data(), sizeof(scene::light_buffer) * gpuLights.size());
    renderer::device::g_context->Unmap(g_lightBuffer.get(), 0);

    renderer::device::g_context->Map(g_lightMetaBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    scene::light_meta lm = {};
    lm.lightCount = gpuLights.size();
    memcpy(mapped.pData, &lm, sizeof(scene::light_meta));
    renderer::device::g_context->Unmap(g_lightMetaBuffer.get(), 0);

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
        renderer::device::g_context->VSSetConstantBuffers(0, 1, cameraBuffer);
        renderer::device::g_context->PSSetConstantBuffers(0, 1, cameraBuffer);

        ID3D11ShaderResourceView *const srvBuffer[] = {g_lightSrv.get()};
        renderer::device::g_context->PSSetShaderResources(2, 1, srvBuffer);

        ID3D11Buffer *const lightMetaBuffer[] = {g_lightMetaBuffer.get()};
        renderer::device::g_context->PSSetConstantBuffers(2, 1, lightMetaBuffer);

        ID3D11Buffer *const exposureBuffer[] = {tonemap::g_exposureBuffer.get()};
        renderer::device::g_context->PSSetConstantBuffers(3, 1, exposureBuffer);

        ID3D11ShaderResourceView *const shadowSrv[] = {g_shadowMapArraySrv.get()};
        renderer::device::g_context->PSSetShaderResources(3, 1, shadowSrv);

        UINT stride = sizeof(scene::vertex);
        UINT offset = 0;

        size_t count = std::min(mrc.meshes.size(), mrc.materials.size());
        for (size_t i = 0; i < count; ++i)
        {
            const scene::mesh &m = mrc.meshes[i];
            const scene::material &mat = mrc.materials[i];

            scene::material_bind(mat, renderer::device::g_context.get());

            ID3D11Buffer *const vertexBuffers[] = {m.vertexBuffer.get()};
            renderer::device::g_context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
            renderer::device::g_context->IASetIndexBuffer(m.indexBuffer.get(), DXGI_FORMAT_R32_UINT, 0);
            renderer::device::g_context->DrawIndexed(m.indexCount, 0, 0);
        }
    });
}
