#include <imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx11.h>
#include <imgui/backends/imgui_impl_win32.cpp>

#include "editor.h"
#include "window/window.h"
#include "renderer/device.h"
#include "profiler/profiler.h"

bool ashenvale::editor::initialize()
{
    PIXBeginEvent(0, "editor.initialize");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable | ImGuiConfigFlags_DockingEnable;

    ImGui_ImplWin32_Init(ashenvale::window::g_hwnd);
    ImGui_ImplDX11_Init(ashenvale::renderer::device::g_device.Get(), ashenvale::renderer::device::g_context.Get());

    PIXEndEvent();
    return true;
}

void ashenvale::editor::render()
{
    PIXBeginEvent(0, "editor.render");
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Empty");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Assets")) {
            ImGui::MenuItem("Empty");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Windows")) {
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

    ImGui::Begin("Viewport");
    ImVec2 size = ImGui::GetContentRegionAvail();
    int newWidth = 16 > static_cast<int>(size.x) ? 16 : static_cast<int>(size.x);
    int newHeight = 16 > static_cast<int>(size.y) ? 16 : static_cast<int>(size.y);;

    static int lastW = 0, lastH = 0;
    if (newWidth != lastW || newHeight != lastH)
    {
        lastW = newWidth;
        lastH = newHeight;
        ashenvale::renderer::device::resize_viewport(newWidth, newHeight);
    }
    ImGui::Image((ImTextureID)(intptr_t)ashenvale::renderer::device::g_viewportSRV.Get(), ImVec2(newWidth, newHeight));
    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    PIXEndEvent();
}

LRESULT ashenvale::editor::wind_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);
}
