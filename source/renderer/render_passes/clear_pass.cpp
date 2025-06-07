#include "clear_pass.h"
#include "renderer/device.h"
#include "renderer/renderer.h"

namespace
{
}

void ashenvale::renderer::render_pass::clear::initialize()
{
}

void ashenvale::renderer::render_pass::clear::execute(const render_pass_context &context)
{
    reset_pipeline();
    ashenvale::renderer::device::g_context->ClearRenderTargetView(context.clear.rtv, context.clear.clearColor);
    ashenvale::renderer::device::g_context->ClearDepthStencilView(context.clear.dsv,
                                                                  D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                                                  context.clear.clearDepth, context.clear.clearStencil);
}