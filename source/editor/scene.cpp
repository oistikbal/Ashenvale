#include "scene.h"
#include <imgui.h>

void ashenvale::editor::scene::render()
{
    if (!g_isOpen)
        return;

    bool visible = ImGui::Begin("Scene", &g_isOpen);
    if (visible)
    {

    }

    ImGui::End();
}