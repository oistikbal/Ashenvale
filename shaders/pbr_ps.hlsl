Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);
SamplerState defaultSampler : register(s0);

struct PixelInputType
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD0;
};

cbuffer MaterialConstants : register(b0)
{
    float4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
};

static const float3 lightPos = float3(-3.0f, 0.0f, 0.0f);
static const float3 ambientLight = float3(1.0f, 1.0f, 1.0f) * 0.2f;

float4 main(PixelInputType input) : SV_TARGET
{
    float3 norm = input.normal;
    float3 lightDir = normalize(lightPos - input.position.xyz);
    
    float diff = max(dot(norm, lightDir), 0.0);
    float3 diffuse = diff * float3(1.0f,1.0f,1.0f);
    
    float3 textureColor = diffuseTexture.Sample(defaultSampler, input.tex).rgb;
    float3 finalColor = textureColor * (diffuse + ambientLight) * baseColorFactor.rgb;
    
    return float4(finalColor, baseColorFactor.a);
}