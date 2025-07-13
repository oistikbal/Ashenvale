#pragma once
#include <d3d11_4.h>
#include <winrt/base.h>

#include "renderer/render_pass.h"

namespace ashenvale::renderer::render_pass::tonemap
{
struct exposure_buffer
{
    float exposure;
    float padding[3];
};
inline winrt::com_ptr<ID3D11Buffer> g_exposureBuffer;

void initialize();
void execute(const render_pass_context &context);
} // namespace ashenvale::renderer::render_pass::tonemap