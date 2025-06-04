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

void ashenvale::renderer::render_pass::present::execute(const render_pass_context& context)
{
	bind_pso(g_pso);
	ID3D11RenderTargetView* const rtvs[] = { ashenvale::renderer::g_viewportRTV.get()};
	ashenvale::renderer::device::g_context->OMSetRenderTargets(1, rtvs, nullptr);
	ashenvale::renderer::device::g_context->RSSetViewports(1, &ashenvale::renderer::g_viewportViewport);
	swapchain::g_swapChain->Present(1, 0);
}