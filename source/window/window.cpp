#include "window/window.h"
#include "editor/console.h"
#include "editor/editor.h"
#include "renderer/core/swapchain.h"
#include "renderer/renderer.h"
#include "resource/resource_manager.h"
#include "scene/scene.h"
#include "window/input.h"
#include <imgui/imgui_impl_win32.h>
#include <windowsx.h>

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

namespace
{
inline void input_winproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, ash::win_input::input_state &input_state)
{
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wparam < 256)
        {
            input_state.keyboard[wparam] = true;
        }
        break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (wparam < 256)
        {
            input_state.keyboard[wparam] = false;
        }
        break;

    case WM_LBUTTONDOWN:
        input_state.mouse_button[0] = true;
        break;
    case WM_LBUTTONUP:
        input_state.mouse_button[0] = false;
        break;

    case WM_RBUTTONDOWN:
        input_state.mouse_button[1] = true;
        break;
    case WM_RBUTTONUP:
        input_state.mouse_button[1] = false;
        break;

    case WM_MBUTTONDOWN:
        input_state.mouse_button[2] = true;
        break;
    case WM_MBUTTONUP:
        input_state.mouse_button[2] = false;
        break;

    case WM_MOUSEMOVE:
        int new_x = GET_X_LPARAM(lparam);
        int new_y = GET_Y_LPARAM(lparam);

        input_state.mouse_delta_pos[0] = new_x - input_state.mouse_pos[0];
        input_state.mouse_delta_pos[1] = new_y - input_state.mouse_pos[1];

        input_state.mouse_pos[0] = new_x;
        input_state.mouse_pos[1] = new_y;
        break;
    }
}
} // namespace

void ash::win_init(HINSTANCE hInstance, PWSTR pCmdLine, int nCmdShow)
{
    SCOPED_CPU_EVENT(L"ash::win_init");
    ed_console_log(ed_console_log_level::info, "[Window] Initialization begin.");

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "Ashenvale";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    win_g_hwnd = CreateWindowEx(0, wc.lpszClassName, "Ashenvale", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                screenWidth, screenHeight, nullptr, nullptr, wc.hInstance, nullptr);

    assert(win_g_hwnd);
    ed_console_log(ed_console_log_level::info, "[Window] Main window created.");

    ShowWindow(win_g_hwnd, SW_SHOWMAXIMIZED);
    UpdateWindow(win_g_hwnd);
    rhi_init();
    rm_init();
    ed_init();
    scene_init();

    rhi_start();

    ed_console_log(ed_console_log_level::info, "[Window] Initialization complete.");
}

void ash::win_run()
{
    MSG msg = {};

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        SCOPED_CPU_EVENT(L"ash::win_run");
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        return true;

    input_winproc(hwnd, uMsg, wParam, lParam, ash::win_input_get_back_buffer(ash::g_win_input));
    ash::win_input_swap_buffers(ash::g_win_input);

    switch (uMsg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEMOVE:
        break;

    case WM_DESTROY:
        ash::ed_console_log(ash::ed_console_log_level::info, "[App] Shutdown sequence begin.");
        ash::rm_shutdown();
        ash::rhi_stop();
        ash::scene_shutdown();
        ash::ed_shutdown();
        ash::rhi_shutdown();
        ash::ed_console_log(ash::ed_console_log_level::info, "[App] Shutdown sequence end.");
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        ash::ed_console_log(ash::ed_console_log_level::info, "[Window] Resize requested.");
        ash::win_evt_push(ash::win_g_queue, {ash::win_evt_windows_event::resize});
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
