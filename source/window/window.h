#pragma once

#include <wtypes.h>

namespace ashenvale::window
{
	inline HWND g_hwnd = nullptr;
}

namespace ashenvale::window
{
	bool initialize(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow);
	void run();
}