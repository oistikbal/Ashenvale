#include "render_pass.h"
#include "device.h"
#include "render_graph.h"
#include "renderer/renderer.h"
#include "swapchain.h"

#include "render_passes/clear_pass.h"
#include "render_passes/debug_depth_pass.h"
#include "render_passes/debug_wireframe_pass.h"
#include "render_passes/editor_pass.h"
#include "render_passes/geometry_pass.h"
#include "render_passes/present_pass.h"

using namespace winrt;

namespace
{
ashenvale::renderer::render_pass::render_pass_info g_clearPassInfo = {};
ashenvale::renderer::render_pass::render_pass_info g_geometryPassInfo = {};
ashenvale::renderer::render_pass::render_pass_info g_editorPassInfo = {};
ashenvale::renderer::render_pass::render_pass_info g_presentPassInfo = {};
ashenvale::renderer::render_pass::render_pass_info g_debugDepthPassInfo = {};
ashenvale::renderer::render_pass::render_pass_info g_debugWireframePassInfo = {};
} // namespace

void ashenvale::renderer::render_pass::initialize()
{
    clear::initialize();
    geometry::initialize();
    editor::initialize();
    present::initialize();
    debug_depth::initialize();
    debug_wireframe::initialize();

    resize();
}

void ashenvale::renderer::render_pass::resize()
{
    g_clearPassInfo.execute = clear::execute;
    clear_pass_context clearCtx = {};
    clearCtx.clearColor[0] = 0.0f;
    clearCtx.clearColor[1] = 0.0f;
    clearCtx.clearColor[2] = 0.0f;
    clearCtx.clearColor[3] = 1.0f;
    clearCtx.clearDepth = 1.0;
    clearCtx.clearStencil = 0.0;
    clearCtx.rtv = renderer::g_viewportRTV.get();
    clearCtx.dsv = renderer::g_viewportDSV.get();
    g_clearPassInfo.context.clear = clearCtx;

    g_geometryPassInfo.execute = geometry::execute;
    geometry_pass_context geometryCtx = {};
    geometryCtx.rtv = renderer::g_viewportRTV.get();
    geometryCtx.dsv = renderer::g_viewportDSV.get();
    g_geometryPassInfo.context.geometry = geometryCtx;

    g_editorPassInfo.execute = editor::execute;
    editor_pass_context editorCtx = {};
    editorCtx.rtv = renderer::swapchain::g_baseRTV.get();
    editorCtx.viewport = renderer::swapchain::g_viewport;
    editorCtx.clearColor[0] = 0.0f;
    editorCtx.clearColor[1] = 0.0f;
    editorCtx.clearColor[2] = 0.0f;
    editorCtx.clearColor[3] = 1.0f;
    g_editorPassInfo.context.editor = editorCtx;

    g_presentPassInfo.execute = present::execute;
    present_pass_context presentCtx = {};
    g_presentPassInfo.context.present = presentCtx;

    g_debugDepthPassInfo.execute = debug_depth::execute;
    debug_depth_pass_context debugDepthCtx = {};
    debugDepthCtx.rtv = renderer::g_viewportRTV.get();
    debugDepthCtx.viewport = renderer::g_viewportViewport;
    g_debugDepthPassInfo.context.debug_depth = debugDepthCtx;

    g_debugWireframePassInfo.execute = debug_wireframe::execute;
    debug_wireframe_pass_context debugWireframeCtx = {};
    debugWireframeCtx.rtv = renderer::g_viewportRTV.get();
    debugWireframeCtx.dsv = renderer::g_viewportDSV.get();
    g_debugWireframePassInfo.context.debug_wireframe = debugWireframeCtx;
}

void ashenvale::renderer::render_pass::bind_pso(const render_pass_pso &pso)
{
    renderer::device::g_context->IASetInputLayout(pso.inputLayout);
    renderer::device::g_context->VSSetShader(pso.vs, nullptr, 0);
    renderer::device::g_context->PSSetShader(pso.ps, nullptr, 0);
    renderer::device::g_context->RSSetState(pso.rs);
    renderer::device::g_context->OMSetBlendState(pso.bs, nullptr, 0xffffffff);
    renderer::device::g_context->OMSetDepthStencilState(pso.dss, 0);
    renderer::device::g_context->IASetPrimitiveTopology(pso.topology);
}

void ashenvale::renderer::render_pass::bind_resources(const resource_bindings &defs, const bound_resources &res)
{
    for (size_t i = 0; i < defs.vsCount; ++i)
    {
        const auto &def = defs.vsResources[i];
        switch (def.type)
        {
        case resource_definition::input_type::constant:
            renderer::device::g_context->VSSetConstantBuffers(def.slot, 1, &res.vsCBs[def.slot]);
            break;
        case resource_definition::input_type::srv:
            renderer::device::g_context->VSSetShaderResources(def.slot, 1, &res.vsSRVs[def.slot]);
            break;
        case resource_definition::input_type::sampler:
            renderer::device::g_context->VSSetSamplers(def.slot, 1, &res.vsSamplers[def.slot]);
            break;
        }
    }

    for (size_t i = 0; i < defs.psCount; ++i)
    {
        const auto &def = defs.psResources[i];
        switch (def.type)
        {
        case resource_definition::input_type::constant:
            renderer::device::g_context->PSSetConstantBuffers(def.slot, 1, &res.psCBs[def.slot]);
            break;
        case resource_definition::input_type::srv:
            renderer::device::g_context->PSSetShaderResources(def.slot, 1, &res.psSRVs[def.slot]);
            break;
        case resource_definition::input_type::sampler:
            renderer::device::g_context->PSSetSamplers(def.slot, 1, &res.psSamplers[def.slot]);
            break;
        }
    }
}

void ashenvale::renderer::render_pass::render()
{
    switch (render_graph::g_renderPath)
    {
    case ashenvale::renderer::render_graph::render_path::forward:
        g_clearPassInfo.execute(g_clearPassInfo.context);
        g_geometryPassInfo.execute(g_geometryPassInfo.context);
        break;
    case ashenvale::renderer::render_graph::render_path::deferred:
        g_clearPassInfo.execute(g_clearPassInfo.context);
        g_geometryPassInfo.execute(g_geometryPassInfo.context);
        break;
    }

    switch (render_graph::g_debugView)
    {
    case ashenvale::renderer::render_graph::debug_view::none:
        break;
    case ashenvale::renderer::render_graph::debug_view::depth:
        g_debugDepthPassInfo.execute(g_debugDepthPassInfo.context);
        break;
    case ashenvale::renderer::render_graph::debug_view::wireframe:
        g_debugWireframePassInfo.context.debug_wireframe.clear = true;
        g_debugWireframePassInfo.execute(g_debugWireframePassInfo.context);
        break;
    case ashenvale::renderer::render_graph::debug_view::wireframe_lit:
        g_debugWireframePassInfo.context.debug_wireframe.clear = false;
        g_debugWireframePassInfo.execute(g_debugWireframePassInfo.context);
        break;
    }

    g_editorPassInfo.execute(g_editorPassInfo.context);
    g_presentPassInfo.execute(g_presentPassInfo.context);
}
