Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
SamplerState defaultSampler : register(s0);


struct PixelInputType
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD1;
};

cbuffer CameraBuffer : register(b0)
{
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float3 cameraPosition;
    float padding;
};

cbuffer MaterialConstants : register(b1)
{
    float4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
};


float4 main(PixelInputType input) : SV_TARGET
{
    float3 ambientLight = float3(1.0f, 1.0f, 1.0f) * 0.1f;
    float3 lightPos = float3(0.0f, 5.0f, 0.0f);
    float3 lightColor = float3(1.0f, 1.0f, 1.0f);
    float lightIntensity = 1.0f;

    float distance = length(lightPos - input.worldPos);
    float attenuation = 1.0f / (1.0f + 0.02f * distance + 0.001f * distance * distance);

    float3 norm = input.normal;
    float3 viewDir = normalize(cameraPosition - input.worldPos);
    float3 lightDir = normalize(lightPos - input.worldPos);
    float3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
    float3 specular = spec * lightColor * lightIntensity;

    float diff = max(dot(norm, lightDir), 0.0);
    float3 diffuse = diff * lightColor * lightIntensity;

    float3 ambient = ambientLight * baseColorFactor.rgb;

    float3 textureColor = albedoTexture.Sample(defaultSampler, input.tex).rgb;

    float3 lighting = (diffuse + specular) * attenuation + ambient;
    float3 finalColor = textureColor * lighting * baseColorFactor.rgb;

    return float4(finalColor, baseColorFactor.a);

}