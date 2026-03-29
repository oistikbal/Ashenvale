#include "inspector.h"
#include "IconsMaterialSymbols.h"
#include "common.h"
#include "console.h"
#include "resource/resource_manager.h"
#include "renderer/renderer.h"
#include "scene/component.h"
#include "scene/scene.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <imgui/imgui.h>
#include <cmath>
#include <string>

namespace
{
struct rotation_ui_state
{
    uint64_t entity_id = 0;
    bool initialized = false;
    DirectX::XMFLOAT3 euler_degrees = {0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT4 last_quaternion = {0.0f, 0.0f, 0.0f, 1.0f};
};

rotation_ui_state g_rotation_ui_state = {};

bool quaternion_nearly_equal(const DirectX::XMFLOAT4 &a, const DirectX::XMFLOAT4 &b)
{
    constexpr float epsilon = 1e-5f;
    return std::abs(a.x - b.x) <= epsilon && std::abs(a.y - b.y) <= epsilon && std::abs(a.z - b.z) <= epsilon &&
           std::abs(a.w - b.w) <= epsilon;
}

DirectX::XMFLOAT3 quaternion_to_euler_radians(const DirectX::XMFLOAT4 &q)
{
    // Convert quaternion to pitch (X), yaw (Y), roll (Z) in radians.
    const float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    const float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    const float pitch = std::atan2(sinr_cosp, cosr_cosp);

    const float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    const float yaw = (std::abs(sinp) >= 1.0f) ? std::copysign(DirectX::XM_PIDIV2, sinp) : std::asin(sinp);

    const float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    const float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    const float roll = std::atan2(siny_cosp, cosy_cosp);

    return {pitch, yaw, roll};
}

bool contains_case_insensitive(const std::string &text, const std::string &query)
{
    if (query.empty())
    {
        return true;
    }

    auto it = std::search(text.begin(), text.end(), query.begin(), query.end(), [](char lhs, char rhs) {
        return std::tolower(static_cast<unsigned char>(lhs)) == std::tolower(static_cast<unsigned char>(rhs));
    });
    return it != text.end();
}

bool draw_axis_float(const char *axis_label, const ImVec4 &color, float &value, float speed, bool *is_active = nullptr)
{
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(axis_label);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(70.0f);
    const bool changed = ImGui::DragFloat("##axis", &value, speed);
    if (is_active != nullptr)
    {
        *is_active |= ImGui::IsItemActive();
    }
    return changed;
}

bool draw_vec3_row(const char *row_label, float *values, float speed, bool *is_active = nullptr)
{
    bool changed = false;

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(row_label);

    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(row_label);
    changed |= draw_axis_float("X", ImVec4(0.90f, 0.30f, 0.30f, 1.0f), values[0], speed, is_active);
    ImGui::SameLine();
    ImGui::PushID("y");
    ImGui::SameLine();

    changed |= draw_axis_float("Y", ImVec4(0.30f, 0.85f, 0.30f, 1.0f), values[1], speed, is_active);
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::PushID("z");
    ImGui::SameLine();

    changed |= draw_axis_float("Z", ImVec4(0.35f, 0.60f, 0.95f, 1.0f), values[2], speed, is_active);
    ImGui::PopID();
    ImGui::PopID();

    return changed;
}

void draw_name_field(flecs::entity entity)
{
    static uint64_t buffered_entity_id = 0;
    static char name_buffer[256] = {};

    if (buffered_entity_id != entity.id())
    {
        std::string name = entity.name().c_str();

        memset(name_buffer, 0, sizeof(name_buffer));
        const size_t copy_count = (name.size() < (sizeof(name_buffer) - 1)) ? name.size() : (sizeof(name_buffer) - 1);
        memcpy(name_buffer, name.c_str(), copy_count);
        buffered_entity_id = entity.id();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(ICON_MS_DEPLOYED_CODE);
    ImGui::SameLine();

    ImGui::PushItemWidth(-1.0f);
    const bool submitted =
        ImGui::InputText("##InspectorName", name_buffer, sizeof(name_buffer), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopItemWidth();

    if (submitted || ImGui::IsItemDeactivatedAfterEdit())
    {
        std::string old_name = entity.name().c_str();
        ash::scene_set_entity_name_safe(entity, name_buffer);

        std::string resolved_name = entity.name().c_str();
        if (resolved_name != old_name)
        {
            ash::ed_console_log(ash::ed_console_log_level::info, "[Inspector] Entity renamed.");
        }
        memset(name_buffer, 0, sizeof(name_buffer));
        const size_t copy_count =
            (resolved_name.size() < (sizeof(name_buffer) - 1)) ? resolved_name.size() : (sizeof(name_buffer) - 1);
        memcpy(name_buffer, resolved_name.c_str(), copy_count);
    }
}

void draw_transform_component(flecs::entity entity)
{
    if (!entity.has<ash::transform>())
    {
        return;
    }

    ash::transform transform_value = entity.get<ash::transform>();
    bool changed = false;

    ImGuiTreeNodeFlags header_flags = ImGuiTreeNodeFlags_DefaultOpen;
    if (ImGui::CollapsingHeader(ICON_MS_OPEN_WITH " Transform", header_flags))
    {
        ImGuiTableFlags table_flags = ImGuiTableFlags_SizingFixedFit;
        if (ImGui::BeginTable("TransformTable", 2, table_flags))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            changed |= draw_vec3_row("Position", &transform_value.position.x, 0.1f);

            const uint64_t entity_id = entity.id();
            if (!g_rotation_ui_state.initialized || g_rotation_ui_state.entity_id != entity_id ||
                !quaternion_nearly_equal(g_rotation_ui_state.last_quaternion, transform_value.rotation))
            {
                const DirectX::XMFLOAT3 rotation_euler_radians = quaternion_to_euler_radians(transform_value.rotation);
                g_rotation_ui_state.euler_degrees = {DirectX::XMConvertToDegrees(rotation_euler_radians.x),
                                                     DirectX::XMConvertToDegrees(rotation_euler_radians.y),
                                                     DirectX::XMConvertToDegrees(rotation_euler_radians.z)};
                g_rotation_ui_state.last_quaternion = transform_value.rotation;
                g_rotation_ui_state.entity_id = entity_id;
                g_rotation_ui_state.initialized = true;
            }

            const bool rotation_changed =
                draw_vec3_row("Rotation", &g_rotation_ui_state.euler_degrees.x, 0.5f, nullptr);
            if (rotation_changed)
            {
                const DirectX::XMFLOAT3 rotation_euler_radians = {
                    DirectX::XMConvertToRadians(g_rotation_ui_state.euler_degrees.x),
                    DirectX::XMConvertToRadians(g_rotation_ui_state.euler_degrees.y),
                    DirectX::XMConvertToRadians(g_rotation_ui_state.euler_degrees.z)};
                DirectX::XMVECTOR quat =
                    DirectX::XMQuaternionRotationRollPitchYaw(rotation_euler_radians.x, rotation_euler_radians.y,
                                                              rotation_euler_radians.z);
                quat = DirectX::XMQuaternionNormalize(quat);
                DirectX::XMStoreFloat4(&transform_value.rotation, quat);
                g_rotation_ui_state.last_quaternion = transform_value.rotation;
            }
            changed |= rotation_changed;
            changed |= draw_vec3_row("Scale", &transform_value.scale.x, 0.1f);

            ImGui::EndTable();
        }
    }

    if (changed)
    {
        entity.set<ash::transform>(transform_value);
    }
}

void draw_mesh_component(flecs::entity entity)
{
    if (!entity.has<ash::mesh_component>())
    {
        return;
    }

    ash::mesh_component mesh_value = entity.get<ash::mesh_component>();
    bool changed = false;

    ImGuiTreeNodeFlags header_flags = ImGuiTreeNodeFlags_DefaultOpen;
    if (ImGui::CollapsingHeader(ICON_MS_GRID_VIEW " Mesh", header_flags))
    {
        ImGuiTableFlags table_flags = ImGuiTableFlags_SizingFixedFit;
        if (ImGui::BeginTable("MeshTable", 2, table_flags))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(ICON_MS_DATA_OBJECT " Handle");

            ImGui::TableSetColumnIndex(1);
            changed |= ImGui::InputScalar("##MeshHandle", ImGuiDataType_U32, &mesh_value.handle);

            ImGui::EndTable();
        }
    }

    if (changed)
    {
        entity.set<ash::mesh_component>(mesh_value);
    }
}

bool draw_texture_selector_popup(const char *popup_id, uint32_t &texture_handle)
{
    bool changed = false;
    static float thumbnail_size = 72.0f;
    static char search_buffer[64] = {};

    ImGui::SetNextWindowSizeConstraints(ImVec2(420.0f, 280.0f), ImVec2(1200.0f, 900.0f));
    ImGui::SetNextWindowSize(ImVec2(760.0f, 520.0f), ImGuiCond_Appearing);

    if (ImGui::BeginPopup(popup_id))
    {
        ImGui::TextUnformatted(ICON_MS_TEXTURE " Select Texture");
        ImGui::SameLine();
        ImGui::TextDisabled("(Current: #%u)", texture_handle);
        ImGui::Separator();

        if (ImGui::Button(ICON_MS_BROKEN_IMAGE " Clear Selection"))
        {
            texture_handle = 0;
            changed = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(170.0f);
        ImGui::SliderFloat("Thumbnail", &thumbnail_size, 48.0f, 160.0f, "%.0f px");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextWithHint("##TextureSearch", "Search by handle or size", search_buffer, sizeof(search_buffer));

        const uint32_t texture_count = ash::rm_get_texture_count();
        if (texture_count == 0)
        {
            ImGui::TextDisabled("No textures loaded.");
            ImGui::EndPopup();
            return changed;
        }

        ImGui::Separator();
        if (ImGui::BeginChild("TextureSelectorGrid", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_AlwaysVerticalScrollbar))
        {
            const ImGuiStyle &style = ImGui::GetStyle();
            const float cell_width = thumbnail_size + style.ItemSpacing.x + 28.0f;
            int column_count = static_cast<int>(ImGui::GetContentRegionAvail().x / cell_width);
            column_count = (column_count < 1) ? 1 : column_count;

            uint32_t visible_count = 0;
            if (ImGui::BeginTable("TextureSelectorTable", column_count, ImGuiTableFlags_SizingStretchSame))
            {
                for (uint32_t handle = 1; handle <= texture_count; ++handle)
                {
                    const ash::resource_texture *texture = ash::rm_get_texture(handle);
                    if (texture == nullptr)
                    {
                        continue;
                    }

                    const std::string handle_text = std::to_string(handle);
                    const std::string size_text = std::to_string(texture->width) + "x" + std::to_string(texture->height);
                    const std::string search_text = handle_text + " " + size_text;
                    if (!contains_case_insensitive(search_text, search_buffer))
                    {
                        continue;
                    }

                    ++visible_count;
                    ImGui::TableNextColumn();
                    ImGui::PushID(static_cast<int>(handle));

                    const bool is_current = (handle == texture_handle);
                    if (is_current)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.65f, 1.00f, 1.0f));
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
                    }

                    bool selected = false;
                    if (texture->srv_descriptor_index != ash::rhi_invalid_descriptor_index)
                    {
                        const D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle =
                            ash::rhi_get_cbv_srv_uav_gpu_descriptor(texture->srv_descriptor_index);
                        selected = ImGui::ImageButton("##TextureButton", (ImTextureID)(intptr_t)gpu_handle.ptr,
                                                      ImVec2(thumbnail_size, thumbnail_size));
                    }
                    else
                    {
                        selected = ImGui::Button(ICON_MS_BROKEN_IMAGE "##TextureButton",
                                                 ImVec2(thumbnail_size, thumbnail_size));
                    }

                    if (is_current)
                    {
                        ImGui::PopStyleVar();
                        ImGui::PopStyleColor();
                    }

                    if (selected)
                    {
                        texture_handle = handle;
                        changed = true;
                        ImGui::CloseCurrentPopup();
                    }

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Handle: %u\nSize: %ux%u", handle, texture->width, texture->height);
                    }

                    if (is_current)
                    {
                        ImGui::Text("%s #%u", ICON_MS_CHECK, handle);
                    }
                    else
                    {
                        ImGui::TextDisabled("#%u", handle);
                    }
                    ImGui::TextDisabled("%ux%u", texture->width, texture->height);
                    ImGui::PopID();
                }

                ImGui::EndTable();
            }

            if (visible_count == 0)
            {
                ImGui::TextDisabled("No textures match the current search.");
            }
            ImGui::EndChild();
        }

        ImGui::EndPopup();
    }

    return changed;
}

bool draw_texture_preview_row(const char *label, const char *popup_id, uint32_t &texture_handle)
{
    bool changed = false;

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);

    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);

    const ash::resource_texture *texture = ash::rm_get_texture(texture_handle);
    if (texture == nullptr || texture->srv_descriptor_index == ash::rhi_invalid_descriptor_index)
    {
        ImGui::TextDisabled(ICON_MS_BROKEN_IMAGE " Unassigned");
    }
    else
    {
        const D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle =
            ash::rhi_get_cbv_srv_uav_gpu_descriptor(texture->srv_descriptor_index);

        constexpr float preview_size = 72.0f;
        ImGui::Image((ImTextureID)(intptr_t)gpu_handle.ptr, ImVec2(preview_size, preview_size));
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::TextDisabled("Handle: %u", texture_handle);
        ImGui::TextDisabled("%ux%u", texture->width, texture->height);
        ImGui::EndGroup();
    }

    if (ImGui::Button("Select Texture"))
    {
        ImGui::OpenPopup(popup_id);
    }
    changed |= draw_texture_selector_popup(popup_id, texture_handle);

    ImGui::PopID();
    return changed;
}

void draw_material_component(flecs::entity entity)
{
    if (!entity.has<ash::material>())
    {
        return;
    }

    ash::material material_value = entity.get<ash::material>();
    bool changed = false;

    ImGui::PushID(static_cast<int>(entity.id()));

    ImGuiTreeNodeFlags header_flags = ImGuiTreeNodeFlags_DefaultOpen;
    if (ImGui::CollapsingHeader(ICON_MS_PALETTE " Material", header_flags))
    {
        ImGuiTableFlags table_flags = ImGuiTableFlags_SizingFixedFit;
        if (ImGui::BeginTable("MaterialTable", 2, table_flags))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            changed |= draw_texture_preview_row(ICON_MS_TEXTURE " Albedo", "AlbedoTextureSelectorPopup",
                                                material_value.albedo);
            changed |= draw_texture_preview_row(ICON_MS_TEXTURE " Normal", "NormalTextureSelectorPopup",
                                                material_value.normal);

            ImGui::EndTable();
        }
    }

    if (changed)
    {
        entity.set<ash::material>(material_value);
    }

    ImGui::PopID();
}
} // namespace

void ash::ed_inspector_init()
{
    SCOPED_CPU_EVENT(L"ash::ed_inspector_init");
}

void ash::ed_inspector_render()
{
    SCOPED_CPU_EVENT(L"ash::ed_inspector_render");
    if (!ed_inspector_g_is_open)
        return;

    ImGui::Begin(ICON_MS_INFO " Inspector ###Inspector", &ed_inspector_g_is_open);

    if (!scene_g_selected.is_valid())
    {
        ImGui::TextDisabled("No entity selected.");
        ImGui::End();
        return;
    }

    draw_name_field(scene_g_selected);
    draw_transform_component(scene_g_selected);
    draw_mesh_component(scene_g_selected);
    draw_material_component(scene_g_selected);

    ImGui::End();
}
