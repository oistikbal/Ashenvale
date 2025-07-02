#pragma once

#include <d3d11_4.h>
#include <string>
#include <winrt/base.h>

namespace ashenvale::renderer::shader
{
struct resource_binding
{
    std::string name;
    D3D_SHADER_INPUT_TYPE type;
    uint32_t slot;
    uint32_t bindCount;
};

struct shader
{
    winrt::com_ptr<ID3D11VertexShader> vertexShader;
    winrt::com_ptr<ID3D11PixelShader> pixelShader;
    winrt::com_ptr<ID3D11InputLayout> inputLayout;

    std::vector<resource_binding> vsBindings;
    std::vector<resource_binding> psBindings;
};

struct gs_shader
{
    winrt::com_ptr<ID3D11VertexShader> vertexShader;
    winrt::com_ptr<ID3D11GeometryShader> geometryShader;
    winrt::com_ptr<ID3D11PixelShader> pixelShader;
    winrt::com_ptr<ID3D11InputLayout> inputLayout;


    std::vector<resource_binding> vsBindings;
    std::vector<resource_binding> gsBindings;
    std::vector<resource_binding> psBindings;
};

inline shader g_quadShader;
inline shader g_pbrShader;
inline shader g_wireframeShader;
inline shader g_skydomeShader;
inline shader g_shadowShader;
inline gs_shader g_normalShader;
} // namespace ashenvale::renderer::shader

namespace ashenvale::renderer::shader
{
void initialize();
}