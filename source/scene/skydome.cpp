#include "skydome.h"
#include "renderer/device.h"
#include "renderer/shader.h"
#include <filesystem>
#include <fstream>
#include <renderer/camera.h>
#include <stb_image.h>

namespace
{
struct vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 uv;
};

winrt::com_ptr<ID3D11Buffer> g_cameraBuffer;
winrt::com_ptr<ID3D11Buffer> g_vertexBuffer;
winrt::com_ptr<ID3D11Buffer> g_indexBuffer;
winrt::com_ptr<ID3D11RasterizerState> g_rasterizer;
winrt::com_ptr<ID3D11ShaderResourceView> g_skydomeSrv;
winrt::com_ptr<ID3D11SamplerState> g_sampler;
winrt::com_ptr<ID3D11DepthStencilState> g_depthStencilState;

uint32_t g_indexCount;

bool g_isLoaded;
} // namespace

void ashenvale::scene::skydome::initialize()
{
    const int latitudeBands = 8;
    const int longitudeBands = 8;
    const float radius = 1;

    std::vector<vertex> vertices;
    std::vector<uint32_t> indices;

    for (int latNumber = 0; latNumber <= latitudeBands; ++latNumber)
    {
        float theta = latNumber * DirectX::XM_PI / latitudeBands;
        float sinTheta = sinf(theta);
        float cosTheta = cosf(theta);

        for (int longNumber = 0; longNumber <= longitudeBands; ++longNumber)
        {
            float phi = longNumber * DirectX::XM_2PI / longitudeBands;
            float sinPhi = sinf(phi);
            float cosPhi = cosf(phi);

            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;

            float u = 1 - ((float)longNumber / longitudeBands);
            float v = (float)latNumber / latitudeBands;

            vertices.push_back({DirectX::XMFLOAT3(x * radius, y * radius, z * radius), DirectX::XMFLOAT2(u, v)});
        }
    }

    // Generate indices (two triangles per quad)
    for (int latNumber = 0; latNumber < latitudeBands; ++latNumber)
    {
        for (int longNumber = 0; longNumber < longitudeBands; ++longNumber)
        {
            int first = (latNumber * (longitudeBands + 1)) + longNumber;
            int second = first + longitudeBands + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.ByteWidth = UINT(vertices.size() * sizeof(vertex));

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices.data();
    renderer::device::g_device->CreateBuffer(&vbDesc, &vbData, g_vertexBuffer.put());

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
    ibDesc.ByteWidth = UINT(indices.size() * sizeof(uint32_t));
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices.data();
    renderer::device::g_device->CreateBuffer(&ibDesc, &ibData, g_indexBuffer.put());

    g_indexCount = indices.size();

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_FRONT;
    rasterDesc.FrontCounterClockwise = true;
    rasterDesc.DepthClipEnable = true;
    renderer::device::g_device->CreateRasterizerState(&rasterDesc, g_rasterizer.put());

    D3D11_BUFFER_DESC cameraBufferDesc = {};
    cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    cameraBufferDesc.ByteWidth = sizeof(ashenvale::renderer::camera::mvp_buffer);
    cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    renderer::device::g_device->CreateBuffer(&cameraBufferDesc, nullptr, g_cameraBuffer.put());

    D3D11_SAMPLER_DESC desc = {};
    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    desc.Filter = D3D11_FILTER_ANISOTROPIC;

    ashenvale::renderer::device::g_device->CreateSamplerState(&desc, g_sampler.put());

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = false;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    dsDesc.StencilEnable = false;

    renderer::device::g_device->CreateDepthStencilState(&dsDesc, g_depthStencilState.put());
}

void ashenvale::scene::skydome::load_hdri_skydome(const char *path)
{

    std::filesystem::path basePath(path);

    std::ifstream file(path, std::ios::binary | std::ios::ate);

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(fileSize);
    file.read(reinterpret_cast<char *>(buffer.data()), fileSize);

    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    float *pixels = stbi_loadf_from_memory(buffer.data(), static_cast<int>(fileSize), &width, &height, &channels, 4);

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 0;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = pixels;
    initData.SysMemPitch = width * sizeof(float) * 4;

    winrt::com_ptr<ID3D11Texture2D> texture;
    HRESULT hr = ashenvale::renderer::device::g_device->CreateTexture2D(&desc, nullptr, texture.put());

    ashenvale::renderer::device::g_context->UpdateSubresource(texture.get(), 0, nullptr, pixels,
                                                              width * sizeof(float) * 4, 0);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = -1;

    hr = ashenvale::renderer::device::g_device->CreateShaderResourceView(texture.get(), &srvDesc, g_skydomeSrv.put());

    ashenvale::renderer::device::g_context->GenerateMips(g_skydomeSrv.get());

    stbi_image_free(pixels);

    g_isLoaded = true;
}

void ashenvale::scene::skydome::render()
{
    if (!g_isLoaded)
    {
        return;
    }

    auto context = ashenvale::renderer::device::g_context;

    DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();

    DirectX::XMMATRIX view = renderer::camera::g_viewMatrix;

    view.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

    renderer::camera::mvp_buffer mvp = {};
    DirectX::XMStoreFloat4x4(&mvp.world, DirectX::XMMatrixTranspose(world));
    DirectX::XMStoreFloat4x4(&mvp.view, XMMatrixTranspose(view));

    DirectX::XMStoreFloat4x4(&mvp.projection, DirectX::XMMatrixTranspose(renderer::camera::g_projectionMatrix));

    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    renderer::device::g_context->Map(g_cameraBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    memcpy(mappedResource.pData, &mvp, sizeof(mvp));
    renderer::device::g_context->Unmap(g_cameraBuffer.get(), 0);

    ID3D11Buffer *const cameraBuffer[] = {g_cameraBuffer.get()};
    renderer::device::g_context->VSSetConstantBuffers(0, 1, cameraBuffer);

    context->RSSetState(g_rasterizer.get());
    context->OMSetDepthStencilState(g_depthStencilState.get(), 0);

    context->VSSetShader(renderer::shader::g_skydomeShader.vertexShader.get(), nullptr, 0);
    context->PSSetShader(renderer::shader::g_skydomeShader.pixelShader.get(), nullptr, 0);

    context->IASetInputLayout(renderer::shader::g_skydomeShader.inputLayout.get());

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UINT stride = sizeof(vertex);
    UINT offset = 0;

    ID3D11Buffer *const vertexBuffers[] = {g_vertexBuffer.get()};
    context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);

    context->IASetIndexBuffer(g_indexBuffer.get(), DXGI_FORMAT_R32_UINT, 0);

    ID3D11ShaderResourceView *const srvs[] = {g_skydomeSrv.get()};
    ID3D11SamplerState *sampler = g_sampler.get();
    context->PSSetShaderResources(0, 1, srvs);
    context->PSSetSamplers(0, 1, &sampler);

    context->DrawIndexed(g_indexCount, 0, 0);

    context->RSSetState(nullptr);
    context->OMSetDepthStencilState(nullptr, 0);
}

void ashenvale::scene::skydome::close()
{
    g_isLoaded = false;

    g_skydomeSrv = nullptr;
}