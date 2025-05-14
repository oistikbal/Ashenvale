#include "renderer/shader_compiler.h"
#include "configs/shader.h"

#include <string>


HRESULT ashenvale::renderer::shader_compiler::compile(const wchar_t* file, const char* entryPoint, const char* target,
    const D3D_SHADER_MACRO* defines, ID3DBlob** shaderBlob, ID3DBlob** errorBlob) {

    std::wstring fullPath = std::wstring(config::SHADER_PATH) + file;

    HRESULT hr = D3DCompileFromFile(
        fullPath.c_str(),
        defines,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint,
        target,
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        shaderBlob,
        errorBlob
    );

    return hr;
}

std::vector<D3D11_INPUT_ELEMENT_DESC> ashenvale::renderer::shader_compiler::get_input_layout(ID3D11ShaderReflection* reflection)
{
    D3D11_SHADER_DESC shaderDesc {};
    reflection->GetDesc(&shaderDesc);
    std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayout(shaderDesc.InputParameters);

    for (unsigned i = 0; i < shaderDesc.InputParameters; ++i)
    {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc {};
        reflection->GetInputParameterDesc(i, &paramDesc);

        D3D11_INPUT_ELEMENT_DESC elementDesc = {};

        elementDesc.SemanticName = paramDesc.SemanticName;
        elementDesc.SemanticIndex = paramDesc.SemanticIndex;

        switch (paramDesc.ComponentType)
        {
        case D3D_REGISTER_COMPONENT_FLOAT32:
            if (paramDesc.Mask == 1) {
                elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
            }
            else if (paramDesc.Mask == 3) {
                elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
            }
            else if (paramDesc.Mask == 7) {
                elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
            }
            else if (paramDesc.Mask == 15) {
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
