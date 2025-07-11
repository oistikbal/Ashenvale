#pragma once

#include "renderer/render_pass.h"
#include "renderer/shader.h"
#include <DirectXMath.h>
#include <flecs.h>
#include <winrt/base.h>

namespace ashenvale::scene
{
struct vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 uv;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT3 tangent;
};

struct mesh
{
    winrt::com_ptr<ID3D11Buffer> vertexBuffer = nullptr;
    winrt::com_ptr<ID3D11Buffer> indexBuffer = nullptr;
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;
};

struct material_resource
{
    std::string name;
    winrt::com_ptr<ID3D11Buffer> constantBuffer = nullptr;
    winrt::com_ptr<ID3D11ShaderResourceView> srv = nullptr;
    winrt::com_ptr<ID3D11SamplerState> sampler = nullptr;
};

struct material
{
    std::string name;
    renderer::shader::shader *shader = nullptr;

    std::vector<material_resource> resources;

    winrt::com_ptr<ID3D11RasterizerState> rasterizerState = nullptr;
    winrt::com_ptr<ID3D11BlendState> blendState = nullptr;
    winrt::com_ptr<ID3D11DepthStencilState> depthStencilState = nullptr;
};

struct material_constants
{
    DirectX::XMFLOAT4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float padding;
};

struct name
{
    std::string name;
};

struct alignas(64) transform
{
    transform *parent = nullptr;
    DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT4 rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    DirectX::XMFLOAT3 scale = {1.0f, 1.0f, 1.0f};
};

struct mesh_renderer
{
    std::vector<mesh> meshes;
    std::vector<material> materials;
};

struct light
{
    enum class light_type : uint8_t
    {
        directional,
        point,
        spot,
    };

    DirectX::XMFLOAT3 color;
    float intensity;
    light_type type;
    float range;
    float spot_inner_cone_angle;
    float spot_outer_cone_angle;

    winrt::com_ptr<ID3D11Texture2D> shadowMap;
    winrt::com_ptr<ID3D11ShaderResourceView> shadowSrv;
    winrt::com_ptr<ID3D11DepthStencilView> shadowDepthView;
};

struct light_buffer
{
    DirectX::XMFLOAT3 position;
    float pad1;
    DirectX::XMFLOAT3 color;
    float intensity;
    uint32_t light_type;
    float linear;
    float quadratic;
    float range; 
    float spot_inner_cone_angle;
    float spot_outer_cone_angle;
    float pad2[2];
};

struct light_meta
{
    uint32_t lightCount;
    float padding[3];
};

inline flecs::world g_world;
} // namespace ashenvale::scene

namespace ashenvale::scene
{
void initialize();
void load_scene(const char *path);
void close_scene();
void material_set_constant_buffer(material &mat, const std::string &name, winrt::com_ptr<ID3D11Buffer> buffer);
void material_set_texture(material &mat, const std::string &name, winrt::com_ptr<ID3D11ShaderResourceView> srv);
void material_set_sampler(material &mat, const std::string &name, winrt::com_ptr<ID3D11SamplerState> sampler);
material_resource *material_find_resource(material &mat, const std::string &name);
const material_resource *material_find_resource(const material &mat, const std::string &name);
void material_bind(const material &mat, ID3D11DeviceContext *context);
} // namespace ashenvale::scene