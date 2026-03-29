#include "object_pass.h"
#include "common.h"
#include "renderer/pipeline/pipeline.h"
#include "renderer/renderer.h"
#include "resource/resource_manager.h"
#include <DirectXMath.h>

using namespace DirectX;

struct vertex
{
    DirectX::XMFLOAT3 position = {};
    DirectX::XMFLOAT3 normal = {0.0f, 0.0f, 1.0f};
    DirectX::XMFLOAT4 tangent = {1.0f, 0.0f, 0.0f, 1.0f};
    DirectX::XMFLOAT2 uv = {};
};

void ash::object_pass_render(const frame_context &frame_context, const scene_data &scene_data)
{
    SCOPED_GPU_EVENT(frame_context.cmd, L"ash::object_pass_render")

    constexpr float clear_color[] = {0.0f, 0.0f, 0.0f, 1.0f};

    ID3D12DescriptorHeap *heap[] = {rhi_g_cbv_srv_uav_heap.get(), rhi_g_sampler_heap.get()};
    frame_context.cmd->SetDescriptorHeaps(2, heap);

    frame_context.cmd->SetGraphicsRootSignature(rhi_pl_g_object.root_signature.get());

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = frame_context.viewport_texture;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    frame_context.cmd->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE viewport_rtv_handle = frame_context.viewport_rtv;
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = frame_context.viewport_dsv_rtv;

    frame_context.cmd->OMSetRenderTargets(1, &viewport_rtv_handle, FALSE, &dsv_handle);

    frame_context.cmd->ClearRenderTargetView(viewport_rtv_handle, clear_color, 0, nullptr);
    frame_context.cmd->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    frame_context.cmd->SetPipelineState(rhi_pl_g_object.pso.get());
    frame_context.cmd->RSSetViewports(1, &frame_context.viewport);
    D3D12_RECT scissorRect = {0, 0, static_cast<UINT>(frame_context.viewport.Width),
                              static_cast<UINT>(frame_context.viewport.Height)};
    frame_context.cmd->RSSetScissorRects(1, &scissorRect);
    frame_context.cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (const scene_object& so : scene_data.objects)
    {
        struct scene_constant
        {
            XMFLOAT4X4 mvp;
            uint32_t albedo_index;
        };

        scene_constant sc;
        XMStoreFloat4x4(&sc.mvp, XMLoadFloat4x4(&so.model) * XMLoadFloat4x4(&scene_data.view_proj));
        sc.albedo_index = so.mat.albedo;
        if (const ash::resource_texture *albedo_texture = ash::rm_get_texture(so.mat.albedo))
        {
            sc.albedo_index = albedo_texture->srv_descriptor_index;
        }

        frame_context.cmd->SetGraphicsRoot32BitConstants(0, 17, &sc, 0);

        D3D12_VERTEX_BUFFER_VIEW vbv = {};
        vbv.BufferLocation =
            rm_get_buffer(rm_get_mesh(so.mesh_handle)->vertex_buffer)->resource->GetGPUVirtualAddress();
        vbv.StrideInBytes = sizeof(vertex);
        vbv.SizeInBytes = rm_get_buffer(rm_get_mesh(so.mesh_handle)->vertex_buffer)->size;

        D3D12_INDEX_BUFFER_VIEW ibv = {};
        ibv.BufferLocation = rm_get_buffer(rm_get_mesh(so.mesh_handle)->index_buffer)->resource->GetGPUVirtualAddress();
        ibv.Format = DXGI_FORMAT_R32_UINT;
        ibv.SizeInBytes = rm_get_buffer(rm_get_mesh(so.mesh_handle)->index_buffer)->size;

        frame_context.cmd->IASetVertexBuffers(0, 1, &vbv);
        frame_context.cmd->IASetIndexBuffer(&ibv);

        frame_context.cmd->DrawIndexedInstanced(rm_get_mesh(so.mesh_handle)->index_count, 1, 0, 0, 0);
    }

    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = rhi_g_viewport_texture->GetResource();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    frame_context.cmd->ResourceBarrier(1, &barrier);
}
