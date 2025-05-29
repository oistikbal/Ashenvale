#pragma once
#include <wtypes.h>

namespace ashenvale::window::input
{
	inline bool g_inputs[256] = {};
	inline bool g_mouse_buttons[3] = {};
	inline int g_mouse_x = 0;
	inline int g_mouse_y = 0;
}

namespace ashenvale::window::input
{
	void update(float deltaTime);
	void input_winproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
}