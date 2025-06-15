#include "scene.h"
#include "renderer/device.h"
#include "editor/inspector.h"
#include "skydome.h"

#include "component.h"
#include "stb_image.h"
#include <cgltf.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <windows.h>
#include <winrt/base.h>

using namespace DirectX;

namespace
{

} // namespace

namespace
{
winrt::com_ptr<ID3D11ShaderResourceView> create_texture_from_gltf_image(const cgltf_image *image,
                                                                        const char *imagePathBase)
{
    if (!image || !image->uri)
        return nullptr;

    std::filesystem::path basePath(imagePathBase);
    std::filesystem::path fullPath = basePath.parent_path() / image->uri;

    std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        OutputDebugStringA(("Failed to open texture file: " + fullPath.string() + "\n").c_str());
        return nullptr;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(fileSize);
    if (!file.read(reinterpret_cast<char *>(buffer.data()), fileSize))
    {
        OutputDebugStringA(("Failed to read texture file: " + fullPath.string() + "\n").c_str());
        return nullptr;
    }

    int width, height, channels;
    stbi_set_flip_vertically_on_load(false);
    unsigned char *pixels =
        stbi_load_from_memory(buffer.data(), static_cast<int>(fileSize), &width, &height, &channels, 4);
    if (!pixels)
    {
        OutputDebugStringA("stb_image failed to decode texture image\n");
        return nullptr;
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 0;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    ;
    desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = pixels;
    initData.SysMemPitch = width * 4;

    winrt::com_ptr<ID3D11Texture2D> texture;
    HRESULT hr = ashenvale::renderer::device::g_device->CreateTexture2D(&desc, nullptr, texture.put());

    if (FAILED(hr))
    {
        OutputDebugStringA("Failed to create D3D11 texture from image\n");
        return nullptr;
    }

    // Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = -1;

    winrt::com_ptr<ID3D11ShaderResourceView> srv;
    hr = ashenvale::renderer::device::g_device->CreateShaderResourceView(texture.get(), &srvDesc, srv.put());

    if (FAILED(hr))
    {
        OutputDebugStringA("Failed to create SRV for texture\n");
        return nullptr;
    }

    ashenvale::renderer::device::g_context->UpdateSubresource(texture.get(), 0, nullptr, pixels, width * 4, 0);
    ashenvale::renderer::device::g_context->GenerateMips(srv.get());
    stbi_image_free(pixels);

    return srv;
}

winrt::com_ptr<ID3D11Buffer> create_material_constant_buffer(const cgltf_material &cgMat)
{

    ashenvale::scene::material_constants constants = {};

    memcpy(&constants.baseColorFactor, cgMat.pbr_metallic_roughness.base_color_factor, sizeof(float) * 4);
    constants.metallicFactor = cgMat.pbr_metallic_roughness.metallic_factor;
    constants.roughnessFactor = cgMat.pbr_metallic_roughness.roughness_factor;
    constants.normalScale = cgMat.normal_texture.scale;

    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = sizeof(ashenvale::scene::material_constants);
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = &constants;

    winrt::com_ptr<ID3D11Buffer> buffer;
    HRESULT hr = ashenvale::renderer::device::g_device->CreateBuffer(&desc, &data, buffer.put());

    return SUCCEEDED(hr) ? buffer : nullptr;
}

winrt::com_ptr<ID3D11SamplerState> create_default_sampler()
{
    D3D11_SAMPLER_DESC desc = {};
    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.MaxAnisotropy = 16;
    desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    desc.MinLOD = 0;
    desc.MaxLOD = D3D11_FLOAT32_MAX;

    winrt::com_ptr<ID3D11SamplerState> sampler;
    HRESULT hr = ashenvale::renderer::device::g_device->CreateSamplerState(&desc, sampler.put());

    return SUCCEEDED(hr) ? sampler : nullptr;
}

} // namespace

void ashenvale::scene::initialize()
{
    skydome::initialize();
}

void ashenvale::scene::load_scene(const char *path)
{
    cgltf_data *data = nullptr;
    cgltf_options options = {};

    if (cgltf_parse_file(&options, path, &data) == cgltf_result_success)
    {
        cgltf_load_buffers(&options, data, path);

        for (size_t i = 0; i < data->nodes_count; i++)
        {
            const cgltf_node &node = data->nodes[i];

            auto e = g_world.entity();

            scene::transform tc{};
            if (node.has_translation)
                memcpy(&tc.position, node.translation, sizeof(float) * 3);
            else
                tc.position = {0.f, 0.f, 0.f};

            if (node.has_rotation)
                memcpy(&tc.rotation, node.rotation, sizeof(float) * 4);
            else
                tc.rotation = {0.f, 0.f, 0.f, 1.f};

            if (node.has_scale)
                memcpy(&tc.scale, node.scale, sizeof(float) * 3);
            else
                tc.scale = {1.f, 1.f, 1.f};

            e.add<scene::transform>();
            e.set<scene::transform>(tc);
            e.add<scene::name>();
            if (node.mesh && node.mesh->primitives_count > 0)
            {
                scene::mesh_renderer mrc = {};
                for (size_t j = 0; j < node.mesh->primitives_count; ++j)
                {
                    scene::name name;
                    if (node.name)
                    {
                        name.name = node.name;
                        e.set<scene::name>(name);
                    }
                    else
                    {
                        name.name = "Unknown";
                        e.set<scene::name>(name);
                    }

                    const cgltf_primitive &primitive = node.mesh->primitives[j];

                    std::vector<vertex> vertices;
                    std::vector<uint32_t> indices;

                    size_t vertexCount = 0;
                    cgltf_accessor *posAccessor = nullptr;

                    for (size_t k = 0; k < primitive.attributes_count; ++k)
                    {
                        const cgltf_attribute &attr = primitive.attributes[k];
                        if (attr.type == cgltf_attribute_type_position)
                        {
                            posAccessor = attr.data;
                            vertexCount = posAccessor->count;
                            break;
                        }
                    }

                    vertices.resize(vertexCount);

                    for (size_t k = 0; k < primitive.attributes_count; ++k)
                    {
                        const cgltf_attribute &attr = primitive.attributes[k];
                        const cgltf_accessor *accessor = attr.data;
                        const cgltf_buffer_view *view = accessor->buffer_view;
                        const uint8_t *base =
                            static_cast<const uint8_t *>(view->buffer->data) + view->offset + accessor->offset;

                        for (size_t v = 0; v < accessor->count; ++v)
                        {
                            const float *data = reinterpret_cast<const float *>(base + accessor->stride * v);
                            switch (attr.type)
                            {
                            case cgltf_attribute_type_position:
                                memcpy(&vertices[v].position, data, sizeof(float) * 3);
                                break;
                            case cgltf_attribute_type_texcoord:
                                memcpy(&vertices[v].uv, data, sizeof(float) * 2);
                                break;
                            case cgltf_attribute_type_normal:
                                memcpy(&vertices[v].normal, data, sizeof(float) * 3);
                                break;
                            case cgltf_attribute_type_tangent:
                                memcpy(&vertices[v].tangent, data, sizeof(float) * 3);
                                break;
                            default:
                                break;
                            }
                        }
                    }

                    if (primitive.indices)
                    {
                        const cgltf_accessor *accessor = primitive.indices;
                        const cgltf_buffer_view *view = accessor->buffer_view;
                        const uint8_t *base =
                            static_cast<const uint8_t *>(view->buffer->data) + view->offset + accessor->offset;

                        for (size_t k = 0; k < accessor->count; ++k)
                        {
                            uint32_t index = 0;
                            switch (accessor->component_type)
                            {
                            case cgltf_component_type_r_16u:
                                index = reinterpret_cast<const uint16_t *>(base)[k];
                                break;
                            case cgltf_component_type_r_32u:
                                index = reinterpret_cast<const uint32_t *>(base)[k];
                                break;
                            case cgltf_component_type_r_8u:
                                index = reinterpret_cast<const uint8_t *>(base)[k];
                                break;
                            default:
                                OutputDebugStringA("Unsupported index format.\n");
                                break;
                            }

                            indices.push_back(index);
                        }
                    }

                    mesh m;
                    {
                        D3D11_BUFFER_DESC desc = {};
                        desc.Usage = D3D11_USAGE_DEFAULT;
                        desc.ByteWidth = UINT(vertices.size() * sizeof(vertex));
                        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

                        D3D11_SUBRESOURCE_DATA data = {};
                        data.pSysMem = vertices.data();

                        renderer::device::g_device->CreateBuffer(&desc, &data, m.vertexBuffer.put());
                    }

                    {
                        D3D11_BUFFER_DESC desc = {};
                        desc.Usage = D3D11_USAGE_DEFAULT;
                        desc.ByteWidth = UINT(indices.size() * sizeof(uint32_t));
                        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

                        D3D11_SUBRESOURCE_DATA data = {};
                        data.pSysMem = indices.data();

                        renderer::device::g_device->CreateBuffer(&desc, &data, m.indexBuffer.put());
                    }

                    m.indexCount = indices.size();

                    material mat = {};
                    if (primitive.material)
                    {
                        const cgltf_material &cgMat = *primitive.material;
                        if (cgMat.name)
                        {
                            mat.name = cgMat.name;
                        }
                        else
                        {
                            mat.name = "Unknown Material";
                        }
                        mat.shader = &renderer::shader::g_pbrShader;

                        if (cgMat.pbr_metallic_roughness.base_color_texture.texture)
                        {
                            const cgltf_texture *tex = cgMat.pbr_metallic_roughness.base_color_texture.texture;
                            if (tex->image)
                            {
                                winrt::com_ptr<ID3D11ShaderResourceView> srv =
                                    create_texture_from_gltf_image(tex->image, tex->image->uri);
                                if (srv)
                                {
                                    material_set_texture(mat, "diffuseTexture", srv);
                                }
                            }
                        }

                        if (cgMat.normal_texture.texture)
                        {
                            const cgltf_texture *tex = cgMat.normal_texture.texture;
                            if (tex->image)
                            {
                                winrt::com_ptr<ID3D11ShaderResourceView> srv =
                                    create_texture_from_gltf_image(tex->image, tex->image->uri);
                                if (srv)
                                {
                                    material_set_texture(mat, "normalTexture", srv);
                                }
                            }
                        }

                        winrt::com_ptr<ID3D11Buffer> materialCB = create_material_constant_buffer(cgMat);
                        if (materialCB)
                        {
                            material_set_constant_buffer(mat, "MaterialConstants", materialCB);
                        }

                        winrt::com_ptr<ID3D11SamplerState> defaultSampler = create_default_sampler();
                        if (defaultSampler)
                        {
                            material_set_sampler(mat, "defaultSampler", defaultSampler);
                        }

                        mrc.meshes.push_back(m);
                        mrc.materials.push_back(mat);
                    }

                }
                e.add<scene::mesh_renderer>();
                e.set<scene::mesh_renderer>(mrc);
            }
        }

        cgltf_free(data);
    }
}

void ashenvale::scene::close_scene()
{
    g_world.reset();
    editor::inspector::g_selectedEntity = flecs::entity::null();
}

namespace ashenvale::scene
{

void material_set_constant_buffer(material &mat, const std::string &name, winrt::com_ptr<ID3D11Buffer> buffer)
{
    material_resource *resource = material_find_resource(mat, name);
    if (resource)
    {
        resource->constantBuffer = buffer;
        return;
    }

    material_resource newResource;
    newResource.name = name;
    newResource.constantBuffer = buffer;
    mat.resources.push_back(newResource);
}

void material_set_texture(material &mat, const std::string &name, winrt::com_ptr<ID3D11ShaderResourceView> srv)
{
    material_resource *resource = material_find_resource(mat, name);
    if (resource)
    {
        resource->srv = srv;
        return;
    }

    material_resource newResource;
    newResource.name = name;
    newResource.srv = srv;
    mat.resources.push_back(newResource);
}

void material_set_sampler(material &mat, const std::string &name, winrt::com_ptr<ID3D11SamplerState> sampler)
{
    material_resource *resource = material_find_resource(mat, name);
    if (resource)
    {
        resource->sampler = sampler;
        return;
    }

    material_resource newResource;
    newResource.name = name;
    newResource.sampler = sampler;
    mat.resources.push_back(newResource);
}

material_resource *material_find_resource(material &mat, const std::string &name)
{
    for (auto &resource : mat.resources)
    {
        if (resource.name == name)
            return &resource;
    }
    return nullptr;
}

const material_resource *material_find_resource(const material &mat, const std::string &name)
{
    for (const auto &resource : mat.resources)
    {
        if (resource.name == name)
            return &resource;
    }
    return nullptr;
}

void material_bind(const material &mat, ID3D11DeviceContext *context)
{
    if (!mat.shader)
        return;

    // Bind shader
    context->VSSetShader(mat.shader->vertexShader.get(), nullptr, 0);
    context->PSSetShader(mat.shader->pixelShader.get(), nullptr, 0);
    context->IASetInputLayout(mat.shader->inputLayout.get());

    // Bind render state
    if (mat.rasterizerState)
        context->RSSetState(mat.rasterizerState.get());
    if (mat.blendState)
        context->OMSetBlendState(mat.blendState.get(), nullptr, 0xffffffff);
    if (mat.depthStencilState)
        context->OMSetDepthStencilState(mat.depthStencilState.get(), 0);

    // Bind vertex shader resources
    for (const auto &binding : mat.shader->vsBindings)
    {
        const material_resource *resource = material_find_resource(mat, binding.name);
        if (!resource)
            continue;

        switch (binding.type)
        {
        case D3D_SIT_CBUFFER:
            if (resource->constantBuffer)
            {
                ID3D11Buffer *cb = resource->constantBuffer.get();
                context->VSSetConstantBuffers(binding.slot, 1, &cb);
            }
            break;
        case D3D_SIT_TEXTURE:
            if (resource->srv)
            {
                ID3D11ShaderResourceView *srv = resource->srv.get();
                context->VSSetShaderResources(binding.slot, 1, &srv);
            }
            break;
        case D3D_SIT_SAMPLER:
            if (resource->sampler)
            {
                ID3D11SamplerState *sampler = resource->sampler.get();
                context->VSSetSamplers(binding.slot, 1, &sampler);
            }
            break;
        }
    }

    // Bind pixel shader resources
    for (const auto &binding : mat.shader->psBindings)
    {
        const material_resource *resource = material_find_resource(mat, binding.name);
        if (!resource)
            continue;

        switch (binding.type)
        {
        case D3D_SIT_CBUFFER:
            if (resource->constantBuffer)
            {
                ID3D11Buffer *cb = resource->constantBuffer.get();
                context->PSSetConstantBuffers(binding.slot, 1, &cb);
            }
            break;
        case D3D_SIT_TEXTURE:
            if (resource->srv)
            {
                ID3D11ShaderResourceView *srv = resource->srv.get();
                context->PSSetShaderResources(binding.slot, 1, &srv);
            }
            break;
        case D3D_SIT_SAMPLER:
            if (resource->sampler)
            {
                ID3D11SamplerState *sampler = resource->sampler.get();
                context->PSSetSamplers(binding.slot, 1, &sampler);
            }
            break;
        }
    }
}
} // namespace ashenvale::scene