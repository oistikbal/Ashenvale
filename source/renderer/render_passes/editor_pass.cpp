#include "editor_pass.h"
#include "..\renderer.h"
#include <imgui/imgui_impl_dx12.h>
#include <renderer/core/swapchain.h>

void ash::editor_pass_render(const frame_context &frame_context)
{
    SCOPED_GPU_EVENT(frame_context.cmd, L"ash::editor_pass_render")

    constexpr float clear_color[] = {0.0f, 0.0f, 0.0f, 1.0f};

    D3D12_RESOURCE_BARRIER barrier = {};

    barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = frame_context.swapchain_backbuffer;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    frame_context.cmd->ResourceBarrier(1, &barrier);

    frame_context.cmd->OMSetRenderTargets(1, &frame_context.swapchain_rtv, FALSE, nullptr);
    frame_context.cmd->ClearRenderTargetView(frame_context.swapchain_rtv, clear_color, 0, nullptr);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), frame_context.cmd);

    barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = frame_context.swapchain_backbuffer;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    frame_context.cmd->ResourceBarrier(1, &barrier);
}
