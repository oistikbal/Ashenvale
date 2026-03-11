#include "scene.h"
#include "editor/console.h"
#include "editor/editor.h"
#include "renderer/core/command_queue.h"
#include "renderer/core/swapchain.h"
#include "renderer/pipeline/pipeline.h"
#include "renderer/renderer.h"
#include "resource/resource_manager.h"
#include "scene/camera.h"
#include <common.h>
#include <cstdint>
#include <fastgltf/core.hpp>
#include <fastgltf/math.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <filesystem>
#include <functional>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

using namespace winrt;
using namespace DirectX;

namespace
{
struct vertex
{
    DirectX::XMFLOAT3 position = {};
    DirectX::XMFLOAT3 normal = {0.0f, 0.0f, 1.0f};
    DirectX::XMFLOAT4 tangent = {1.0f, 0.0f, 0.0f, 1.0f};
    DirectX::XMFLOAT2 uv = {};
};

inline com_ptr<D3D12MA::Allocation> g_scene_buffer;

flecs::entity lookup_name_in_scope(const std::string &name, flecs::entity parent)
{
    if (parent.is_valid())
    {
        return parent.lookup(name.c_str(), false);
    }

    return ash::scene_g_world.lookup(name.c_str(), "::", "::", false);
}

D3D12_RESOURCE_DESC make_upload_buffer_desc(uint64_t size)
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    return desc;
}

D3D12MA::ALLOCATION_DESC make_upload_allocation_desc()
{
    D3D12MA::ALLOCATION_DESC desc = {};
    desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    return desc;
}

const fastgltf::Attribute *find_primitive_attribute(const fastgltf::Primitive &primitive, std::string_view name)
{
    for (const fastgltf::Attribute &attribute : primitive.attributes)
    {
        if (attribute.name == name)
        {
            return &attribute;
        }
    }

    return nullptr;
}

bool import_gltf_primitive_resource(const fastgltf::Asset &asset, const fastgltf::Primitive &primitive,
                                    ash::mesh_component &out_mesh)
{
    if (primitive.type != fastgltf::PrimitiveType::Triangles)
    {
        return false;
    }

    const fastgltf::Attribute *position_attribute = find_primitive_attribute(primitive, "POSITION");
    if (!position_attribute || position_attribute->accessorIndex >= asset.accessors.size())
    {
        ash::ed_console_log(ash::ed_console_log_level::warning,
                            "[Scene] glTF mesh import skipped: POSITION attribute missing.");
        return false;
    }

    const fastgltf::Accessor &position_accessor = asset.accessors[position_attribute->accessorIndex];
    std::vector<vertex> vertices(position_accessor.count);
    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
        asset, position_accessor, [&](const fastgltf::math::fvec3 &position, std::size_t index) {
            vertices[index].position = {position[0], position[1], position[2]};
        });

    if (const fastgltf::Attribute *normal_attribute = find_primitive_attribute(primitive, "NORMAL");
        normal_attribute && normal_attribute->accessorIndex < asset.accessors.size())
    {
        const fastgltf::Accessor &normal_accessor = asset.accessors[normal_attribute->accessorIndex];
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
            asset, normal_accessor, [&](const fastgltf::math::fvec3 &normal, std::size_t index) {
                if (index < vertices.size())
                {
                    vertices[index].normal = {normal[0], normal[1], normal[2]};
                }
            });
    }

    if (const fastgltf::Attribute *tangent_attribute = find_primitive_attribute(primitive, "TANGENT");
        tangent_attribute && tangent_attribute->accessorIndex < asset.accessors.size())
    {
        const fastgltf::Accessor &tangent_accessor = asset.accessors[tangent_attribute->accessorIndex];
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(
            asset, tangent_accessor, [&](const fastgltf::math::fvec4 &tangent, std::size_t index) {
                if (index < vertices.size())
                {
                    vertices[index].tangent = {tangent[0], tangent[1], tangent[2], tangent[3]};
                }
            });
    }

    if (const fastgltf::Attribute *uv_attribute = find_primitive_attribute(primitive, "TEXCOORD_0");
        uv_attribute && uv_attribute->accessorIndex < asset.accessors.size())
    {
        const fastgltf::Accessor &uv_accessor = asset.accessors[uv_attribute->accessorIndex];
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
            asset, uv_accessor, [&](const fastgltf::math::fvec2 &uv, std::size_t index) {
                if (index < vertices.size())
                {
                    vertices[index].uv = {uv[0], uv[1]};
                }
            });
    }

    if (vertices.empty())
    {
        ash::ed_console_log(ash::ed_console_log_level::warning,
                            "[Scene] glTF mesh import skipped: mesh has no vertices.");
        return false;
    }

    std::vector<std::uint32_t> indices;
    if (primitive.indicesAccessor.has_value() && primitive.indicesAccessor.value() < asset.accessors.size())
    {
        const fastgltf::Accessor &index_accessor = asset.accessors[primitive.indicesAccessor.value()];
        indices.reserve(index_accessor.count);
        fastgltf::iterateAccessor<std::uint32_t>(asset, index_accessor,
                                                 [&](std::uint32_t index) { indices.push_back(index); });
    }
    else
    {
        indices.resize(vertices.size());
        std::iota(indices.begin(), indices.end(), 0u);
    }

    if (indices.empty())
    {
        ash::ed_console_log(ash::ed_console_log_level::warning,
                            "[Scene] glTF mesh import skipped: mesh has no indices.");
        return false;
    }

    const D3D12MA::ALLOCATION_DESC allocation_desc = make_upload_allocation_desc();
    const ash::resource vertex_buffer =
        ash::rm_create_buffer(make_upload_buffer_desc(sizeof(vertex) * vertices.size()), allocation_desc,
                              D3D12_RESOURCE_STATE_GENERIC_READ, vertices.data(), sizeof(vertex) * vertices.size());
    const ash::resource index_buffer = ash::rm_create_buffer(
        make_upload_buffer_desc(sizeof(std::uint32_t) * indices.size()), allocation_desc,
        D3D12_RESOURCE_STATE_GENERIC_READ, indices.data(), sizeof(std::uint32_t) * indices.size());
    const ash::resource mesh_resource =
        ash::rm_create_mesh(vertex_buffer.handle, index_buffer.handle, static_cast<uint32_t>(indices.size()));

    out_mesh.handle = mesh_resource.handle;
    return true;
}
} // namespace

std::string ash::scene_make_unique_name(std::string_view desired_name, flecs::entity parent, flecs::entity ignore)
{
    const std::string base_name = desired_name.empty() ? "GameObject" : std::string(desired_name);
    std::string candidate_name = base_name;

    uint32_t suffix = 2;
    while (true)
    {
        flecs::entity existing = lookup_name_in_scope(candidate_name, parent);
        const bool is_unique = !existing.is_valid() || (ignore.is_valid() && existing.id() == ignore.id());

        if (is_unique)
        {
            return candidate_name;
        }

        candidate_name = base_name + " (" + std::to_string(suffix++) + ")";
    }
}

void ash::scene_set_entity_name_safe(flecs::entity entity, std::string_view desired_name)
{
    if (!entity.is_valid())
    {
        return;
    }

    const flecs::entity parent = entity.parent();
    const std::string unique_name = scene_make_unique_name(desired_name, parent, entity);
    entity.set_name(unique_name.c_str());
}

flecs::entity ash::scene_create_empty(flecs::entity parent)
{
    static uint32_t game_object_counter = 1;
    const std::string name = "GameObject_" + std::to_string(game_object_counter++);

    flecs::entity game_object_entity = scene_g_world.entity().add<ash::game_object>().set<ash::transform>({});

    if (parent.is_valid())
    {
        game_object_entity.child_of(parent);
    }

    scene_set_entity_name_safe(game_object_entity, name);
    scene_g_selected = game_object_entity;
    ed_console_log(ed_console_log_level::info, "[Scene] Empty entity created.");
    return game_object_entity;
}

bool ash::scene_load_gltf(const std::filesystem::path &path)
{
    ed_console_log(ed_console_log_level::info, "[Scene] glTF import begin.");

    if (!std::filesystem::exists(path))
    {
        ed_console_log(ed_console_log_level::error, "glTF import failed: file does not exist.");
        return false;
    }

    fastgltf::Parser parser(fastgltf::Extensions::KHR_mesh_quantization | fastgltf::Extensions::KHR_lights_punctual);
    constexpr auto options = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::DecomposeNodeMatrices |
                             fastgltf::Options::LoadExternalBuffers;
    constexpr auto categories = fastgltf::Category::Asset | fastgltf::Category::Scenes | fastgltf::Category::Nodes |
                                fastgltf::Category::Meshes | fastgltf::Category::Buffers |
                                fastgltf::Category::BufferViews | fastgltf::Category::Accessors;

    auto gltf_file = fastgltf::MappedGltfFile::FromPath(path);
    if (!bool(gltf_file))
    {
        ed_console_log(ed_console_log_level::error, "glTF import failed: unable to open file.");
        return false;
    }

    auto loaded_asset = parser.loadGltf(gltf_file.get(), path.parent_path(), options, categories);
    if (loaded_asset.error() != fastgltf::Error::None)
    {
        ed_console_log(ed_console_log_level::error,
                       std::format("glTF import failed: {}.", fastgltf::getErrorMessage(loaded_asset.error())));
        return false;
    }

    fastgltf::Asset asset = std::move(loaded_asset.get());
    if (asset.scenes.empty())
    {
        ed_console_log(ed_console_log_level::error, "glTF import failed: no scenes in file.");
        return false;
    }

    std::size_t scene_index = asset.defaultScene.value_or(0);
    if (scene_index >= asset.scenes.size())
    {
        scene_index = 0;
    }

    const auto &gltf_scene = asset.scenes[scene_index];
    std::string scene_name = gltf_scene.name.empty() ? path.stem().string() : std::string(gltf_scene.name.c_str());
    if (scene_name.empty())
    {
        scene_name = "Imported Scene";
    }

    flecs::entity import_root = scene_g_world.entity().add<ash::game_object>().set<ash::transform>({});
    scene_set_entity_name_safe(import_root, "glTF: " + scene_name);
    scene_g_selected = import_root;

    std::function<void(std::size_t, flecs::entity)> import_node = [&](std::size_t node_index, flecs::entity parent) {
        if (node_index >= asset.nodes.size())
        {
            return;
        }

        const auto &node = asset.nodes[node_index];
        std::string node_name =
            node.name.empty() ? ("Node_" + std::to_string(node_index)) : std::string(node.name.c_str());
        if (node.meshIndex.has_value())
        {
            node_name += " [Mesh " + std::to_string(node.meshIndex.value()) + "]";
        }

        ash::transform entity_transform = {};
        if (const auto *trs = std::get_if<fastgltf::TRS>(&node.transform))
        {
            entity_transform.position = {trs->translation[0], trs->translation[1], trs->translation[2]};
            entity_transform.rotation = {trs->rotation[0], trs->rotation[1], trs->rotation[2], trs->rotation[3]};
            entity_transform.scale = {trs->scale[0], trs->scale[1], trs->scale[2]};
        }
        else
        {
            fastgltf::math::fvec3 scale = {1.0f, 1.0f, 1.0f};
            fastgltf::math::fquat rotation(0.0f, 0.0f, 0.0f, 1.0f);
            fastgltf::math::fvec3 translation = {0.0f, 0.0f, 0.0f};

            auto matrix = std::get<fastgltf::math::fmat4x4>(node.transform);
            fastgltf::math::decomposeTransformMatrix(matrix, scale, rotation, translation);

            entity_transform.position = {translation[0], translation[1], translation[2]};
            entity_transform.rotation = {rotation[0], rotation[1], rotation[2], rotation[3]};
            entity_transform.scale = {scale[0], scale[1], scale[2]};
        }

        flecs::entity entity = scene_g_world.entity().add<ash::game_object>().set<ash::transform>(entity_transform);
        if (parent.is_valid())
        {
            entity.child_of(parent);
        }
        scene_set_entity_name_safe(entity, node_name);

        if (node.meshIndex.has_value())
        {
            const std::size_t mesh_index = node.meshIndex.value();
            if (mesh_index < asset.meshes.size())
            {
                const fastgltf::Mesh &mesh = asset.meshes[mesh_index];
                std::size_t imported_primitive_count = 0;

                for (std::size_t primitive_index = 0; primitive_index < mesh.primitives.size(); ++primitive_index)
                {
                    ash::mesh_component mesh_component = {};
                    if (!import_gltf_primitive_resource(asset, mesh.primitives[primitive_index], mesh_component))
                    {
                        continue;
                    }

                    flecs::entity primitive_entity =
                        scene_g_world.entity().add<ash::game_object>().set<ash::transform>({});
                    primitive_entity.child_of(entity);

                    std::string primitive_name = node_name + " Primitive " + std::to_string(primitive_index);
                    scene_set_entity_name_safe(primitive_entity, primitive_name);
                    primitive_entity.set<ash::mesh_component>(mesh_component);
                    ++imported_primitive_count;
                }

                if (mesh.primitives.empty())
                {
                    ash::ed_console_log(ash::ed_console_log_level::warning,
                                        "[Scene] glTF mesh import skipped: mesh has no primitives.");
                }
                else if (imported_primitive_count == 0)
                {
                    ash::ed_console_log(ash::ed_console_log_level::warning,
                                        "[Scene] glTF mesh import skipped: no supported triangle primitives.");
                }
            }
        }

        for (std::size_t child_node_index : node.children)
        {
            import_node(child_node_index, entity);
        }
    };

    for (std::size_t root_node_index : gltf_scene.nodeIndices)
    {
        import_node(root_node_index, import_root);
    }

    ed_console_log(ed_console_log_level::info, "[Scene] glTF import complete.");
    return true;
}

void ash::scene_init()
{
    D3D12_RESOURCE_DESC buf_desc = {};
    buf_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buf_desc.Alignment = 0;
    buf_desc.Width = sizeof(XMFLOAT4X4) * 256;
    buf_desc.Height = 1;
    buf_desc.DepthOrArraySize = 1;
    buf_desc.DepthOrArraySize = 1;
    buf_desc.MipLevels = 1;
    buf_desc.Format = DXGI_FORMAT_UNKNOWN;
    buf_desc.SampleDesc.Count = 1;
    buf_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    buf_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12MA::ALLOCATION_DESC alloc_desc = {};
    alloc_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

    rhi_g_allocator->CreateResource(&alloc_desc, &buf_desc, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, nullptr,
                                    g_scene_buffer.put(), IID_NULL, nullptr);

    assert(g_scene_buffer.get());

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = 256;
    srvDesc.Buffer.StructureByteStride = sizeof(XMFLOAT4X4);
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = rhi_g_cbv_srv_uav_heap->GetCPUDescriptorHandleForHeapStart();
    UINT handle_size = rhi_g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    cpu_handle.ptr += 5 * handle_size;

    rhi_g_device->CreateShaderResourceView(g_scene_buffer->GetResource(), &srvDesc, cpu_handle);
}

void ash::scene_shutdown()
{
    g_scene_buffer = nullptr;
}

void ash::scene_render()
{
    {
        cam_update_view_mat(g_camera);
        cam_update_proj_mat(g_camera, XM_PI / 3, rhi_sw_g_viewport.Width / rhi_sw_g_viewport.Height, 0.1f, 1000.0f);

        XMMATRIX view_proj = cam_get_view_proj_mat(g_camera);

        auto &command_list = rhi_cmd_g_command_list;
        {
            SCOPED_GPU_EVENT(rhi_cmd_g_command_list.get(), L"ash::scene_render")

            constexpr float clear_color[] = {0.0f, 0.0f, 0.0f, 1.0f};

            ID3D12DescriptorHeap *heap[] = {rhi_g_cbv_srv_uav_heap.get(), rhi_g_sampler_heap.get()};
            command_list->SetDescriptorHeaps(2, heap);

            command_list->SetGraphicsRootSignature(rhi_pl_g_geometry.root_signature.get());

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = rhi_g_viewport_texture->GetResource();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            command_list->ResourceBarrier(1, &barrier);

            D3D12_CPU_DESCRIPTOR_HANDLE viewport_rtv_handle =
                rhi_g_viewport_rtv_heap->GetCPUDescriptorHandleForHeapStart();
            D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = rhi_g_viewport_dsv_heap->GetCPUDescriptorHandleForHeapStart();

            command_list->OMSetRenderTargets(1, &viewport_rtv_handle, FALSE, &dsv_handle);

            command_list->ClearRenderTargetView(viewport_rtv_handle, clear_color, 0, nullptr);
            command_list->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

            command_list->SetPipelineState(rhi_pl_g_geometry.pso.get());
            command_list->RSSetViewports(1, &rhi_g_viewport);
            D3D12_RECT scissorRect = {0, 0, static_cast<UINT>(rhi_g_viewport.Width),
                                      static_cast<UINT>(rhi_g_viewport.Height)};
            command_list->RSSetScissorRects(1, &scissorRect);
            command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


            scene_g_world.each([&](flecs::entity e, transform &t, mesh_component &m) {
                XMFLOAT4X4 mvp;
                XMStoreFloat4x4(&mvp, get_world_transform_matrix(e) * view_proj);
                command_list->SetGraphicsRoot32BitConstants(0, 16, &mvp, 0);


                D3D12_VERTEX_BUFFER_VIEW vbv = {};
                vbv.BufferLocation =
                    rm_get_buffer(rm_get_mesh(m.handle)->vertex_buffer)->resource->GetGPUVirtualAddress();
                vbv.StrideInBytes = sizeof(vertex);
                vbv.SizeInBytes = rm_get_buffer(rm_get_mesh(m.handle)->vertex_buffer)->size;

                D3D12_INDEX_BUFFER_VIEW ibv = {};
                ibv.BufferLocation =
                    rm_get_buffer(rm_get_mesh(m.handle)->index_buffer)->resource->GetGPUVirtualAddress();
                ibv.Format = DXGI_FORMAT_R32_UINT;
                ibv.SizeInBytes = rm_get_buffer(rm_get_mesh(m.handle)->index_buffer)->size;

                command_list->IASetVertexBuffers(0, 1, &vbv);
                command_list->IASetIndexBuffer(&ibv);

                command_list->DrawIndexedInstanced(rm_get_mesh(m.handle)->index_count, 1, 0, 0, 0);
            });

            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = rhi_g_viewport_texture->GetResource();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            command_list->ResourceBarrier(1, &barrier);

            //// Swapchain

            rhi_sw_g_current_backbuffer = rhi_sw_g_swapchain->GetCurrentBackBufferIndex();
            uint8_t &frame_index = rhi_sw_g_current_backbuffer;

            const uint32_t rtv_descriptor_size =
                rhi_g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            D3D12_CPU_DESCRIPTOR_HANDLE swapchain_rtv_Handle =
                rhi_sw_g_swapchain_rtv_heap->GetCPUDescriptorHandleForHeapStart();
            swapchain_rtv_Handle.ptr += frame_index * rtv_descriptor_size;

            barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = rhi_sw_g_render_targets[frame_index].get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            command_list->ResourceBarrier(1, &barrier);

            command_list->OMSetRenderTargets(1, &swapchain_rtv_Handle, FALSE, nullptr);
            command_list->ClearRenderTargetView(swapchain_rtv_Handle, clear_color, 0, nullptr);

            ed_render_backend();

            barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = rhi_sw_g_render_targets[frame_index].get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            command_list->ResourceBarrier(1, &barrier);
        }
        command_list->Close();
    }
}
