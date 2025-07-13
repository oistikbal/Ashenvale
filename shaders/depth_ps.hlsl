Texture2D<float> g_depthTexture : register(t0);
SamplerState g_sampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

float LinearizeDepth(float depth)
{
    return (0.1f * 100.0f) / (100.0f - depth * (100.0f - 0.1));
}

float4 main(PSInput input) : SV_Target
{
    float depth = g_depthTexture.Sample(g_sampler, input.uv);
    float linearDepth = LinearizeDepth(depth);

    float normalized = (linearDepth - 0.1f) / (100.0f - 0.1f);
    
    return float4(normalized, 0.0f, 0.0f, 1.0f);
}