#pragma once

#include "uuid.h"
#include <D3D12MemAlloc.h>
#include <variant>
#include <winrt/base.h>

namespace ash
{
using resource_handle = uint32_t;
static constexpr resource_handle invalid_resource_handle = 0;

enum class resource_type
{
    none,
    buffer,
    texture_2d,
    mesh,
};

struct resource
{
    uuid id = {};
    resource_type type = resource_type::none;
    bool loaded = false;
    resource_handle handle = invalid_resource_handle;
};

struct resource_buffer
{
    winrt::com_ptr<D3D12MA::Allocation> allocation;
    winrt::com_ptr<ID3D12Resource> resource;

    uint32_t size = 0;
    uint32_t stride = 0;
};

struct resource_texture
{
    winrt::com_ptr<D3D12MA::Allocation> allocation;
    winrt::com_ptr<ID3D12Resource> resource;

    uint32_t width = 0;
    uint32_t height = 0;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
};

struct resource_mesh
{
    resource_handle vertex_buffer = {};
    resource_handle index_buffer = {};
    uint32_t index_count = 0;
};

void rm_init();
void rm_shutdown();

resource *rm_find(const uuid &id);
resource rm_create_buffer(const D3D12_RESOURCE_DESC &resource_desc, const D3D12MA::ALLOCATION_DESC &allocation_desc,
                          const D3D12_RESOURCE_STATES resource_state, const void *initial_data = nullptr,
                          uint64_t initial_data_size = 0, uint32_t stride = 0);

resource rm_create_texture(const D3D12_RESOURCE_DESC &resource_desc, const D3D12MA::ALLOCATION_DESC &allocation_desc,
                           const D3D12_RESOURCE_STATES resource_state, const void *data, const uint64_t size);

resource rm_create_mesh(resource_handle vertex_buffer, resource_handle index_buffer, uint32_t index_count);

resource_mesh *rm_get_mesh(resource_handle handle);
resource_texture *rm_get_texture(resource_handle handle);
resource_buffer *rm_get_buffer(resource_handle handle);
} // namespace ash