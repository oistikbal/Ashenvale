#include "present_pass.h"
#include "renderer/device.h"
#include "renderer/renderer.h"
#include "renderer/swapchain.h"

namespace
{

}

void ashenvale::renderer::render_pass::present::initialize()
{

}

void ashenvale::renderer::render_pass::present::execute(const render_pass_context &context)
{
    reset_pipeline();
    swapchain::g_swapChain->Present(1, 0);
}