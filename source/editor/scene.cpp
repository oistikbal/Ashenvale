#include "scene.h"
#include "inspector.h"
#include "scene/component.h"
#include "scene/scene.h"
#include <imgui.h>

void ashenvale::editor::scene::render()
{
    if (!g_isOpen)
        return;

    bool visible = ImGui::Begin("Scene", &g_isOpen);
    {
        ashenvale::scene::g_world.each([&](flecs::entity e, ashenvale::scene::name n) {
            bool isSelected = (ashenvale::editor::inspector::g_selectedEntity == e);
            if (ImGui::Selectable((n.name + "###entity" + std::to_string(e.id())).c_str(), isSelected))
            {
                ashenvale::editor::inspector::g_selectedEntity = e;
            }
        });
    }

    ImGui::End();
}