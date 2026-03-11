#include "shader.h"
#include "shader_compiler.h"

using namespace winrt;

namespace
{
void init_shader(ash::rhi_sh_shader &shader, const wchar_t *file, const wchar_t *entryPoint, const wchar_t *target)
{
    com_ptr<IDxcBlobUtf8> error_blob;
    com_ptr<IDxcResult> result;

    com_ptr<IDxcBlob> reflection_blob;

    ash::rhi_sc_compile(file, entryPoint, target, result.put(), error_blob.put());

    result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(shader.blob.put()), nullptr);
    result->GetOutput(DXC_OUT_ROOT_SIGNATURE, IID_PPV_ARGS(shader.root_blob.put()), nullptr);
    result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(reflection_blob.put()), nullptr);

    DxcBuffer reflectionBuffer{
        .Ptr = reflection_blob->GetBufferPointer(),
        .Size = reflection_blob->GetBufferSize(),
        .Encoding = 0,
    };

    ash::rhi_sc_g_utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(shader.reflector.put()));

    shader.input_layout = ash::rhi_sc_get_input_layout(shader.reflector.get());
    shader.bindings = ash::rhi_sc_get_bindings(shader.reflector.get());
}
} // namespace

void ash::rhi_sh_init()
{
    init_shader(rhi_sh_g_triangle_vs, L"triangle.hlsl", L"vs_main", L"vs_6_6");
    init_shader(rhi_sh_g_triangle_ps, L"triangle.hlsl", L"ps_main", L"ps_6_6");

    init_shader(rhi_sh_g_triangle_instanced_vs, L"triangle_instanced.hlsl", L"vs_main", L"vs_6_6");
    init_shader(rhi_sh_g_triangle_instanced_ps, L"triangle_instanced.hlsl", L"ps_main", L"ps_6_6");

    init_shader(rhi_sh_g_geometry_vs, L"geometry.hlsl", L"vs_main", L"vs_6_6");
    init_shader(rhi_sh_g_geometry_ps, L"geometry.hlsl", L"ps_main", L"ps_6_6");
}
