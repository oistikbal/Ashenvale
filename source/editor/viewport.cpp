#define IMGUI_DEFINE_MATH_OPERATORS

#include "viewport.h"
#include "IconsMaterialSymbols.h"
#include "editor.h"
#include "renderer/renderer.h"
#include <imgui/imgui.h>

#include "scene/component.h"
#include "common.h"
#include "scene/camera.h"
#include "scene/scene.h"
#include <flecs.h>
#include <imgui/ImGuizmo.h>

namespace
{
DirectX::XMMATRIX get_parent_world_matrix(flecs::entity entity)
{
    flecs::entity parent = entity.parent();
    if (!parent.is_valid())
        return DirectX::XMMatrixIdentity();

    return ash::get_world_transform_matrix(parent);
}

void apply_local_transform_from_matrix(flecs::entity entity, const DirectX::XMMATRIX &local_matrix)
{
    using namespace DirectX;

    ash::transform local = entity.get<ash::transform>();

    XMVECTOR scale;
    XMVECTOR rotation;
    XMVECTOR translation;
    XMMatrixDecompose(&scale, &rotation, &translation, local_matrix);

    XMStoreFloat3(&local.position, translation);
    XMStoreFloat4(&local.rotation, XMQuaternionNormalize(rotation));
    XMStoreFloat3(&local.scale, scale);

    entity.set<ash::transform>(local);
}

void draw_transform_gizmo(const ImVec2 &image_min, const ImVec2 &image_size)
{
    using namespace DirectX;

    if (!ash::scene_g_selected.is_valid() || !ash::scene_g_selected.has<ash::transform>())
        return;

    XMFLOAT4X4 view;
    XMFLOAT4X4 proj;
    XMStoreFloat4x4(&view, XMLoadFloat4x4(&ash::g_camera.mat_view));
    XMStoreFloat4x4(&proj, XMLoadFloat4x4(&ash::g_camera.mat_proj));

    XMMATRIX world_matrix = ash::get_world_transform_matrix(ash::scene_g_selected);
    XMFLOAT4X4 world;
    XMStoreFloat4x4(&world, world_matrix);

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(image_min.x, image_min.y, image_size.x, image_size.y);

    static ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_W))
        operation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_E))
        operation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_R))
        operation = ImGuizmo::SCALE;

    if (ImGuizmo::Manipulate(&view.m[0][0], &proj.m[0][0], operation, ImGuizmo::LOCAL, &world.m[0][0]))
    {
        XMMATRIX edited_world = XMLoadFloat4x4(&world);
        XMMATRIX parent_world = get_parent_world_matrix(ash::scene_g_selected);
        XMMATRIX local_matrix = edited_world * XMMatrixInverse(nullptr, parent_world);

        apply_local_transform_from_matrix(ash::scene_g_selected, local_matrix);
    }
}
} // namespace

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

        const ImVec2 image_min = ImGui::GetItemRectMin();
        const ImVec2 image_size = ImGui::GetItemRectSize();
        draw_transform_gizmo(image_min, image_size);
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
}
