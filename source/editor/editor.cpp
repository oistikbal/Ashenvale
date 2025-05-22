#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>

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
    ImGui::StyleColorsDark();

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

    ImGui::Begin("Hello, world!");
    ImGui::Text("Imgui");
    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    PIXEndEvent();
}