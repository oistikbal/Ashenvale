#include "settings.h"
#include "renderer/device.h"
#include "renderer/render_passes/tonemap_pass.h"

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

            if (ImGui::SliderFloat("Exposure", &g_exposure, 0.1f, 100.0f, "%.1f"))
            {
                D3D11_MAPPED_SUBRESOURCE mapped;
                HRESULT hr = renderer::device::g_context->Map(renderer::render_pass::tonemap::g_exposureBuffer.get(), 0,
                                                              D3D11_MAP_WRITE_DISCARD, 0, &mapped);

                auto *exposure =
                    reinterpret_cast<ashenvale::renderer::render_pass::tonemap::exposure_buffer *>(mapped.pData);

                exposure->exposure = g_exposure;

                renderer::device::g_context->Unmap(renderer::render_pass::tonemap::g_exposureBuffer.get(), 0);
            }
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