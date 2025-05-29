#include "window/window.h"
#include "renderer/swapchain.h"
#include "profiler/profiler.h"
#include "editor/editor.h"
#include "renderer/renderer.h"
#include "window/input.h"

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool ashenvale::window::initialize(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    PIX_SCOPED_EVENT("window.initialize")
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "Ashenvale";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    g_hwnd = CreateWindowEx(0, wc.lpszClassName, "Ashenvale", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, nullptr, nullptr, wc.hInstance, nullptr);

    if (!g_hwnd)
    {
        return false;
    }

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);
    return true;
}

void ashenvale::window::run()
{
    MSG msg = {};

    auto lastTime = std::chrono::high_resolution_clock::now();

    while (msg.message != WM_QUIT)
    {
        PIX_SCOPED_EVENT("window.run")
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> delta = now - lastTime;
        lastTime = now;

        ashenvale::window::input::update(delta.count());
        ashenvale::renderer::render();
    }
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (ashenvale::editor::wind_proc(hwnd, uMsg, wParam, lParam))
        return true;

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
        ashenvale::window::input::input_winproc(hwnd, uMsg, wParam, lParam);
        break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            ashenvale::renderer::swapchain::resize(width, height);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
