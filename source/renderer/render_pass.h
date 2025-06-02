#pragma once

#include <d3d11_4.h>
#include <stdint.h>

namespace ashenvale::renderer::render_pass
{

	struct clear_pass_context
	{
		ID3D11RenderTargetView* rtvs[1];
		int rtvCount;
		ID3D11DepthStencilView* dsv;
		float clearColor[4];
		float clearDepth;
		uint8_t clearStencil;
	};

	struct geometry_pass_context
	{
		ID3D11RenderTargetView* rtvs[4];
		int rtvCount;
		ID3D11DepthStencilView* dsv;
	};

	struct editor_pass_context
	{
		ID3D11RenderTargetView* rtvs[4];
		int rtvCount;
		ID3D11DepthStencilView* dsv;
	};

	struct present_pass_context
	{
		ID3D11RenderTargetView* rtvs[1];
		int rtvCount = 1;
	};

	struct debug_depth_pass_context
	{
		ID3D11RenderTargetView* rtvs[1];
		int rtvCount = 1;
		ID3D11DepthStencilView* dsv;
	};

	struct debug_wireframe_pass_context
	{
		ID3D11RenderTargetView* rtvs[1];
		int rtvCount = 1;
		ID3D11DepthStencilView* dsv;
		ID3D11RasterizerState* rasterizerState;
	};

	struct debug_wireframe_shaded_pass_context
	{
		ID3D11RenderTargetView* rtvs[1];
		int rtvCount = 1;
		ID3D11DepthStencilView* dsv;
		ID3D11RasterizerState* rasterizerState;
	};

	union render_pass_context
	{
		clear_pass_context clear;
		geometry_pass_context geometry;
		editor_pass_context editor;
		present_pass_context present;
		debug_depth_pass_context debug_depth;
		debug_wireframe_pass_context debug_wireframe;
		debug_wireframe_shaded_pass_context debug_shaded;
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
}