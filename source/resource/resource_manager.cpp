#include "resource_manager.h"

#include <cassert>
#include <cstring>
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
} // namespace

namespace ash
{
void rm_init()
{
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

    const HRESULT hr = rhi_g_allocator->CreateResource(&allocation_desc, &resource_desc, resource_state, nullptr,
                                                       texture.allocation.put(), IID_PPV_ARGS(texture.resource.put()));
    assert(SUCCEEDED(hr));
    assert(data == nullptr || size == 0);

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
} // namespace ash
