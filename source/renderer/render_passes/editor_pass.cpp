#include "editor_pass.h"
#include "editor/editor.h"
#include "renderer/device.h"
#include "renderer/renderer.h"

namespace
{
ashenvale::renderer::render_pass::render_pass_pso g_pso;
}

void ashenvale::renderer::render_pass::editor::initialize()
{
    g_pso = {};
}

void ashenvale::renderer::render_pass::editor::execute(const render_pass_context &context)
{
    bind_pso(g_pso);

    ID3D11RenderTargetView *const rtvs[] = {context.editor.rtv};
    ashenvale::renderer::device::g_context->OMSetRenderTargets(1, rtvs, nullptr);
    ashenvale::renderer::device::g_context->RSSetViewports(1, &context.editor.viewport);
    ashenvale::editor::render();
}