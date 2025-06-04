#pragma once
#include <cstdint>
#include <d3d11_4.h>

#include "render_pass.h"

namespace ashenvale::renderer::render_graph
{
	enum class debug_view : uint8_t
	{
		none,
		depth,
		wireframe,
		wireframe_lit
	};

	enum class render_path : uint8_t
	{
		forward,
		deferred
	};

	inline render_path g_renderPath = render_path::forward;
	inline debug_view g_debugView = debug_view::none;
}

namespace ashenvale::renderer::render_graph
{
	void initialize();
	void render();
	void resize();
}