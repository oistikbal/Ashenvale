#include "clear_pass.h"
#include "renderer/device.h"
#include "renderer/renderer.h"

namespace
{
	ashenvale::renderer::render_pass::render_pass_pso g_pso;
}

void ashenvale::renderer::render_pass::clear::initialize()
{
	g_pso = {};
}

void ashenvale::renderer::render_pass::clear::execute(const render_pass_context& context)
{
	bind_pso(g_pso);
    ashenvale::renderer::device::g_context->ClearRenderTargetView(context.clear.rtv, context.clear.clearColor);
    ashenvale::renderer::device::g_context->ClearDepthStencilView(context.clear.dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, context.clear.clearDepth, context.clear.clearStencil);
}