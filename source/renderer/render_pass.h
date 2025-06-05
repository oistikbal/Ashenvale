#pragma once

#include <d3d11_4.h>
#include <stdint.h>

#include "render_graph.h"

namespace ashenvale::renderer::render_pass
{
	struct render_pass_pso
	{
		ID3D11InputLayout* inputLayout = nullptr;
		ID3D11VertexShader* vs = nullptr;
		ID3D11PixelShader* ps = nullptr;
		ID3D11RasterizerState* rs = nullptr;
		ID3D11BlendState* bs = nullptr;
		ID3D11DepthStencilState* dss = nullptr;
		D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	};

	struct resource_definition
    {
        enum class input_type
        {
            constant,
            srv,
            sampler
        };
        input_type type;
        UINT slot;
    };

	struct resource_bindings
    {
        resource_definition vsResources[160];
        resource_definition psResources[160];
        int vsCount = 0;
        int psCount = 0;
    };

	struct bound_resources
    {
        ID3D11Buffer *vsCBs[16] = {nullptr};
        ID3D11ShaderResourceView *vsSRVs[128] = {nullptr};
        ID3D11SamplerState *vsSamplers[16] = {nullptr};

        ID3D11Buffer *psCBs[16] = {nullptr};
        ID3D11ShaderResourceView *psSRVs[128] = {nullptr};
        ID3D11SamplerState *psSamplers[16] = {nullptr};
    };

	struct clear_pass_context
	{
		ID3D11RenderTargetView* rtv;
		ID3D11DepthStencilView* dsv;
		float clearColor[4];
		float clearDepth;
		uint8_t clearStencil;
	};

	struct geometry_pass_context
	{
		ID3D11RenderTargetView* rtv;
		ID3D11DepthStencilView* dsv;
	};

	struct editor_pass_context
	{
		ID3D11RenderTargetView* rtv;
		float clearColor[4];
		D3D11_VIEWPORT viewport;
	};

	struct present_pass_context
	{
	};

	struct debug_depth_pass_context
	{
		ID3D11RenderTargetView* rtv;
		D3D11_VIEWPORT viewport;
	};

	struct debug_wireframe_pass_context
	{
		ID3D11RenderTargetView* rtv;
		ID3D11DepthStencilView* dsv;
		bool clear;
	};


	union render_pass_context
	{
		clear_pass_context clear;
		geometry_pass_context geometry;
		editor_pass_context editor;
		present_pass_context present;
		debug_depth_pass_context debug_depth;
		debug_wireframe_pass_context debug_wireframe;
	};

	using render_pass_execute = void(*)(const render_pass_context&);

	struct render_pass_info
	{
		render_pass_context context;
		render_pass_execute execute;

		render_pass_info() : context{ .clear = {} }, execute(nullptr) {}
	};
}

namespace ashenvale::renderer::render_pass
{
	void initialize();
	void resize();
	void bind_pso(const render_pass_pso& pso);
    void bind_resources(const resource_bindings &defs, const bound_resources &res);
	void render();
}