#pragma once

#include <minwindef.h>

namespace ashenvale::editor
{
	bool initialize();
	void render();
	LRESULT wind_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
}