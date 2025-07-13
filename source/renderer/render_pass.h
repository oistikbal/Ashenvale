#pragma once

#include <d3d11_4.h>
#include <stdint.h>

#include "render_graph.h"

namespace ashenvale::renderer::render_pass
{

struct clear_pass_context
{
    ID3D11RenderTargetView *rtv;
    ID3D11DepthStencilView *dsv;
    float clearColor[4];
    float clearDepth;
    uint8_t clearStencil;
};

struct geometry_pass_context
{
    ID3D11RenderTargetView *rtv;
    ID3D11DepthStencilView *dsv;
};

struct editor_pass_context
{
    ID3D11RenderTargetView *rtv;
    float clearColor[4];
    D3D11_VIEWPORT viewport;
};

struct present_pass_context
{
};

struct normal_pass_context
{
    ID3D11RenderTargetView *rtv;
    ID3D11DepthStencilView *dsv;
};

struct tonemap_pass_context
{
    ID3D11RenderTargetView *rtv;
};

struct debug_depth_pass_context
{
    ID3D11RenderTargetView *rtv;
    D3D11_VIEWPORT viewport;
};

struct debug_wireframe_pass_context
{
    ID3D11RenderTargetView *rtv;
    ID3D11DepthStencilView *dsv;
    bool clear;
};

union render_pass_context {
    clear_pass_context clear;
    geometry_pass_context geometry;
    editor_pass_context editor;
    present_pass_context present;
    debug_depth_pass_context debug_depth;
    debug_wireframe_pass_context debug_wireframe;
    normal_pass_context normal;
    tonemap_pass_context tonemap;
};

using render_pass_execute = void (*)(const render_pass_context &);

struct render_pass_info
{
    render_pass_context context;
    render_pass_execute execute;

    render_pass_info() : context{.clear = {}}, execute(nullptr)
    {
    }
};
} // namespace ashenvale::renderer::render_pass

namespace ashenvale::renderer::render_pass
{
void initialize();
void resize();
void reset_pipeline();
void render();
} // namespace ashenvale::renderer::render_pass