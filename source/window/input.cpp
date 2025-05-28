#include "input.h"

static bool g_inputs[256] = {};
static bool g_mouse_buttons[3] = {};
static int g_mouse_x = 0;
static int g_mouse_y = 0;

void ashenvale::window::input::update(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wparam < 256)
        {
            g_inputs[wparam] = true;
        }
        break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (wparam < 256)
        {
            g_inputs[wparam] = false;
        }
        break;

    case WM_LBUTTONDOWN:
        g_mouse_buttons[0] = true;
        break;
    case WM_LBUTTONUP:
        g_mouse_buttons[0] = false;
        break;

    case WM_RBUTTONDOWN:
        g_mouse_buttons[1] = true;
        break;
    case WM_RBUTTONUP:
        g_mouse_buttons[1] = false;
        break;

    case WM_MBUTTONDOWN:
        g_mouse_buttons[2] = true;
        break;
    case WM_MBUTTONUP:
        g_mouse_buttons[2] = false;
        break;

    case WM_MOUSEMOVE:
        g_mouse_x = LOWORD(lparam);
        g_mouse_y = HIWORD(lparam);
        break;
    }
}
