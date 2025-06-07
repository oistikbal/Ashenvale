#include "editor_pass.h"
#include "editor/editor.h"
#include "renderer/device.h"
#include "renderer/renderer.h"

namespace
{

}

void ashenvale::renderer::render_pass::editor::initialize()
{

}

void ashenvale::renderer::render_pass::editor::execute(const render_pass_context &context)
{
    reset_pipeline();

    ID3D11RenderTargetView *const rtvs[] = {context.editor.rtv};
    ashenvale::renderer::device::g_context->OMSetRenderTargets(1, rtvs, nullptr);
    ashenvale::renderer::device::g_context->RSSetViewports(1, &context.editor.viewport);
    ashenvale::editor::render();
}