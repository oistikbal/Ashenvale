Texture2D<float> g_depthTexture : register(t0);
SamplerState g_sampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(PSInput input) : SV_Target
{
    return g_depthTexture.Sample(g_sampler, input.uv);
}