#pragma once

#include "renderer/render_pass.h"
#include "renderer/shader.h"
#include <DirectXMath.h>
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

struct renderable
{
    uint32_t meshIndex;
    uint32_t materialIndex;
};

struct scene_node
{
    std::string name;
    std::vector<ashenvale::scene::renderable> renderables;
    DirectX::XMFLOAT3 translation = {0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT4 rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    DirectX::XMFLOAT3 scale = {1.0f, 1.0f, 1.0f};

    DirectX::XMMATRIX worldMatrix;
};

inline std::vector<ashenvale::scene::mesh> g_meshes;
inline std::vector<ashenvale::scene::material> g_materials;
inline std::vector<ashenvale::scene::scene_node> g_nodes;
} // namespace ashenvale::scene

namespace ashenvale::scene
{
void load_scene(const char *path);
void close_scene();
void material_set_constant_buffer(material &mat, const std::string &name, winrt::com_ptr<ID3D11Buffer> buffer);
void material_set_texture(material &mat, const std::string &name, winrt::com_ptr<ID3D11ShaderResourceView> srv);
void material_set_sampler(material &mat, const std::string &name, winrt::com_ptr<ID3D11SamplerState> sampler);
material_resource *material_find_resource(material &mat, const std::string &name);
const material_resource *material_find_resource(const material &mat, const std::string &name);
void material_bind(const material &mat, ID3D11DeviceContext *context);
void update_world_matrix(ashenvale::scene::scene_node &node);
} // namespace ashenvale::scene