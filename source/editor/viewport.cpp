#include "viewport.h"
#include "IconsMaterialSymbols.h"
#include "editor.h"
#include "renderer/renderer.h"
#include <imgui/imgui.h>

#include "common.h"

void ash::ed_vp_init()
{
    SCOPED_CPU_EVENT(L"ash::ed_vp_init");
}

void ash::ed_vp_render()
{
    SCOPED_CPU_EVENT(L"ash::ed_vp_render");
    ed_vp_g_is_focused = false;
    if (!ed_vp_g_is_open)
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    bool is_expanded = ImGui::Begin(ICON_MS_LANDSCAPE " Viewport ###Viewport", &ed_vp_g_is_open);
    if (is_expanded)
    {
        ed_vp_g_is_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        ImVec2 size = ImGui::GetContentRegionAvail();
        int newWidth = size.x < 16 ? 16 : static_cast<int>(size.x);
        int newHeight = size.y < 16 ? 16 : static_cast<int>(size.y);

        static int lastW = 0, lastH = 0;
        if (newWidth != lastW || newHeight != lastH)
        {
            lastW = newWidth;
            lastH = newHeight;

            D3D12_VIEWPORT new_viewport = ash::rhi_g_viewport;
            new_viewport.Height = newHeight;
            new_viewport.Width = newWidth;
            ash::rhi_resize(new_viewport);
        }

        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle =
            ash::rhi_get_cbv_srv_uav_gpu_descriptor(ash::rhi_cbv_srv_uav_viewport_descriptor_index);

        ImGui::Image((ImTextureID)(intptr_t)gpu_handle.ptr, ImVec2(newWidth, newHeight));
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
}
