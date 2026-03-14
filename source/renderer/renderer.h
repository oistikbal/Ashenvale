#pragma once

#include "common.h"
#include <atomic>
#include <thread>
#include <D3D12MemAlloc.h>

using namespace winrt;

namespace ash
{
inline std::thread rhi_g_renderer_thread;
inline std::atomic<bool> rhi_g_running = true;

inline constexpr uint32_t rhi_invalid_descriptor_index = UINT32_MAX;
inline constexpr uint32_t rhi_cbv_srv_uav_imgui_descriptor_index = 0;
inline constexpr uint32_t rhi_cbv_srv_uav_viewport_descriptor_index = 1;
inline constexpr uint32_t rhi_cbv_srv_uav_first_dynamic_descriptor_index = 2;

inline winrt::com_ptr<ID3D12DescriptorHeap> rhi_g_cbv_srv_uav_heap;
inline winrt::com_ptr<ID3D12DescriptorHeap> rhi_g_sampler_heap;
inline winrt::com_ptr<ID3D12DescriptorHeap> rhi_g_viewport_rtv_heap;
inline winrt::com_ptr<ID3D12DescriptorHeap> rhi_g_viewport_dsv_heap;
inline winrt::com_ptr<ID3D12DescriptorHeap> rhi_g_rtv_heap;
inline winrt::com_ptr<D3D12MA::Allocation> rhi_g_viewport_texture;
inline winrt::com_ptr<D3D12MA::Allocation> rhi_g_viewport_dsv_buffer;

inline winrt::com_ptr<ID3D12Device> rhi_g_device;
inline winrt::com_ptr<IDXGIAdapter> rhi_g_adapter;
inline winrt::com_ptr<IDXGIOutput> rhi_g_output;
inline winrt::com_ptr<D3D12MA::Allocator> rhi_g_allocator;

inline std::atomic<uint32_t> rhi_g_next_cbv_srv_uav_descriptor = rhi_cbv_srv_uav_first_dynamic_descriptor_index;
inline uint32_t rhi_g_cbv_srv_uav_descriptor_capacity = 0;

inline D3D12_VIEWPORT rhi_g_viewport;
inline DXGI_FORMAT rhi_g_format;
} // namespace ash

namespace ash
{
uint32_t rhi_alloc_cbv_srv_uav_descriptor();
D3D12_CPU_DESCRIPTOR_HANDLE rhi_get_cbv_srv_uav_cpu_descriptor(uint32_t descriptor_index);
D3D12_GPU_DESCRIPTOR_HANDLE rhi_get_cbv_srv_uav_gpu_descriptor(uint32_t descriptor_index);

void rhi_init();
void rhi_render();
void rhi_start();
void rhi_stop();
void rhi_shutdown();
void rhi_resize(D3D12_VIEWPORT viewport);
} // namespace ash
