
#include "editor/editor.h"
#include "renderer/device.h"
#include "window/window.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    SetThreadDescription(GetCurrentThread(), L"MainThread");

    if (!ashenvale::window::initialize(hInstance, hPrevInstance, pCmdLine, nCmdShow))
    {
        return -1;
    }

    if (!ashenvale::renderer::device::initialize())
    {
        return -1;
    }

    if (!ashenvale::editor::initialize())
    {
        return -1;
    }

    ashenvale::window::run();

    return 0;
}
