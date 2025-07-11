#include <Windows.h>
#include <commdlg.h>
#include <imgui.h>
#include <imgui/backends/imgui_impl_dx11.h>
#include <imgui/backends/imgui_impl_win32.cpp>
#include <imgui/backends/imgui_impl_win32.h>

#include "console.h"
#include "editor.h"
#include "editor/scene.h"
#include "editor/settings.h"
#include "editor/viewport.h"
#include "inspector.h"
#include "profiler/profiler.h"
#include "renderer/device.h"
#include "renderer/renderer.h"
#include "scene/scene.h"
#include "scene/skydome.h"
#include "window/window.h"

bool ashenvale::editor::initialize()
{
    PIX_SCOPED_EVENT("editor.initialize")
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable | ImGuiConfigFlags_DockingEnable;

    ImGui_ImplWin32_Init(ashenvale::window::g_hwnd);
    ImGui_ImplDX11_Init(ashenvale::renderer::device::g_device.get(), ashenvale::renderer::device::g_context.get());

    ImGuiStyle &style = ImGui::GetStyle();
    ImVec4 *colors = style.Colors;

    colors[ImGuiCol_Text] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.082f, 0.082f, 0.082f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.082f, 0.082f, 0.082f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.082f, 0.082f, 0.082f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.082f, 0.082f, 0.082f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.082f, 0.082f, 0.082f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.43f, 0.43f, 0.43f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.44f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.000f, 0.434f, 0.878f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.000f, 0.434f, 0.878f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.000f, 0.439f, 0.878f, 0.824f);
    colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.082f, 0.082f, 0.082f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.082f, 0.082f, 0.082f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.19f, 0.53f, 0.78f, 0.22f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.00f, 0.44f, 0.88f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.00f, 0.47f, 0.94f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 0.44f, 0.88f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.00f, 0.47f, 0.94f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.082f, 0.082f, 0.082f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.082f, 0.082f, 0.082f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.011f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.188f, 0.529f, 0.780f, 1.000f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.00f, 0.44f, 0.88f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.00f, 0.44f, 0.88f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

    style.FrameRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.TabRounding = 0.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 0.0f;
    style.PopupBorderSize = 1.5f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;

    style.AntiAliasedFill = true;
    style.AntiAliasedLines = true;
    style.AntiAliasedLinesUseTex = true;

    style.WindowPadding = ImVec2(4.0f, 4.0f);
    style.FramePadding = ImVec2(4.0f, 4.0f);
    style.CellPadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 3.0f);
    style.ItemInnerSpacing = ImVec2(2.0f, 4.0f);
    style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
    style.IndentSpacing = 12;
    style.ScrollbarSize = 14;
    style.GrabMinSize = 10;

    ashenvale::editor::console::initialize();
    ashenvale::editor::inspector::initialize();

    return true;
}

void ashenvale::editor::render()
{
    PIX_SCOPED_EVENT("editor.render")
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Scene"))
        {
            if (ImGui::MenuItem("Open glTF Scene"))
            {
                char filename[MAX_PATH] = {};

                OPENFILENAME ofn = {};
                ofn.lStructSize = sizeof(ofn);
                ofn.lpstrFilter = "glTF Files\0*.gltf;*.glb\0All Files\0*.*\0";
                ofn.lpstrFile = filename;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST;
                ofn.hwndOwner = window::g_hwnd;
                if (GetOpenFileNameA(&ofn))
                {
                    ashenvale::scene::load_scene(filename);
                }
            }
            if (ImGui::MenuItem("Open HDRI Skydome"))
            {
                char filename[MAX_PATH] = {};

                OPENFILENAME ofn = {};
                ofn.lStructSize = sizeof(ofn);
                ofn.lpstrFilter = "HDRI Files\0*.hdr;";
                ofn.lpstrFile = filename;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST;
                ofn.hwndOwner = window::g_hwnd;
                if (GetOpenFileNameA(&ofn))
                {
                    ashenvale::scene::skydome::load_hdri_skydome(filename);
                }
            }

            if (ImGui::MenuItem("Close Scene"))
            {
                ashenvale::scene::close_scene();
            }

            if (ImGui::MenuItem("Close Skydome"))
            {
                ashenvale::scene::skydome::close();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Windows"))
        {
            if (ImGui::MenuItem("Viewport"))
                viewport::g_isOpen = true;
            if (ImGui::MenuItem("Settings"))
                settings::g_isOpen = true;
            if (ImGui::MenuItem("Console"))
                console::g_isOpen = true;
            if (ImGui::MenuItem("Scene"))
                scene::g_isOpen = true;
            if (ImGui::MenuItem("Inspector"))
                inspector::g_isOpen = true;

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

    ashenvale::editor::viewport::render();
    ashenvale::editor::console::render();
    ashenvale::editor::settings::render();
    ashenvale::editor::scene::render();
    ashenvale::editor::inspector::render();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
}

LRESULT ashenvale::editor::wind_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);
}
