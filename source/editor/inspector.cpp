#include "inspector.h"
#include <imgui.h>
#include "scene/scene.h"

void ashenvale::editor::inspector::render()
{
    if (!g_isOpen)
        return;

    bool visible = ImGui::Begin("Inspector", &g_isOpen);
    if (visible)
    {
        if (g_selectedItem)
        {
            auto *node = static_cast<ashenvale::scene::scene_node *>(g_selectedItem);

            ImGui::Separator();
            ImGui::Text("Transform");

            ImGui::DragFloat3("Position", &node->translation.x, 0.1f);
            ImGui::DragFloat3("Rotation", &node->rotation.x, 0.1f);
            ImGui::DragFloat3("Scale", &node->scale.x, 0.1f);
        }
        else
        {
            ImGui::Text("No node selected.");
        }
    }

    ImGui::End();
}