#include "scene.h"
#include <imgui.h>
#include "scene/scene.h"
#include "inspector.h"

void ashenvale::editor::scene::render()
{
    if (!g_isOpen)
        return;

    bool visible = ImGui::Begin("Scene", &g_isOpen);
    {
        ImGui::Separator();

        for (size_t i = 0; i < ashenvale::scene::g_nodes.size(); ++i)
        {
            auto &node = ashenvale::scene::g_nodes[i];
            std::string label = node.name.empty() ? ("Node " + std::to_string(i)) : node.name;

            bool isSelected = (ashenvale::editor::inspector::g_selectedItem == &node);

            if (ImGui::Selectable(label.c_str(), isSelected))
            {
                ashenvale::editor::inspector::g_selectedItem = &node;
            }
        }
    }

    ImGui::End();
}