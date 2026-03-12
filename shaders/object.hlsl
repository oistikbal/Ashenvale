#define RS "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | SAMPLER_HEAP_DIRECTLY_INDEXED | CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED ), " \
            "RootConstants(num32BitConstants=16, b0)"

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

struct SceneBuffer
{
    float4x4 mvp;
};

ConstantBuffer<SceneBuffer> sb : register(b0);

[RootSignature(RS)]
VSOutput vs_main(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1.0f), sb.mvp);
    output.uv = input.uv;
    return output;
}

float4 ps_main(VSOutput input) : SV_Target
{
    Texture2D<float4> tex = ResourceDescriptorHeap[0];
    SamplerState samp = SamplerDescriptorHeap[0];
    return tex.Sample(samp, input.uv);
}
