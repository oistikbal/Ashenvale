#include "settings.h"

#include <imgui.h>
#include <renderer/render_graph.h>

void ashenvale::editor::settings::render()
{
    if (!g_isOpen)
        return;

  
    bool visible = ImGui::Begin("Settings", &g_isOpen);
    if (visible)
    {
        if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen))
        {
            static int currentItem = 0;
            const char *items[] = {"Forward", "Deferred"};

            ImGui::Combo("Render Path", &currentItem, items, IM_ARRAYSIZE(items));

            renderer::render_graph::g_renderPath =
                static_cast<ashenvale::renderer::render_graph::render_path>(static_cast<uint8_t>(currentItem));
        }

        if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_DefaultOpen))
        {
            static int currentItem = 0;
            const char *items[] = {"None", "Depth", "Wireframe", "Wireframe Lit", "Normal"};

            ImGui::Combo("View Mode", &currentItem, items, IM_ARRAYSIZE(items));

            renderer::render_graph::g_debugView =
                static_cast<ashenvale::renderer::render_graph::debug_view>(static_cast<uint8_t>(currentItem));
        }
    }

    ImGui::End();
}