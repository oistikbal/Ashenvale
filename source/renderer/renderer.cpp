#include <DirectXMath.h>
#include <d3d11_4.h>

#include "camera.h"
#include "editor/editor.h"
#include "profiler/profiler.h"
#include "render_graph.h"
#include "renderer/device.h"
#include "renderer/renderer.h"
#include "renderer/shader_compiler.h"
#include "renderer/swapchain.h"

using namespace winrt;

void ashenvale::renderer::initialize()
{
    PIX_SCOPED_EVENT("renderer.initialize")

    renderer::camera::initialize();
    render_graph::initialize();
}

void ashenvale::renderer::resize_viewport(int width, int height)
{
    PIX_SCOPED_EVENT("renderer.resize_viewport")
    g_viewportTexture = nullptr;
    g_viewportRTV = nullptr;
    g_viewportSRV = nullptr;
    g_viewportDepthStencil = nullptr;
    g_viewportDSV = nullptr;
    g_viewportDepthStencilState = nullptr;
    g_viewportDepthSRV = nullptr;

    D3D11_TEXTURE2D_DESC colorDesc = {};
    colorDesc.Width = width;
    colorDesc.Height = height;
    colorDesc.MipLevels = 1;
    colorDesc.ArraySize = 1;
    colorDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    colorDesc.SampleDesc.Count = 1;
    colorDesc.Usage = D3D11_USAGE_DEFAULT;
    colorDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    renderer::device::g_device->CreateTexture2D(&colorDesc, nullptr, g_viewportTexture.put());
    renderer::device::g_device->CreateRenderTargetView(g_viewportTexture.get(), nullptr, g_viewportRTV.put());
    renderer::device::g_device->CreateShaderResourceView(g_viewportTexture.get(), nullptr, g_viewportSRV.put());

    D3D11_TEXTURE2D_DESC depthDesc = colorDesc;
    depthDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    renderer::device::g_device->CreateTexture2D(&depthDesc, nullptr, g_viewportDepthStencil.put());

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

    renderer::device::g_device->CreateDepthStencilView(g_viewportDepthStencil.get(), &dsvDesc, g_viewportDSV.put());

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    renderer::device::g_device->CreateDepthStencilState(&dsDesc, g_viewportDepthStencilState.put());

    dsDesc = {};
    dsDesc.DepthEnable = false;
    renderer::device::g_device->CreateDepthStencilState(&dsDesc, g_viewportDepthStencilStateDisabled.put());

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    renderer::device::g_device->CreateShaderResourceView(g_viewportDepthStencil.get(), &srvDesc,
                                                         g_viewportDepthSRV.put());

    g_viewportViewport = {};
    g_viewportViewport.Width = static_cast<float>(width);
    g_viewportViewport.Height = static_cast<float>(height);
    g_viewportViewport.MinDepth = 0.0f;
    g_viewportViewport.MaxDepth = 1.0f;
    g_viewportViewport.TopLeftX = 0.0f;
    g_viewportViewport.TopLeftY = 0.0f;

    render_graph::resize();
}

void ashenvale::renderer::render()
{
    PIX_SCOPED_EVENT("renderer.render")
    renderer::camera::update(90.0f, g_viewportViewport.Width / g_viewportViewport.Height, 0.1f, 1000.0f);
    render_graph::render();
}
