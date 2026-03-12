#pragma once

#include "common.h"
#include "renderer/pipeline/shader.h"

namespace ash
{
struct rhi_pl_pipeline_state
{
    winrt::com_ptr<ID3D12PipelineState> pso;
    winrt::com_ptr<ID3D12RootSignature> root_signature;
};
inline rhi_pl_pipeline_state rhi_pl_g_object;
} // namespace ash

namespace ash
{
void rhi_pl_init();
}