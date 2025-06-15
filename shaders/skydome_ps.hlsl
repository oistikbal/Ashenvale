Texture2D hdrSky : register(t0);
SamplerState samp : register(s0);

struct PixelInputType
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
};

float4 main(PixelInputType input) : SV_TARGET
{
    float3 dir = normalize(input.worldPos);

    float2 uv;
    uv.x = atan2(dir.z, dir.x) / (2.0 * 3.14159265f) + 0.5;
    uv.y = asin(clamp(dir.y, -1.0, 1.0)) / 3.14159265f + 0.5;

    return hdrSky.Sample(samp, uv);
}