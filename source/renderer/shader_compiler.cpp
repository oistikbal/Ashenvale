#include "renderer/shader_compiler.h"
#include <string>
#include <comdef.h>

#include "configs/shader.h"


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