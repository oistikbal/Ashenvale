#include <imgui.h>

#include "editor/viewport.h"
#include "profiler/profiler.h"
#include "renderer/render_graph.h"
#include "renderer/renderer.h"

void ashenvale::editor::viewport::render()
{
    PIX_SCOPED_EVENT("editor.viewport.render")
    ImGui::Begin("Viewport");

    ImVec2 imagePos = ImGui::GetCursorScreenPos();

    ImVec2 size = ImGui::GetContentRegionAvail();
    int topbarWidth = size.x < 16 ? 16 : static_cast<int>(size.x);

    ImGui::SetCursorScreenPos(imagePos);

    ImGui::BeginChild("TopBar", ImVec2((float)topbarWidth, 25.0f), false,
                      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove |
                          ImGuiWindowFlags_NoSavedSettings);

    ImGui::SameLine();
    static int currentItem = 0;
    const char *items[] = {"None", "Depth", "Wireframe", "Wireframe Lit"};

    ImGui::SetNextItemWidth(100);
    ImGui::Combo("##combo", &currentItem, items, IM_ARRAYSIZE(items));

    renderer::render_graph::g_debugView =
        static_cast<ashenvale::renderer::render_graph::debug_view>(static_cast<uint8_t>(currentItem));

    ImGui::EndChild();

    ImGui::SetCursorScreenPos(ImVec2(imagePos.x, imagePos.y + 24.0f));

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
    ImGui::End();
}