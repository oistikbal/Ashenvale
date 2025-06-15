Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);
SamplerState defaultSampler : register(s0);

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

cbuffer MaterialConstants : register(b0)
{
    float4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
};

float4 main(PixelInputType input) : SV_TARGET
{
    float4 textureColor;
    float4 textureColor2;

    textureColor = diffuseTexture.Sample(defaultSampler, input.tex);
    textureColor2 = normalTexture.Sample(defaultSampler, input.tex);

    return textureColor * baseColorFactor;
}