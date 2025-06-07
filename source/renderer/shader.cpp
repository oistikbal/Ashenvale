#include "shader.h"
#include "renderer/device.h"
#include "shader_compiler.h"
#include <d3d11_4.h>
#include <winrt/base.h>

using namespace winrt;

namespace
{
void compile(ashenvale::renderer::shader::shader &shader, const wchar_t *vsPath, const wchar_t *psPath)
{
    com_ptr<ID3DBlob> errorBlob;
    com_ptr<ID3DBlob> vsBlob;
    com_ptr<ID3DBlob> psBlob;

    if (vsPath != nullptr)
    {
        ashenvale::renderer::shader_compiler::compile(vsPath, "main", "vs_5_0", nullptr, vsBlob.put(), errorBlob.put());

        ashenvale::renderer::device::g_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                                                  nullptr, shader.vertexShader.put());

        com_ptr<ID3D11ShaderReflection> reflection;
        D3DReflect(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), IID_PPV_ARGS(reflection.put()));

        auto layouts = ashenvale::renderer::shader_compiler::get_input_layout(reflection.get());

        ashenvale::renderer::device::g_device->CreateInputLayout(layouts.data(), layouts.size(),
                                                                 vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                                                 shader.inputLayout.put());

        D3D11_SHADER_DESC shaderDesc = {};
        reflection->GetDesc(&shaderDesc);

        for (UINT i = 0; i < shaderDesc.BoundResources; ++i)
        {
            D3D11_SHADER_INPUT_BIND_DESC desc = {};
            if (SUCCEEDED(reflection->GetResourceBindingDesc(i, &desc)) && desc.Name)
            {
                ashenvale::renderer::shader::resource_binding binding;
                binding.name = desc.Name;
                binding.slot = desc.BindPoint;
                binding.bindCount = desc.BindCount;
                binding.type = desc.Type;
                shader.vsBindings.push_back(binding);
            }
        }
    }

    if (psPath != nullptr)
    {
        ashenvale::renderer::shader_compiler::compile(psPath, "main", "ps_5_0", nullptr, psBlob.put(), errorBlob.put());

        ashenvale::renderer::device::g_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
                                                                 nullptr, shader.pixelShader.put());

        com_ptr<ID3D11ShaderReflection> reflection;
        D3D11_SHADER_DESC shaderDesc = {};
        D3DReflect(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), IID_PPV_ARGS(reflection.put()));
        reflection->GetDesc(&shaderDesc);
        for (UINT i = 0; i < shaderDesc.BoundResources; ++i)
        {
            D3D11_SHADER_INPUT_BIND_DESC desc = {};
            if (SUCCEEDED(reflection->GetResourceBindingDesc(i, &desc)) && desc.Name)
            {
                ashenvale::renderer::shader::resource_binding binding;
                binding.name = desc.Name;
                binding.slot = desc.BindPoint;
                binding.bindCount = desc.BindCount;
                binding.type = desc.Type;
                shader.psBindings.push_back(binding);
            }
        }
    }
}
} // namespace

void ashenvale::renderer::shader::initialize()
{
    compile(g_triangleShader, L"vs.hlsl", L"ps.hlsl");
    compile(g_quadShader, L"quad_vs.hlsl", L"quad_ps.hlsl");
    compile(g_pbrShader, L"pbr_vs.hlsl", L"pbr_ps.hlsl");
    compile(g_wireframeShader, nullptr, L"wireframe_ps.hlsl");
}