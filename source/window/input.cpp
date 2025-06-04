#include <algorithm>

#include "input.h"
#include "renderer/camera.h"

using namespace ashenvale::renderer::camera;
using namespace DirectX;

void ashenvale::window::input::update(float deltaTime)
{
    const float moveSpeed = 2.0f * deltaTime;
    const float mouseSensitivity = 0.002f;

    static int lastMouseX = ashenvale::window::input::g_mouse_x;
    static int lastMouseY = ashenvale::window::input::g_mouse_y;

    int deltaX = ashenvale::window::input::g_mouse_x - lastMouseX;
    int deltaY = ashenvale::window::input::g_mouse_y - lastMouseY;

    lastMouseX = ashenvale::window::input::g_mouse_x;
    lastMouseY = ashenvale::window::input::g_mouse_y;

    if (ashenvale::window::input::g_mouse_buttons[1])
    {
        g_rotation.y += deltaX * mouseSensitivity;
        g_rotation.x += deltaY * mouseSensitivity;

        g_rotation.x = std::clamp(g_rotation.x, -XM_PIDIV2 + 0.01f, XM_PIDIV2 - 0.01f);
    }

    ashenvale::renderer::camera::g_rotation.x = std::clamp(g_rotation.x, -XM_PIDIV2 + 0.01f, XM_PIDIV2 - 0.01f);

    XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(g_rotation.x, g_rotation.y, g_rotation.z);

    XMVECTOR forward = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rotationMatrix);
    XMVECTOR right = XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), rotationMatrix);
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);

    XMVECTOR position = XMLoadFloat3(&g_position);

    if (ashenvale::window::input::g_inputs['W'])
        position = XMVectorAdd(position, XMVectorScale(forward, moveSpeed));
    if (ashenvale::window::input::g_inputs['S'])
        position = XMVectorSubtract(position, XMVectorScale(forward, moveSpeed));
    if (ashenvale::window::input::g_inputs['A'])
        position = XMVectorSubtract(position, XMVectorScale(right, moveSpeed));
    if (ashenvale::window::input::g_inputs['D'])
        position = XMVectorAdd(position, XMVectorScale(right, moveSpeed));

    XMStoreFloat3(&g_position, position);
}

void ashenvale::window::input::input_winproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
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
