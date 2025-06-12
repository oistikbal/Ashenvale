#include "inspector.h"
#include "scene/scene.h"
#include <imgui.h>

void ashenvale::editor::inspector::render()
{
    if (!g_isOpen)
        return;

    bool visible = ImGui::Begin("Inspector##", &g_isOpen);
    if (visible)
    {
        if (g_selectedEntity && g_selectedEntity.is_alive())
        {
            auto e = g_selectedEntity;
            auto name = e.get<scene::name>();
            ImGui::Text("Entity ID: %llu", static_cast<flecs::entity_t>(e.id()));
            ImGui::Text("%s", name.name.c_str());

            if (auto *transform = e.try_get<ashenvale::scene::transform>())
            {
                ashenvale::scene::transform nt = *transform;

                if (ImGui::CollapsingHeader("Transform"))
                {
                    ImGui::DragFloat3("Position", &nt.position.x, 0.1f);
                    ImGui::DragFloat3("Rotation", &nt.rotation.x, 1.0f, -180.0f, 180.0f);
                    ImGui::DragFloat3("Scale", &nt.scale.x, 0.1f);
                }

                e.set<ashenvale::scene::transform>(nt);
            }

            if (auto *meshRenderer = e.try_get<ashenvale::scene::mesh_renderer>())
            {
                ashenvale::scene::mesh_renderer nmc = *meshRenderer;

                if (ImGui::CollapsingHeader("Mesh Renderer"))
                {
                    ImGui::Text("Meshes: %zu", nmc.meshes.size());
                    ImGui::Text("Materials: %zu", nmc.materials.size());
                }

                e.set<ashenvale::scene::mesh_renderer>(nmc);
            }
        }
    }
    ImGui::End();
}