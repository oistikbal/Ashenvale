Texture2D g_viewportTexture : register(t0);
SamplerState g_sampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer ExposureBuffer : register(b0)
{
    float exposure;
    float3 _padding;
}


float3 SRGBToLinear(float3 color)
{
    return color <= 0.04045 ?
        color / 12.92 :
        pow((color + 0.055) / 1.055, 2.4);
}

float3 LinearToSRGB(float3 color)
{
    return color <= 0.0031308 ?
        color * 12.92 :
        1.055 * pow(color, 1.0 / 2.4) - 0.055;
}

float4 main(PSInput input) : SV_Target
{
    return g_viewportTexture.Sample(g_sampler, input.uv) * exposure;
}