#include "debug_depth_pass.h"
#include "renderer/device.h"
#include "renderer/renderer.h"
#include "renderer/shader.h"

using namespace winrt;

namespace
{
com_ptr<ID3D11RasterizerState> g_debugDepthRasterize;
com_ptr<ID3D11DepthStencilState> g_depthStencilState;
com_ptr<ID3D11SamplerState> g_sampler;
} // namespace

void ashenvale::renderer::render_pass::debug_depth::initialize()
{
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthClipEnable = true;
    renderer::device::g_device->CreateRasterizerState(&rasterDesc, g_debugDepthRasterize.put());

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = false;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    renderer::device::g_device->CreateDepthStencilState(&dsDesc, g_depthStencilState.put());

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    renderer::device::g_device->CreateSamplerState(&sampDesc, g_sampler.put());

}

void ashenvale::renderer::render_pass::debug_depth::execute(const render_pass_context &context)
{
    reset_pipeline();

    const float color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    ID3D11RenderTargetView *const viewportRtvs[] = {context.debug_depth.rtv};
    ashenvale::renderer::device::g_context->IASetInputLayout(shader::g_quadShader.inputLayout.get());
    ashenvale::renderer::device::g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    ashenvale::renderer::device::g_context->OMSetRenderTargets(1, viewportRtvs, nullptr);
    ashenvale::renderer::device::g_context->ClearRenderTargetView(context.debug_depth.rtv, color);
    ashenvale::renderer::device::g_context->RSSetViewports(1, &context.debug_depth.viewport);
    ashenvale::renderer::device::g_context->RSSetState(g_debugDepthRasterize.get());
    ashenvale::renderer::device::g_context->OMSetDepthStencilState(g_depthStencilState.get(), 0);

    ashenvale::renderer::device::g_context->VSSetShader(shader::g_quadShader.vertexShader.get(), 0, 0);
    ashenvale::renderer::device::g_context->PSSetShader(shader::g_quadShader.pixelShader.get(), 0, 0);


    ID3D11Buffer *nullBuffer = nullptr;
    UINT zero = 0;
    ashenvale::renderer::device::g_context->IASetVertexBuffers(0, 1, &nullBuffer, &zero, &zero);

    ID3D11SamplerState *const samplers[] = {g_sampler.get()};
    ashenvale::renderer::device::g_context->PSSetSamplers(0, 1, samplers);
    ID3D11ShaderResourceView *const srvs[] = {renderer::g_viewportDepthSRV.get()};
    ashenvale::renderer::device::g_context->PSSetShaderResources(0, 1, srvs);

    ashenvale::renderer::device::g_context->Draw(4, 0);

    ID3D11ShaderResourceView *nullSRV = nullptr;
    ashenvale::renderer::device::g_context->PSSetShaderResources(0, 1, &nullSRV);
    ID3D11SamplerState *nullSampler = nullptr;
    ashenvale::renderer::device::g_context->PSSetSamplers(0, 1, &nullSampler);
}