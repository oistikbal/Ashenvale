#pragma once

#include "render_pass.h"
#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <vector>
#include <winrt/base.h>

namespace ashenvale::renderer::shader_compiler
{
HRESULT compile(const wchar_t *file, const char *entryPoint, const char *target, const D3D_SHADER_MACRO *defines,
                ID3DBlob **shaderBlob, ID3DBlob **errorBlob);
std::vector<D3D11_INPUT_ELEMENT_DESC> get_input_layout(ID3D11ShaderReflection *reflection);
void reflect_shader_resources(ID3DBlob *shaderBlob, render_pass::resource_definition *definitions, int &count);
} // namespace ashenvale::renderer::shader_compiler