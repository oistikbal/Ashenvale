#include "render_graph.h"

void ashenvale::renderer::render_graph::initialize()
{
	renderer::render_pass::initialize();
}

void ashenvale::renderer::render_graph::render()
{
	switch (g_renderPath)
	{
		case ashenvale::renderer::render_graph::render_path::forward:
			for(int i = 0; i < 2; i++)
			{
				g_forwardPathPassInfos[i].execute(g_forwardPathPassInfos[i].context);
			}
			break;
		case ashenvale::renderer::render_graph::render_path::deferred:
			for (int i = 0; i < 2; i++)
			{
				g_forwardPathPassInfos[i].execute(g_forwardPathPassInfos[i].context);
			}
			break;
	}

	switch (g_debugView)
	{
		case ashenvale::renderer::render_graph::debug_view::none:
			break;
		case ashenvale::renderer::render_graph::debug_view::depth:
			g_debugDepthPassInfo.execute(g_debugDepthPassInfo.context);
			break;
		case ashenvale::renderer::render_graph::debug_view::wireframe:
			break;
		case ashenvale::renderer::render_graph::debug_view::wireframe_lit:
			break;
	}

	g_editorPassInfo.execute(g_debugDepthPassInfo.context);
	g_presentPassInfo.execute(g_presentPassInfo.context);
}