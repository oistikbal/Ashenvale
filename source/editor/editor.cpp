#include "editor.h"
#include "IconsMaterialSymbols.h"
#include "common.h"
#include "configs/config.h"
#include "console.h"
#include "hierarchy.h"
#include "inspector.h"
#include "renderer/core/command_queue.h"
#include "renderer/core/swapchain.h"
#include "renderer/renderer.h"
#include "scene/scene.h"
#include "viewport.h"
#include "window/window.h"
#include <array>
#include <commdlg.h>
#include <filesystem>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx12.h>
#include <imgui/imgui_impl_win32.h>
#include <optional>

using namespace winrt;

namespace
{
void load_default_ini()
{
    SCOPED_CPU_EVENT(L"ash::ed_load_default_ini")

    std::string full_path = (std::filesystem::path(ash::cfg_RESOURCES_PATH) / "imgui.ini").string();
    ImGui::LoadIniSettingsFromDisk(full_path.c_str());
    ash::ed_console_log(ash::ed_console_log_level::info, "[Editor] Default layout loaded.");
}

std::optional<std::filesystem::path> choose_gltf_scene_file()
{
    std::array<char, MAX_PATH> file_path = {};

    OPENFILENAMEA open_file_dialog = {};
    open_file_dialog.lStructSize = sizeof(open_file_dialog);
    open_file_dialog.hwndOwner = ash::win_g_hwnd;
    open_file_dialog.lpstrFile = file_path.data();
    open_file_dialog.nMaxFile = static_cast<DWORD>(file_path.size());
    open_file_dialog.lpstrFilter = "glTF Scene Files (*.gltf;*.glb)\0*.gltf;*.glb\0All Files (*.*)\0*.*\0";
    open_file_dialog.nFilterIndex = 1;
    open_file_dialog.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&open_file_dialog) == FALSE)
    {
        return std::nullopt;
    }

    return std::filesystem::path(file_path.data());
}
} // namespace

void ash::ed_init()
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpu = rhi_get_cbv_srv_uav_cpu_descriptor(rhi_cbv_srv_uav_imgui_descriptor_index);
    D3D12_GPU_DESCRIPTOR_HANDLE gpu = rhi_get_cbv_srv_uav_gpu_descriptor(rhi_cbv_srv_uav_imgui_descriptor_index);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = rhi_g_device.get();
    init_info.CommandQueue = rhi_cmd_g_direct.get();
    init_info.NumFramesInFlight = 1;
    init_info.RTVFormat = rhi_sw_g_format;

    init_info.SrvDescriptorHeap = rhi_g_cbv_srv_uav_heap.get();
    init_info.SrvDescriptorAllocFn = nullptr;
    init_info.SrvDescriptorFreeFn = nullptr;
    init_info.LegacySingleSrvCpuDescriptor = cpu;
    init_info.LegacySingleSrvGpuDescriptor = gpu;

    ImGui_ImplWin32_Init(win_g_hwnd);
    ImGui_ImplDX12_Init(&init_info);

    ImFontConfig config{};
    config.MergeMode = false;

    std::string full_path = (std::filesystem::path(cfg_RESOURCES_PATH) / "Roboto-Regular.ttf").string();
    io.Fonts->AddFontFromFileTTF((full_path.c_str()), 14.0f, &config);

    static const ImWchar icon_ranges[] = {ICON_MIN_MS, ICON_MAX_MS, 0};
    config.MergeMode = true;
    config.GlyphOffset.y = 6;

    full_path =
        (std::filesystem::path(cfg_RESOURCES_PATH) / "MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf").string();
    io.Fonts->AddFontFromFileTTF((full_path.c_str()), 24, &config, icon_ranges);

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

    style.WindowRounding = 2.0f;
    style.ChildRounding = 2.0f;
    style.FrameRounding = 2.0f;
    style.PopupRounding = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 2.0f;

    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(6.0f, 7.0f);
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.IndentSpacing = 21.0f;
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 10.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 1.0f;
    style.TabBarBorderSize = 0.0f;
    style.TabBarOverlineSize = 0.0f;

    style.CellPadding = ImVec2(0.0f, 4.0f);
    style.TreeLinesSize = 1.0f;
    style.TreeLinesRounding = 5.0f;
    style.CellPadding = ImVec2(4.0f, 4.0f);
    style.TreeLinesFlags = ImGuiTreeNodeFlags_DrawLinesFull;

    style.DockingSeparatorSize = 1.0f;

    ed_vp_init();
    ed_hierarchy_init();
    ed_inspector_init();
    ed_console_init();

    if (!std::filesystem::exists("imgui.ini"))
    {
        load_default_ini();
    }
}

void ash::ed_render()
{
    SCOPED_CPU_EVENT(L"ash::ed_render")

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Scene"))
        {
            if (ImGui::MenuItem("Load glTF Scene..."))
            {
                if (auto selected_path = choose_gltf_scene_file(); selected_path.has_value())
                {
                    ed_console_log(ed_console_log_level::info, "[Editor] glTF file selected from dialog.");
                    scene_load_gltf(selected_path.value());
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Windows"))
        {
            if (ImGui::MenuItem("Viewport"))
                ed_vp_g_is_open = true;
            if (ImGui::MenuItem("Hierarchy"))
                ed_hierarchy_g_is_open = true;
            if (ImGui::MenuItem("Inspector"))
                ed_inspector_g_is_open = true;
            if (ImGui::MenuItem("Console"))
                ed_console_g_is_open = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("GameObject"))
        {
            if (ImGui::MenuItem(ICON_MS_ADD " Create Empty"))
            {
                ed_console_log(ed_console_log_level::info, "[Editor] Create Empty requested.");
                scene_create_empty();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Layout"))
        {
            if (ImGui::MenuItem("Load default"))
            {
                load_default_ini();
            }

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

    ed_vp_render();
    ed_hierarchy_render();
    ed_inspector_render();
    ed_console_render();

    ImGui::Render();
}

void ash::ed_shutdown()
{
    SCOPED_CPU_EVENT(L"ash::ed_shutdown")

    if (ImGui::GetCurrentContext())
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
}
