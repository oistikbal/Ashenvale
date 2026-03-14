#include "resource_manager.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <renderer/core/command_queue.h>
#include <renderer/renderer.h>
#include <unordered_map>
#include <vector>

namespace
{
std::unordered_map<ash::uuid, ash::resource> g_resources;

std::vector<ash::resource_buffer> g_buffers;
std::vector<ash::resource_texture> g_textures;
std::vector<ash::resource_mesh> g_meshes;

template <typename T> uint32_t pool_add(std::vector<T> &pool, T &&value)
{
    pool.push_back(std::move(value));
    return static_cast<uint32_t>(pool.size());
}

template <typename T> T *pool_get(std::vector<T> &pool, uint32_t handle)
{
    if (handle == 0)
        return nullptr;

    size_t index = static_cast<size_t>(handle - 1);
    if (index >= pool.size())
        return nullptr;

    return &pool[index];
}

ash::resource register_resource(ash::resource_type type, ash::resource_handle handle)
{
    ash::resource out = {};
    out.id = ash::uuid_generate_random();
    out.type = type;
    out.loaded = true;
    out.handle = handle;
    g_resources[out.id] = out;
    return out;
}

void upload_buffer_data(ID3D12Resource *resource, const void *initial_data, uint64_t initial_data_size)
{
    if (!resource || !initial_data || initial_data_size == 0)
        return;

    void *mapped = nullptr;
    const HRESULT hr = resource->Map(0, nullptr, &mapped);
    assert(SUCCEEDED(hr));
    std::memcpy(mapped, initial_data, static_cast<size_t>(initial_data_size));
    resource->Unmap(0, nullptr);
}

uint32_t align_up(uint32_t value, uint32_t alignment)
{
    return (value + (alignment - 1)) & ~(alignment - 1);
}

void upload_texture_rgba8_data(ID3D12Resource *texture_resource, uint32_t width, uint32_t height, uint16_t mip_levels,
                               const void *data, uint64_t size, D3D12_RESOURCE_STATES final_state)
{
    if (!texture_resource || !data || width == 0 || height == 0 || mip_levels == 0)
    {
        return;
    }

    std::vector<uint32_t> mip_widths(mip_levels);
    std::vector<uint32_t> mip_heights(mip_levels);
    std::vector<uint32_t> src_row_pitches(mip_levels);
    std::vector<uint32_t> upload_row_pitches(mip_levels);
    std::vector<uint64_t> src_offsets(mip_levels);
    std::vector<uint64_t> upload_offsets(mip_levels);

    uint32_t level_width = width;
    uint32_t level_height = height;
    uint64_t required_size = 0;
    uint64_t upload_buffer_size = 0;

    for (uint16_t mip = 0; mip < mip_levels; ++mip)
    {
        mip_widths[mip] = level_width;
        mip_heights[mip] = level_height;
        src_row_pitches[mip] = level_width * 4;
        upload_row_pitches[mip] = align_up(src_row_pitches[mip], D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

        src_offsets[mip] = required_size;
        upload_offsets[mip] = upload_buffer_size;

        required_size += static_cast<uint64_t>(src_row_pitches[mip]) * level_height;
        upload_buffer_size += static_cast<uint64_t>(upload_row_pitches[mip]) * level_height;

        level_width = max(1u, level_width / 2);
        level_height = max(1u, level_height / 2);
    }

    if (size < required_size)
    {
        assert(false && "Texture upload failed: source size is smaller than packed mip chain size.");
        return;
    }

    D3D12_RESOURCE_DESC upload_desc = {};
    upload_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    upload_desc.Alignment = 0;
    upload_desc.Width = upload_buffer_size;
    upload_desc.Height = 1;
    upload_desc.DepthOrArraySize = 1;
    upload_desc.MipLevels = 1;
    upload_desc.Format = DXGI_FORMAT_UNKNOWN;
    upload_desc.SampleDesc.Count = 1;
    upload_desc.SampleDesc.Quality = 0;
    upload_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    upload_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12MA::ALLOCATION_DESC upload_alloc_desc = {};
    upload_alloc_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

    winrt::com_ptr<D3D12MA::Allocation> upload_allocation;
    winrt::com_ptr<ID3D12Resource> upload_resource;
    HRESULT hr = ash::rhi_g_allocator->CreateResource(&upload_alloc_desc, &upload_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                 upload_allocation.put(), IID_PPV_ARGS(upload_resource.put()));
    assert(SUCCEEDED(hr));

    std::vector<std::uint8_t> upload_data(static_cast<size_t>(upload_buffer_size));
    const auto *src_bytes = static_cast<const std::uint8_t *>(data);
    for (uint16_t mip = 0; mip < mip_levels; ++mip)
    {
        for (uint32_t y = 0; y < mip_heights[mip]; ++y)
        {
            std::memcpy(upload_data.data() + static_cast<size_t>(upload_offsets[mip]) +
                            static_cast<size_t>(y) * upload_row_pitches[mip],
                        src_bytes + static_cast<size_t>(src_offsets[mip]) + static_cast<size_t>(y) * src_row_pitches[mip],
                        src_row_pitches[mip]);
        }
    }

    upload_buffer_data(upload_resource.get(), upload_data.data(), upload_buffer_size);

    winrt::com_ptr<ID3D12CommandAllocator> cmd_allocator;
    winrt::com_ptr<ID3D12GraphicsCommandList> cmd_list;
    hr = ash::rhi_g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(cmd_allocator.put()));
    assert(SUCCEEDED(hr));

    hr = ash::rhi_g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_allocator.get(), nullptr,
                                              IID_PPV_ARGS(cmd_list.put()));
    assert(SUCCEEDED(hr));

    for (uint16_t mip = 0; mip < mip_levels; ++mip)
    {
        D3D12_TEXTURE_COPY_LOCATION dst_location = {};
        dst_location.pResource = texture_resource;
        dst_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst_location.SubresourceIndex = mip;

        D3D12_TEXTURE_COPY_LOCATION src_location = {};
        src_location.pResource = upload_resource.get();
        src_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src_location.PlacedFootprint.Offset = upload_offsets[mip];
        src_location.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        src_location.PlacedFootprint.Footprint.Width = mip_widths[mip];
        src_location.PlacedFootprint.Footprint.Height = mip_heights[mip];
        src_location.PlacedFootprint.Footprint.Depth = 1;
        src_location.PlacedFootprint.Footprint.RowPitch = upload_row_pitches[mip];

        cmd_list->CopyTextureRegion(&dst_location, 0, 0, 0, &src_location, nullptr);
    }

    if (final_state != D3D12_RESOURCE_STATE_COPY_DEST)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = texture_resource;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = final_state;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd_list->ResourceBarrier(1, &barrier);
    }

    hr = cmd_list->Close();
    assert(SUCCEEDED(hr));

    ID3D12CommandList *lists[] = {cmd_list.get()};
    ash::rhi_cmd_g_direct->ExecuteCommandLists(1, lists);

    winrt::com_ptr<ID3D12Fence> fence;
    hr = ash::rhi_g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.put()));
    assert(SUCCEEDED(hr));

    HANDLE fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    assert(fence_event != nullptr);

    constexpr uint64_t fence_value = 1;
    hr = ash::rhi_cmd_g_direct->Signal(fence.get(), fence_value);
    assert(SUCCEEDED(hr));

    if (fence->GetCompletedValue() < fence_value)
    {
        hr = fence->SetEventOnCompletion(fence_value, fence_event);
        assert(SUCCEEDED(hr));
        WaitForSingleObject(fence_event, INFINITE);
    }

    CloseHandle(fence_event);
}
} // namespace

namespace ash
{
void rm_init()
{
    g_buffers.reserve(512);
    g_textures.reserve(64);
    g_meshes.reserve(64);
}

void rm_shutdown()
{
    g_resources.clear();
    g_buffers.clear();
    g_textures.clear();
    g_meshes.clear();
}

resource *rm_find(const uuid &id)
{
    auto it = g_resources.find(id);
    if (it == g_resources.end())
        return nullptr;

    return &it->second;
}

resource rm_create_buffer(const D3D12_RESOURCE_DESC &resource_desc, const D3D12MA::ALLOCATION_DESC &allocation_desc,
                          const D3D12_RESOURCE_STATES resource_state, const void *initial_data,
                          uint64_t initial_data_size, uint32_t stride)
{
    resource_buffer buffer = {};
    buffer.size =
        initial_data_size != 0 ? static_cast<uint32_t>(initial_data_size) : static_cast<uint32_t>(resource_desc.Width);
    buffer.stride = 0;

    const HRESULT hr = rhi_g_allocator->CreateResource(&allocation_desc, &resource_desc, resource_state, nullptr,
                                                       buffer.allocation.put(), IID_PPV_ARGS(buffer.resource.put()));
    assert(SUCCEEDED(hr));

    upload_buffer_data(buffer.allocation->GetResource(), initial_data, initial_data_size);

    const resource_handle handle = pool_add(g_buffers, std::move(buffer));
    return register_resource(resource_type::buffer, handle);
}

resource rm_create_texture(const D3D12_RESOURCE_DESC &resource_desc, const D3D12MA::ALLOCATION_DESC &allocation_desc,
                           const D3D12_RESOURCE_STATES resource_state, const void *data, const uint64_t size)
{
    resource_texture texture = {};
    texture.width = static_cast<uint32_t>(resource_desc.Width);
    texture.height = resource_desc.Height;
    texture.format = resource_desc.Format;

    const bool has_initial_data = (data != nullptr && size != 0);
    const D3D12_RESOURCE_STATES initial_state = has_initial_data ? D3D12_RESOURCE_STATE_COPY_DEST : resource_state;
    const HRESULT hr = rhi_g_allocator->CreateResource(&allocation_desc, &resource_desc, initial_state, nullptr,
                                                       texture.allocation.put(), IID_PPV_ARGS(texture.resource.put()));
    assert(SUCCEEDED(hr));

    if (has_initial_data)
    {
        const uint16_t mip_levels = (resource_desc.MipLevels == 0) ? 1 : resource_desc.MipLevels;
        assert(resource_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM);
        upload_texture_rgba8_data(texture.resource.get(), texture.width, texture.height, mip_levels, data, size,
                                  resource_state);
    }

    texture.srv_descriptor_index = rhi_alloc_cbv_srv_uav_descriptor();
    assert(texture.srv_descriptor_index != rhi_invalid_descriptor_index);

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Format = resource_desc.Format;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = (resource_desc.MipLevels == 0) ? 1 : resource_desc.MipLevels;
    srv_desc.Texture2D.PlaneSlice = 0;
    srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = rhi_get_cbv_srv_uav_cpu_descriptor(texture.srv_descriptor_index);
    rhi_g_device->CreateShaderResourceView(texture.resource.get(), &srv_desc, cpu_handle);

    const resource_handle handle = pool_add(g_textures, std::move(texture));
    return register_resource(resource_type::texture_2d, handle);
}

resource rm_create_mesh(resource_handle vertex_buffer, resource_handle index_buffer, uint32_t index_count)
{
    assert(pool_get(g_buffers, vertex_buffer) != nullptr);
    assert(pool_get(g_buffers, index_buffer) != nullptr);

    resource_mesh mesh = {};
    mesh.vertex_buffer = vertex_buffer;
    mesh.index_buffer = index_buffer;
    mesh.index_count = index_count;

    const resource_handle handle = pool_add(g_meshes, std::move(mesh));
    return register_resource(resource_type::mesh, handle);
}
resource_mesh *rm_get_mesh(resource_handle handle)
{
    return pool_get(g_meshes, handle);
}
resource_texture *rm_get_texture(resource_handle handle)
{
    return pool_get(g_textures, handle);
}
resource_buffer *rm_get_buffer(resource_handle handle)
{
    return pool_get(g_buffers, handle);
}
} // namespace ash
