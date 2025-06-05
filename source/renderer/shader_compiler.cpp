#include "renderer/shader_compiler.h"
#include "configs/shader.h"

#include <string>

using namespace winrt;

HRESULT ashenvale::renderer::shader_compiler::compile(const wchar_t *file, const char *entryPoint, const char *target,
                                                      const D3D_SHADER_MACRO *defines, ID3DBlob **shaderBlob,
                                                      ID3DBlob **errorBlob)
{

    std::wstring fullPath = std::wstring(config::SHADER_PATH) + file;

    HRESULT hr = D3DCompileFromFile(fullPath.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, target,
                                    D3DCOMPILE_ENABLE_STRICTNESS, 0, shaderBlob, errorBlob);

    return hr;
}

std::vector<D3D11_INPUT_ELEMENT_DESC> ashenvale::renderer::shader_compiler::get_input_layout(
    ID3D11ShaderReflection *reflection)
{
    D3D11_SHADER_DESC shaderDesc{};
    reflection->GetDesc(&shaderDesc);
    std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayout(shaderDesc.InputParameters);

    for (unsigned i = 0; i < shaderDesc.InputParameters; ++i)
    {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc{};
        reflection->GetInputParameterDesc(i, &paramDesc);

        D3D11_INPUT_ELEMENT_DESC elementDesc = {};

        elementDesc.SemanticName = paramDesc.SemanticName;
        elementDesc.SemanticIndex = paramDesc.SemanticIndex;

        switch (paramDesc.ComponentType)
        {
        case D3D_REGISTER_COMPONENT_FLOAT32:
            if (paramDesc.Mask == 1)
            {
                elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
            }
            else if (paramDesc.Mask == 3)
            {
                elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
            }
            else if (paramDesc.Mask == 7)
            {
                elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
            }
            else if (paramDesc.Mask == 15)
            {
                elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
            break;
        case D3D_REGISTER_COMPONENT_SINT32:
            break;
        case D3D_REGISTER_COMPONENT_UINT32:
            break;
        default:
            elementDesc.Format = DXGI_FORMAT_UNKNOWN;
            break;
        }

        elementDesc.InputSlot = 0;
        elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        elementDesc.InstanceDataStepRate = 0;

        inputLayout[i] = elementDesc;
    }

    return inputLayout;
}

void ashenvale::renderer::shader_compiler::reflect_shader_resources(
    ID3DBlob *shaderBlob, ashenvale::renderer::render_pass::resource_definition *definitions, int &count)
{
    count = 0;

    com_ptr<ID3D11ShaderReflection> reflection;
    if (FAILED(D3DReflect(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), IID_PPV_ARGS(&reflection))))
        return;

    D3D11_SHADER_DESC shaderDesc = {};
    reflection->GetDesc(&shaderDesc);

    for (UINT i = 0; i < shaderDesc.ConstantBuffers; ++i)
    {
        // Is this Com Object? it may be memory leak
        ID3D11ShaderReflectionConstantBuffer *cb = reflection->GetConstantBufferByIndex(i);
        D3D11_SHADER_BUFFER_DESC cbDesc;
        cb->GetDesc(&cbDesc);

        for (UINT b = 0; b < shaderDesc.BoundResources; ++b)
        {
            D3D11_SHADER_INPUT_BIND_DESC bindDesc;
            reflection->GetResourceBindingDesc(b, &bindDesc);

            if (strcmp(bindDesc.Name, cbDesc.Name) == 0 && bindDesc.Type == D3D_SIT_CBUFFER)
            {
                definitions[count++] = {ashenvale::renderer::render_pass::resource_definition::input_type::constant,
                                        bindDesc.BindPoint};
                break;
            }
        }
    }

    for (UINT i = 0; i < shaderDesc.BoundResources; ++i)
    {
        D3D11_SHADER_INPUT_BIND_DESC bindDesc;
        reflection->GetResourceBindingDesc(i, &bindDesc);

        ashenvale::renderer::render_pass::resource_definition::input_type type;
        switch (bindDesc.Type)
        {
        case D3D_SIT_TEXTURE:
        case D3D_SIT_STRUCTURED:
        case D3D_SIT_BYTEADDRESS:
        case D3D_SIT_TBUFFER:
            type = ashenvale::renderer::render_pass::resource_definition::input_type::srv;
            break;
        case D3D_SIT_SAMPLER:
            type = ashenvale::renderer::render_pass::resource_definition::input_type::sampler;
            break;
        default:
            continue;
        }

        definitions[count++] = {type, bindDesc.BindPoint};
    }
}
