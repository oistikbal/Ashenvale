#include "present_pass.h"
#include "renderer/device.h"
#include "renderer/renderer.h"
#include "renderer/swapchain.h"

namespace
{
ashenvale::renderer::render_pass::render_pass_pso g_pso;
}

void ashenvale::renderer::render_pass::present::initialize()
{
    g_pso = {};
}

void ashenvale::renderer::render_pass::present::execute(const render_pass_context &context)
{
    bind_pso(g_pso);
    swapchain::g_swapChain->Present(1, 0);
}