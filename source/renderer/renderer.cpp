#include "renderer.h"
#include "common.h"
#include "configs/config.h"
#include "editor/console.h"
#include "editor/editor.h"
#include "editor/viewport.h"
#include "pipeline/pipeline.h"
#include "pipeline/shader.h"
#include "pipeline/shader_compiler.h"
#include "render_pass.h"
#include "render_passes/editor_pass.h"
#include "render_passes/object_pass.h"
#include "renderer/core/command_queue.h"
#include "renderer/core/swapchain.h"
#include "scene/camera.h"
#include "scene/scene.h"
#include "window/input.h"
#include "window/window.h"
#include <d3d12sdklayers.h>
#include <dxgidebug.h>
#include <filesystem>
#include <format>
#include <string>

using namespace winrt;

namespace
{
#if _DEBUG
com_ptr<ID3D12InfoQueue> g_d3d12_info_queue;
com_ptr<IDXGIInfoQueue> g_dxgi_info_queue;
UINT64 g_d3d12_last_msg_index = 0;
UINT64 g_dxgi_last_msg_index = 0;
#endif

#if _DEBUG
ash::ed_console_log_level to_log_level(D3D12_MESSAGE_SEVERITY severity)
{
    switch (severity)
    {
    case D3D12_MESSAGE_SEVERITY_CORRUPTION:
    case D3D12_MESSAGE_SEVERITY_ERROR:
        return ash::ed_console_log_level::error;
    case D3D12_MESSAGE_SEVERITY_WARNING:
        return ash::ed_console_log_level::warning;
    case D3D12_MESSAGE_SEVERITY_INFO:
    case D3D12_MESSAGE_SEVERITY_MESSAGE:
    default:
        return ash::ed_console_log_level::info;
    }
}

ash::ed_console_log_level to_log_level(DXGI_INFO_QUEUE_MESSAGE_SEVERITY severity)
{
    switch (severity)
    {
    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR:
    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION:
        return ash::ed_console_log_level::error;
    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING:
        return ash::ed_console_log_level::warning;
    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO:
    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_MESSAGE:
    default:
        return ash::ed_console_log_level::info;
    }
}

void drain_debug_messages()
{
    constexpr UINT64 max_messages_per_frame = 64;

    if (g_d3d12_info_queue)
    {
        UINT64 message_count = g_d3d12_info_queue->GetNumStoredMessagesAllowedByRetrievalFilter();
        UINT64 end = (message_count > (g_d3d12_last_msg_index + max_messages_per_frame))
                         ? (g_d3d12_last_msg_index + max_messages_per_frame)
                         : message_count;

        for (UINT64 i = g_d3d12_last_msg_index; i < end; ++i)
        {
            SIZE_T len = 0;
            if (FAILED(g_d3d12_info_queue->GetMessage(i, nullptr, &len)) || len == 0)
            {
                continue;
            }

            std::string buffer(len, '\0');
            auto *msg = reinterpret_cast<D3D12_MESSAGE *>(buffer.data());
            if (SUCCEEDED(g_d3d12_info_queue->GetMessage(i, msg, &len)) && msg->pDescription)
            {
                ash::ed_console_log(to_log_level(msg->Severity), std::format("[D3D12] {}", msg->pDescription));
            }
        }

        g_d3d12_last_msg_index = end;
    }

    if (g_dxgi_info_queue)
    {
        UINT64 message_count = g_dxgi_info_queue->GetNumStoredMessages(DXGI_DEBUG_ALL);
        UINT64 end = (message_count > (g_dxgi_last_msg_index + max_messages_per_frame))
                         ? (g_dxgi_last_msg_index + max_messages_per_frame)
                         : message_count;

        for (UINT64 i = g_dxgi_last_msg_index; i < end; ++i)
        {
            SIZE_T len = 0;
            if (FAILED(g_dxgi_info_queue->GetMessage(DXGI_DEBUG_ALL, i, nullptr, &len)) || len == 0)
            {
                continue;
            }

            std::string buffer(len, '\0');
            auto *msg = reinterpret_cast<DXGI_INFO_QUEUE_MESSAGE *>(buffer.data());
            if (SUCCEEDED(g_dxgi_info_queue->GetMessage(DXGI_DEBUG_ALL, i, msg, &len)) && msg->pDescription)
            {
                ash::ed_console_log(to_log_level(msg->Severity), std::format("[DXGI] {}", msg->pDescription));
            }
        }

        g_dxgi_last_msg_index = end;
    }
}
#endif

void init_device()
{
    ash::ed_console_log(ash::ed_console_log_level::info, "[RHI] Device initialization begin.");

#if _DEBUG
    com_ptr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }

    com_ptr<IDXGIInfoQueue> dxgiInfoQueue;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
    {
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
        g_dxgi_info_queue = dxgiInfoQueue;
        g_dxgi_last_msg_index = g_dxgi_info_queue->GetNumStoredMessages(DXGI_DEBUG_ALL);
    }
#endif

    com_ptr<IDXGIFactory7> factory;
    HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
    if (FAILED(hr))
    {
        ash::ed_console_log(ash::ed_console_log_level::error, "[RHI] CreateDXGIFactory2 failed.");
        return;
    }

    hr = factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                             IID_PPV_ARGS(ash::rhi_g_adapter.put()));
    if (FAILED(hr))
    {
        ash::ed_console_log(ash::ed_console_log_level::error, "[RHI] Adapter selection failed.");
        return;
    }

    hr = D3D12CreateDevice(ash::rhi_g_adapter.get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(ash::rhi_g_device.put()));
    if (FAILED(hr))
    {
        ash::ed_console_log(ash::ed_console_log_level::error, "[RHI] D3D12CreateDevice failed.");
    }
    assert(ash::rhi_g_device.get());

#if _DEBUG
    ash::rhi_g_device.as(g_d3d12_info_queue);
    if (g_d3d12_info_queue)
    {
        g_d3d12_last_msg_index = g_d3d12_info_queue->GetNumStoredMessagesAllowedByRetrievalFilter();
    }
#endif

    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    ash::rhi_g_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));

    const char *feature_level = "Unknown";
    D3D12_FEATURE_DATA_FEATURE_LEVELS fl = {};
    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
    };
    fl.NumFeatureLevels = _countof(levels);
    fl.pFeatureLevelsRequested = levels;
    if (SUCCEEDED(ash::rhi_g_device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &fl, sizeof(fl))))
    {
        switch (fl.MaxSupportedFeatureLevel)
        {
        case D3D_FEATURE_LEVEL_12_2:
            feature_level = "12_2";
            break;
        case D3D_FEATURE_LEVEL_12_1:
            feature_level = "12_1";
            break;
        case D3D_FEATURE_LEVEL_12_0:
            feature_level = "12_0";
            break;
        default:
            break;
        }
    }

    switch (options.ResourceBindingTier)
    {
    case D3D12_RESOURCE_BINDING_TIER_1:
        MessageBox(ash::win_g_hwnd, "Tier 1 GPU is not supported.", 0, MB_ICONWARNING);
        ExitProcess(0);
        break;
    case D3D12_RESOURCE_BINDING_TIER_2:
        MessageBox(ash::win_g_hwnd, "Tier 2 GPU is not supported.", 0, MB_ICONWARNING);
        ExitProcess(0);
        break;
    case D3D12_RESOURCE_BINDING_TIER_3:
        break;
    }

    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {D3D_SHADER_MODEL_6_6};
    if (SUCCEEDED(
            ash::rhi_g_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))))
    {
        if (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_6)
        {
            MessageBox(ash::win_g_hwnd, "Shader Model 6.6 is required.", 0, MB_ICONWARNING);
            ExitProcess(0);
        }
    }

    const char *shader_model_str = "unknown";
    switch (shaderModel.HighestShaderModel)
    {
    case D3D_SHADER_MODEL_6_6:
        shader_model_str = "6_6";
        break;
    case D3D_SHADER_MODEL_6_7:
        shader_model_str = "6_7";
        break;
    case D3D_SHADER_MODEL_6_8:
        shader_model_str = "6_8";
        break;
    default:
        break;
    }

    ash::ed_console_log(ash::ed_console_log_level::info,
                        std::format("[RHI] DX12 feature level={} shader model={} agility sdk={}.", feature_level,
                                    shader_model_str, D3D12_SDK_VERSION));

    ash::rhi_g_adapter->EnumOutputs(0, ash::rhi_g_output.put());
    assert(ash::rhi_g_output.get());

    D3D12MA::ALLOCATOR_DESC allocator_desc = {};
    allocator_desc.pDevice = ash::rhi_g_device.get();
    allocator_desc.pAdapter = ash::rhi_g_adapter.get();
    D3D12MA::CreateAllocator(&allocator_desc, ash::rhi_g_allocator.put());

    assert(ash::rhi_g_allocator.get());
    ash::ed_console_log(ash::ed_console_log_level::info, "[RHI] Device initialization complete.");
}

void handle_window_events()
{
    SCOPED_CPU_EVENT(L"ash::rhi_handle_window_events")

    ash::win_evt_swap_buffers(ash::win_g_queue);
    auto &q = ash::win_evt_get_back_buffer(ash::win_g_queue);

    if (q.empty())
        return;

    ash::win_evt_event e;

    bool resize = false;

    while (!q.empty())
    {
        e = std::move(q.front());
        q.pop();

        switch (e.type)
        {
        case ash::win_evt_windows_event::resize:
            resize = true;
            break;
        default:
            break;
        }
    }

    if (resize)
        ash::rhi_sw_resize();
}
} // namespace

void ash::rhi_start()
{
    rhi_g_renderer_thread = std::thread(rhi_render);
    HANDLE hThread = static_cast<HANDLE>(rhi_g_renderer_thread.native_handle());
    SetThreadDescription(hThread, L"Renderer Thread");
    SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);
    ed_console_log(ed_console_log_level::info, "[RHI] Renderer thread started.");
}

void ash::rhi_init()
{
    SCOPED_CPU_EVENT(L"ash::rhi_init");
    ed_console_log(ed_console_log_level::info, "[RHI] Initialization begin.");

    init_device();

    rhi_cmd_init();
    rhi_sw_init();
    rhi_sc_init();
    rhi_sh_init();
    rhi_pl_init();

    rhi_g_viewport = {};
    rhi_g_viewport.Width = rhi_sw_g_viewport.Width;
    rhi_g_viewport.Height = rhi_sw_g_viewport.Height;
    rhi_g_viewport.MinDepth = 0.0f;
    rhi_g_viewport.MaxDepth = 1.0f;
    rhi_g_viewport.TopLeftX = 0.0f;
    rhi_g_viewport.TopLeftY = 0.0f;

    D3D12_DESCRIPTOR_HEAP_DESC viewport_rtv_heap_desc = {};
    viewport_rtv_heap_desc.NumDescriptors = 1;
    viewport_rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    viewport_rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rhi_g_device->CreateDescriptorHeap(&viewport_rtv_heap_desc, IID_PPV_ARGS(rhi_g_viewport_rtv_heap.put()));

    assert(rhi_g_viewport_rtv_heap.get());
    SET_OBJECT_NAME(rhi_g_viewport_rtv_heap.get(), L"Viewport Rtv Desc Heap");

    D3D12_DESCRIPTOR_HEAP_DESC viewport_dsv_heap_desc = {};
    viewport_dsv_heap_desc.NumDescriptors = 1;
    viewport_dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    viewport_dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rhi_g_device->CreateDescriptorHeap(&viewport_dsv_heap_desc, IID_PPV_ARGS(rhi_g_viewport_dsv_heap.put()));

    assert(rhi_g_viewport_dsv_heap.get());
    SET_OBJECT_NAME(rhi_g_viewport_dsv_heap.get(), L"Viewport Dsv Desc Heap");

    D3D12_DESCRIPTOR_HEAP_DESC cbv_srv_uav_heap_desc = {};
    cbv_srv_uav_heap_desc.NumDescriptors = 1000000;
    cbv_srv_uav_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbv_srv_uav_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    rhi_g_device->CreateDescriptorHeap(&cbv_srv_uav_heap_desc, IID_PPV_ARGS(rhi_g_cbv_srv_uav_heap.put()));

    assert(rhi_g_cbv_srv_uav_heap.get());
    SET_OBJECT_NAME(rhi_g_cbv_srv_uav_heap.get(), L"CBV SRV UAV Desc Heap");
    rhi_g_cbv_srv_uav_descriptor_capacity = cbv_srv_uav_heap_desc.NumDescriptors;
    rhi_g_next_cbv_srv_uav_descriptor.store(rhi_cbv_srv_uav_first_dynamic_descriptor_index, std::memory_order_relaxed);

    D3D12_DESCRIPTOR_HEAP_DESC sampler_heap_desc = {};
    sampler_heap_desc.NumDescriptors = 2048;
    sampler_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    sampler_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    rhi_g_device->CreateDescriptorHeap(&sampler_heap_desc, IID_PPV_ARGS(rhi_g_sampler_heap.put()));

    assert(rhi_g_sampler_heap.get());
    SET_OBJECT_NAME(rhi_g_sampler_heap.get(), L"Sampler Desc Heap");

    D3D12_SAMPLER_DESC default_sampler_desc = {};
    default_sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    default_sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    default_sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    default_sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    default_sampler_desc.MipLODBias = 0.0f;
    default_sampler_desc.MaxAnisotropy = 1;
    default_sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NONE;
    default_sampler_desc.BorderColor[0] = 0.0f;
    default_sampler_desc.BorderColor[1] = 0.0f;
    default_sampler_desc.BorderColor[2] = 0.0f;
    default_sampler_desc.BorderColor[3] = 0.0f;
    default_sampler_desc.MinLOD = 0.0f;
    default_sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
    rhi_g_device->CreateSampler(&default_sampler_desc, rhi_g_sampler_heap->GetCPUDescriptorHandleForHeapStart());

    ed_console_log(ed_console_log_level::info, "[RHI] Descriptor heaps created.");

    rhi_resize(rhi_g_viewport);

    g_camera.position = DirectX::XMFLOAT3(0, 0, -1.0f);
    g_camera.rotation = DirectX::XMFLOAT3(0, 0, 0.0f);
    ed_console_log(ed_console_log_level::info, "[RHI] Initialization complete.");
}

void ash::rhi_render()
{
    auto last_time = std::chrono::high_resolution_clock::now();
    frame_context frame_context = {};
    while (rhi_g_running.load(std::memory_order_relaxed))
    {
        SCOPED_CPU_EVENT(L"ash::rhi_render")

        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> delta_time = now - last_time;
        last_time = now;

        frame_context = {};

        handle_window_events();
        ed_render();

        scene_update();
        scene_data scene_data = get_scene_data(scene_g_world, g_camera);

        rhi_sw_g_current_backbuffer = rhi_sw_g_swapchain->GetCurrentBackBufferIndex();
        uint8_t &frame_index = rhi_sw_g_current_backbuffer;

        const uint32_t rtv_descriptor_size =
            rhi_g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE swapchain_rtv_handle =
            rhi_sw_g_swapchain_rtv_heap->GetCPUDescriptorHandleForHeapStart();
        swapchain_rtv_handle.ptr += frame_index * rtv_descriptor_size;

        frame_context.cmd = rhi_cmd_g_command_list.get();
        frame_context.swapchain_backbuffer = rhi_sw_g_render_targets[frame_index].get();
        frame_context.swapchain_rtv = swapchain_rtv_handle;

        frame_context.viewport = rhi_g_viewport;
        frame_context.viewport_rtv = rhi_g_viewport_rtv_heap->GetCPUDescriptorHandleForHeapStart();
        frame_context.viewport_dsv_rtv = rhi_g_viewport_dsv_heap->GetCPUDescriptorHandleForHeapStart();
        frame_context.viewport_texture = rhi_g_viewport_texture->GetResource();

        const auto &input_state = win_input_acquire_front_buffer(ash::g_win_input);
        if (ed_vp_g_is_focused)
        {
            cam_handle_input(g_camera, delta_time.count(), input_state);
        }
        win_input_release_front_buffer(ash::g_win_input);

        object_pass_render(frame_context, scene_data);
        editor_pass_render(frame_context);

        rhi_cmd_g_command_list->Close();
        ID3D12CommandList *cmdLists[] = {rhi_cmd_g_command_list.get()};
        rhi_cmd_g_direct->ExecuteCommandLists(1, cmdLists);
        HRESULT present_hr = rhi_sw_g_swapchain->Present(1, 0);
        if (FAILED(present_hr))
        {
            ed_console_log(ed_console_log_level::error, "[Frame] Swapchain present failed.");
        }
#if _DEBUG
        drain_debug_messages();
#endif

        rhi_sw_g_fence_value++;
        rhi_cmd_g_direct->Signal(rhi_sw_g_fence.get(), rhi_sw_g_fence_value);

        if (rhi_sw_g_fence->GetCompletedValue() < rhi_sw_g_fence_value)
        {
            rhi_sw_g_fence->SetEventOnCompletion(rhi_sw_g_fence_value, rhi_sw_g_fence_event);
            WaitForSingleObject(rhi_sw_g_fence_event, INFINITE);
        }

        rhi_cmd_g_command_allocator->Reset();
        rhi_cmd_g_command_list->Reset(rhi_cmd_g_command_allocator.get(), nullptr);
    }
}

void ash::rhi_stop()
{
    SCOPED_CPU_EVENT(L"ash::rhi_stop")
    ed_console_log(ed_console_log_level::info, "[RHI] Stop requested.");
    rhi_g_running = false;
    if (rhi_g_renderer_thread.joinable())
    {
        rhi_g_renderer_thread.join();
    }
    ed_console_log(ed_console_log_level::info, "[RHI] Renderer thread stopped.");
}

void ash::rhi_shutdown()
{
    SCOPED_CPU_EVENT(L"ash::rhi_shutdown")
    ed_console_log(ed_console_log_level::info, "[RHI] Shutdown begin.");

    rhi_g_viewport_texture = nullptr;
    rhi_g_viewport_dsv_buffer = nullptr;

    for (UINT i = 0; i < 2; ++i)
    {
        rhi_sw_g_render_targets[i] = nullptr;
    }

    rhi_cmd_g_command_list = nullptr;
    rhi_cmd_g_command_allocator = nullptr;
    rhi_cmd_g_copy = nullptr;
    rhi_cmd_g_compute = nullptr;
    rhi_cmd_g_direct = nullptr;

    rhi_g_cbv_srv_uav_heap = nullptr;
    rhi_g_cbv_srv_uav_descriptor_capacity = 0;
    rhi_g_next_cbv_srv_uav_descriptor.store(rhi_cbv_srv_uav_first_dynamic_descriptor_index, std::memory_order_relaxed);
    rhi_g_sampler_heap = nullptr;
    rhi_g_viewport_rtv_heap = nullptr;
    rhi_g_viewport_dsv_heap = nullptr;
    rhi_g_rtv_heap = nullptr;

    rhi_sw_g_swapchain_rtv_heap = nullptr;
    rhi_sw_g_swapchain = nullptr;
    rhi_sw_g_fence = nullptr;

    if (rhi_sw_g_fence_event)
    {
        CloseHandle(rhi_sw_g_fence_event);
        rhi_sw_g_fence_event = nullptr;
    }

    rhi_sc_g_compiler = nullptr;
    rhi_sc_g_utils = nullptr;

    g_d3d12_info_queue = nullptr;
    g_dxgi_info_queue = nullptr;

    rhi_g_allocator = nullptr;
    rhi_g_output = nullptr;
    rhi_g_adapter = nullptr;
    rhi_g_device = nullptr;
    ed_console_log(ed_console_log_level::info, "[RHI] Shutdown complete.");
}

void ash::rhi_resize(D3D12_VIEWPORT viewport)
{
    SCOPED_CPU_EVENT(L"ash::rhi_resize")
    ed_console_log(ed_console_log_level::info, "[RHI] Viewport resource resize.");

    rhi_g_viewport = viewport;

    D3D12_RESOURCE_DESC tex_desc = {};
    tex_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    tex_desc.Alignment = 0;
    tex_desc.Width = static_cast<UINT64>(viewport.Width);
    tex_desc.Height = static_cast<UINT>(viewport.Height);
    tex_desc.DepthOrArraySize = 1;
    tex_desc.MipLevels = 1;
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE optimized_clear_value = {};
    optimized_clear_value.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    optimized_clear_value.Color[0] = 0.0f;
    optimized_clear_value.Color[1] = 0.0f;
    optimized_clear_value.Color[2] = 0.0f;
    optimized_clear_value.Color[3] = 1.0f;

    D3D12MA::ALLOCATION_DESC alloc_desc = {};
    alloc_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    rhi_g_allocator->CreateResource(&alloc_desc, &tex_desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                    &optimized_clear_value, rhi_g_viewport_texture.put(), IID_NULL, nullptr);
    assert(rhi_g_viewport_texture.get());

    SET_OBJECT_NAME(rhi_g_viewport_texture.get(), L"Viewport Texture");

    D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.Format = tex_desc.Format;
    rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtv_desc.Texture2D.MipSlice = 0;

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rhi_g_viewport_rtv_heap->GetCPUDescriptorHandleForHeapStart();

    rhi_g_device->CreateRenderTargetView(rhi_g_viewport_texture->GetResource(), &rtv_desc, rtv_handle);

    D3D12_CLEAR_VALUE depth_optimized_clear_value = {};
    depth_optimized_clear_value.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_optimized_clear_value.DepthStencil.Depth = 1.0f;
    depth_optimized_clear_value.DepthStencil.Stencil = 0;

    D3D12_RESOURCE_DESC depth_desc = {};
    depth_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depth_desc.Alignment = 0;
    depth_desc.Width = static_cast<UINT64>(viewport.Width);
    depth_desc.Height = static_cast<UINT>(viewport.Height);
    depth_desc.DepthOrArraySize = 1;
    depth_desc.MipLevels = 1;
    depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_desc.SampleDesc.Count = 1;
    depth_desc.SampleDesc.Quality = 0;
    depth_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depth_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    rhi_g_allocator->CreateResource(&alloc_desc, &depth_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                    &depth_optimized_clear_value, rhi_g_viewport_dsv_buffer.put(), IID_NULL, nullptr);
    assert(rhi_g_viewport_dsv_buffer.get());
    SET_OBJECT_NAME(rhi_g_viewport_dsv_buffer.get(), L"Viewport Dsv Buffer");

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_view_desc = {};
    dsv_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv_view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv_view_desc.Flags = D3D12_DSV_FLAG_NONE;

    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = rhi_g_viewport_dsv_heap->GetCPUDescriptorHandleForHeapStart();
    rhi_g_device->CreateDepthStencilView(rhi_g_viewport_dsv_buffer->GetResource(), &dsv_view_desc, dsv_handle);

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle =
        rhi_get_cbv_srv_uav_cpu_descriptor(rhi_cbv_srv_uav_viewport_descriptor_index);

    rhi_g_device->CreateShaderResourceView(rhi_g_viewport_texture->GetResource(), &srv_desc, cpu_handle);
}

uint32_t ash::rhi_alloc_cbv_srv_uav_descriptor()
{
    uint32_t current = rhi_g_next_cbv_srv_uav_descriptor.load(std::memory_order_relaxed);
    while (true)
    {
        if (current >= rhi_g_cbv_srv_uav_descriptor_capacity)
        {
            ed_console_log(ed_console_log_level::error, "[RHI] Descriptor heap exhausted.");
            return rhi_invalid_descriptor_index;
        }

        const uint32_t next = current + 1;
        if (rhi_g_next_cbv_srv_uav_descriptor.compare_exchange_weak(current, next, std::memory_order_relaxed))
        {
            return current;
        }
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE ash::rhi_get_cbv_srv_uav_cpu_descriptor(uint32_t descriptor_index)
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = rhi_g_cbv_srv_uav_heap->GetCPUDescriptorHandleForHeapStart();
    const uint32_t descriptor_size = rhi_g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    handle.ptr += static_cast<SIZE_T>(descriptor_size) * descriptor_index;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE ash::rhi_get_cbv_srv_uav_gpu_descriptor(uint32_t descriptor_index)
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = rhi_g_cbv_srv_uav_heap->GetGPUDescriptorHandleForHeapStart();
    const uint32_t descriptor_size = rhi_g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    handle.ptr += static_cast<UINT64>(descriptor_size) * descriptor_index;
    return handle;
}
