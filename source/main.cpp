#include "window/window.h"
#include "renderer/device.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    if (!ashenvale::window::initialize(hInstance, hPrevInstance, pCmdLine, nCmdShow)) {
        return -1;
    }

    if (!ashenvale::renderer::initialize()) {
        return -1;
    }

    ashenvale::window::run();
    ashenvale::renderer::shutdown();

    return 0;
}
