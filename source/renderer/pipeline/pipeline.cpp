#include "pipeline.h"
#include "renderer/core/swapchain.h"
#include "shader.h"
#include <renderer/renderer.h>

using namespace winrt;

void ash::rhi_pl_init()
{
    SCOPED_CPU_EVENT(L"ash::rhi_pl_init");

    // Triangle

    rhi_g_device->CreateRootSignature(0, rhi_sh_g_triangle_vs.root_blob->GetBufferPointer(),
                                      rhi_sh_g_triangle_vs.root_blob->GetBufferSize(),
                                      IID_PPV_ARGS(rhi_pl_g_triangle.root_signature.put()));

    SET_OBJECT_NAME(rhi_pl_g_triangle.root_signature.get(), L"Triangle Root");

    D3D12_RASTERIZER_DESC raster_desc = {};
    raster_desc.FillMode = D3D12_FILL_MODE_SOLID;
    raster_desc.CullMode = D3D12_CULL_MODE_NONE;
    raster_desc.FrontCounterClockwise = false;

    D3D12_RENDER_TARGET_BLEND_DESC defaultBlend = {};
    defaultBlend.BlendEnable = FALSE;
    defaultBlend.LogicOpEnable = FALSE;
    defaultBlend.SrcBlend = D3D12_BLEND_ONE;
    defaultBlend.DestBlend = D3D12_BLEND_ZERO;
    defaultBlend.BlendOp = D3D12_BLEND_OP_ADD;
    defaultBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
    defaultBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
    defaultBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    defaultBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
    defaultBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_BLEND_DESC blend_desc = {};
    blend_desc.AlphaToCoverageEnable = FALSE;
    blend_desc.IndependentBlendEnable = FALSE;
    blend_desc.RenderTarget[0] = defaultBlend;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rhi_pl_g_triangle.root_signature.get();
    psoDesc.VS = {rhi_sh_g_triangle_vs.blob->GetBufferPointer(), rhi_sh_g_triangle_vs.blob->GetBufferSize()};
    psoDesc.PS = {rhi_sh_g_triangle_ps.blob->GetBufferPointer(), rhi_sh_g_triangle_ps.blob->GetBufferSize()};

    D3D12_DEPTH_STENCIL_DESC ds = {};
    ds.DepthEnable = TRUE;
    ds.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    ds.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.RasterizerState = raster_desc;
    psoDesc.BlendState = blend_desc;
    psoDesc.DepthStencilState = ds;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    rhi_g_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(rhi_pl_g_triangle.pso.put()));
    SET_OBJECT_NAME(rhi_pl_g_triangle.pso.get(), L"Triangle PSO");

    // Instanced Triangle

    rhi_g_device->CreateRootSignature(0, rhi_sh_g_triangle_instanced_vs.root_blob->GetBufferPointer(),
                                      rhi_sh_g_triangle_instanced_vs.root_blob->GetBufferSize(),
                                      IID_PPV_ARGS(rhi_pl_g_triangle_instanced.root_signature.put()));

    SET_OBJECT_NAME(rhi_pl_g_triangle_instanced.root_signature.get(), L"Triangle Instanced Root");

    raster_desc = {};
    raster_desc.FillMode = D3D12_FILL_MODE_SOLID;
    raster_desc.CullMode = D3D12_CULL_MODE_NONE;
    raster_desc.FrontCounterClockwise = false;

    defaultBlend = {};
    defaultBlend.BlendEnable = FALSE;
    defaultBlend.LogicOpEnable = FALSE;
    defaultBlend.SrcBlend = D3D12_BLEND_ONE;
    defaultBlend.DestBlend = D3D12_BLEND_ZERO;
    defaultBlend.BlendOp = D3D12_BLEND_OP_ADD;
    defaultBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
    defaultBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
    defaultBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    defaultBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
    defaultBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    blend_desc = {};
    blend_desc.AlphaToCoverageEnable = FALSE;
    blend_desc.IndependentBlendEnable = FALSE;
    blend_desc.RenderTarget[0] = defaultBlend;

    psoDesc = {};
    psoDesc.pRootSignature = rhi_pl_g_triangle_instanced.root_signature.get();
    psoDesc.VS = {rhi_sh_g_triangle_instanced_vs.blob->GetBufferPointer(),
                  rhi_sh_g_triangle_instanced_vs.blob->GetBufferSize()};
    psoDesc.PS = {rhi_sh_g_triangle_instanced_ps.blob->GetBufferPointer(),
                  rhi_sh_g_triangle_instanced_ps.blob->GetBufferSize()};

    ds = {};
    ds.DepthEnable = TRUE;
    ds.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    ds.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.RasterizerState = raster_desc;
    psoDesc.BlendState = blend_desc;
    psoDesc.DepthStencilState = ds;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    rhi_g_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(rhi_pl_g_triangle_instanced.pso.put()));
    SET_OBJECT_NAME(rhi_pl_g_triangle_instanced.pso.get(), L"Triangle Instanced PSO");

    // Geometry

    rhi_g_device->CreateRootSignature(0, rhi_sh_g_geometry_vs.root_blob->GetBufferPointer(),
                                      rhi_sh_g_geometry_vs.root_blob->GetBufferSize(),
                                      IID_PPV_ARGS(rhi_pl_g_geometry.root_signature.put()));

    SET_OBJECT_NAME(rhi_pl_g_geometry.root_signature.get(), L"Geometry Root");

    raster_desc = {};
    raster_desc.FillMode = D3D12_FILL_MODE_SOLID;
    raster_desc.CullMode = D3D12_CULL_MODE_NONE;
    raster_desc.FrontCounterClockwise = false;

    defaultBlend = {};
    defaultBlend.BlendEnable = FALSE;
    defaultBlend.LogicOpEnable = FALSE;
    defaultBlend.SrcBlend = D3D12_BLEND_ONE;
    defaultBlend.DestBlend = D3D12_BLEND_ZERO;
    defaultBlend.BlendOp = D3D12_BLEND_OP_ADD;
    defaultBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
    defaultBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
    defaultBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    defaultBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
    defaultBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    blend_desc = {};
    blend_desc.AlphaToCoverageEnable = FALSE;
    blend_desc.IndependentBlendEnable = FALSE;
    blend_desc.RenderTarget[0] = defaultBlend;

    psoDesc = {};
    psoDesc.pRootSignature = rhi_pl_g_geometry.root_signature.get();
    psoDesc.VS = {rhi_sh_g_geometry_vs.blob->GetBufferPointer(), rhi_sh_g_geometry_vs.blob->GetBufferSize()};
    psoDesc.PS = {rhi_sh_g_geometry_ps.blob->GetBufferPointer(), rhi_sh_g_geometry_ps.blob->GetBufferSize()};

    ds = {};
    ds.DepthEnable = TRUE;
    ds.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    ds.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    D3D12_INPUT_LAYOUT_DESC geometry_input_layout_desc = {};
    geometry_input_layout_desc.NumElements = rhi_sh_g_geometry_vs.input_layout.size();
    geometry_input_layout_desc.pInputElementDescs = rhi_sh_g_geometry_vs.input_layout.data();

    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.RasterizerState = raster_desc;
    psoDesc.BlendState = blend_desc;
    psoDesc.DepthStencilState = ds;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.InputLayout = geometry_input_layout_desc;

    rhi_g_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(rhi_pl_g_geometry.pso.put()));
    SET_OBJECT_NAME(rhi_pl_g_geometry.pso.get(), L"Geometry PSO");
}
