#pragma once
#include <d3d12.h>

namespace ash
{
struct frame_context
{
    ID3D12GraphicsCommandList *cmd;
    D3D12_VIEWPORT viewport;
    D3D12_CPU_DESCRIPTOR_HANDLE viewport_rtv;
    D3D12_CPU_DESCRIPTOR_HANDLE viewport_dsv_rtv;
    D3D12_CPU_DESCRIPTOR_HANDLE swapchain_rtv;
    ID3D12Resource *viewport_texture;
    ID3D12Resource *swapchain_backbuffer;
};

} // namespace ash