Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
SamplerState defaultSampler : register(s0);

struct Light
{
    float3 position;
    float pad1;

    float3 color;
    float intensity;

    uint light_type;
    float linearr;
    float quadratic;
    float range;

    float spot_inner_cone_angle;
    float spot_outer_cone_angle;
    float2 pad2;
};

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

StructuredBuffer<Light> lights : register(t2);

cbuffer LightMeta : register(b2)
{
    uint lightCount;
    float3 _padding;
}

float3 CalcDirLight(Light light, PixelInputType input)
{
    float3 lightDir = normalize(light.position - input.worldPos);    
    float3 viewDir = normalize(cameraPosition - input.worldPos);
    float3 reflectDir = reflect(-lightDir, input.normal);
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
    float3 specular = spec * light.color * light.intensity;

    float diff = max(dot(input.normal, lightDir), 0.0);
    float3 diffuse = diff * light.color * light.intensity;
 
    return (diffuse + specular);
}

float3 CalcPointLight(Light light, PixelInputType input)
{
    float distance = length(light.position - input.worldPos);
    float attenuation = 1.0f / (1.0f + light.linearr * distance + light.quadratic * distance * distance);
    
    float3 viewDir = normalize(cameraPosition - input.worldPos);
    float3 lightDir = normalize(light.position - input.worldPos);
    float3 reflectDir = reflect(-lightDir, input.normal);
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
    float3 specular = spec * light.color * light.intensity;

    float diff = max(dot(input.normal, lightDir), 0.0);
    float3 diffuse = diff * light.color * light.intensity;
    
    return (diffuse + specular) * attenuation;
}

float3 CalcSpotLight(Light light, PixelInputType input)
{
    return float3(1.0f, 1.0f, 1.0f);
}

float4 main(PixelInputType input) : SV_TARGET
{
    float3 lightResult;
    
    for (uint i = 0; i < lightCount; i++)
    {
        switch (lights[i].light_type)
        {
            case 0:
                lightResult += CalcDirLight(lights[i], input);
                break;
            case 1:
                lightResult += CalcPointLight(lights[i], input);
                break;
            //case 2:
            //    lightResult += CalcSpotLight(lights[i], input);
        }
    }
    

    float3 ambient = 0.1f * baseColorFactor.rgb;

    float3 textureColor = albedoTexture.Sample(defaultSampler, input.tex).rgb;

    float3 lighting = lightResult + ambient;
    float3 finalColor = textureColor * lighting * baseColorFactor.rgb;

    return float4(finalColor, baseColorFactor.a);

}