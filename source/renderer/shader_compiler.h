#pragma once

#include <wrl/client.h>
#include <d3dcompiler.h>

using Microsoft::WRL::ComPtr;

namespace ashenvale::renderer::shader_compiler
{
	HRESULT compile(const wchar_t* file, const char* entryPoint, const char* target, const D3D_SHADER_MACRO* defines, ID3DBlob** shaderBlob, ID3DBlob** errorBlob);
}