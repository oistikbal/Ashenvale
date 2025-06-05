#include <imgui.h>

#include "editor/viewport.h"
#include "profiler/profiler.h"
#include "renderer/render_graph.h"
#include "renderer/renderer.h"

void ashenvale::editor::viewport::render()
{
    PIX_SCOPED_EVENT("editor.viewport.render")
    if (!g_isOpen)
        return;

    bool visible = ImGui::Begin("Viewport", &g_isOpen);
    if (visible)
    {
        ImVec2 imagePos = ImGui::GetCursorScreenPos();

        ImVec2 size = ImGui::GetContentRegionAvail();
        int topbarWidth = size.x < 16 ? 16 : static_cast<int>(size.x);

        size = ImGui::GetContentRegionAvail();
        int newWidth = size.x < 16 ? 16 : static_cast<int>(size.x);
        int newHeight = size.y < 16 ? 16 : static_cast<int>(size.y);

        static int lastW = 0, lastH = 0;
        if (newWidth != lastW || newHeight != lastH)
        {
            lastW = newWidth;
            lastH = newHeight;
            ashenvale::renderer::resize_viewport(newWidth, newHeight);
        }

        ImGui::Image((ImTextureID)(intptr_t)ashenvale::renderer::g_viewportSRV.get(), ImVec2(newWidth, newHeight));
    }

    ImGui::End(); // Always call End, even if the window is collapsed
}