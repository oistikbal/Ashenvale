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
    float3 tangent : TANGENT;
    float3 bitangent : BINORMAL;
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

cbuffer ExposureBuffer : register(b3)
{
    float exposure;
    float3 _padding2;
}

//--------------------------------------------------------------------------------------
// Timothy Lottes tone mapper
//--------------------------------------------------------------------------------------
// General tonemapping operator, build 'b' term.
float ColToneB(float hdrMax, float contrast, float shoulder, float midIn, float midOut)
{
    return
        -((-pow(midIn, contrast) + (midOut * (pow(hdrMax, contrast * shoulder) * pow(midIn, contrast) -
            pow(hdrMax, contrast) * pow(midIn, contrast * shoulder) * midOut)) /
            (pow(hdrMax, contrast * shoulder) * midOut - pow(midIn, contrast * shoulder) * midOut)) /
            (pow(midIn, contrast * shoulder) * midOut));
}

// General tonemapping operator, build 'c' term.
float ColToneC(float hdrMax, float contrast, float shoulder, float midIn, float midOut)
{
    return (pow(hdrMax, contrast * shoulder) * pow(midIn, contrast) - pow(hdrMax, contrast) * pow(midIn, contrast * shoulder) * midOut) /
           (pow(hdrMax, contrast * shoulder) * midOut - pow(midIn, contrast * shoulder) * midOut);
}

// General tonemapping operator, p := {contrast,shoulder,b,c}.
float ColTone(float x, float4 p)
{
    float z = pow(x, p.r);
    return z / (pow(z, p.g) * p.b + p.a);
}
float3 TimothyTonemapper(float3 color)
{
    static float hdrMax = 16.0; // How much HDR range before clipping. HDR modes likely need this pushed up to say 25.0.
    static float contrast = 1.0; // Use as a baseline to tune the amount of contrast the tonemapper has.
    static float shoulder = 1.0; // Likely don’t need to mess with this factor, unless matching existing tonemapper is not working well..
    static float midIn = 0.18; // most games will have a {0.0 to 1.0} range for LDR so midIn should be 0.18.
    static float midOut = 0.18; // Use for LDR. For HDR10 10:10:10:2 use maybe 0.18/25.0 to start. For scRGB, I forget what a good starting point is, need to re-calculate.

    float b = ColToneB(hdrMax, contrast, shoulder, midIn, midOut);
    float c = ColToneC(hdrMax, contrast, shoulder, midIn, midOut);

#define EPS 1e-6f
    float peak = max(color.r, max(color.g, color.b));
    peak = max(EPS, peak);

    float3 ratio = color / peak;
    peak = ColTone(peak, float4(contrast, shoulder, b, c));
    // then process ratio

    // probably want send these pre-computed (so send over saturation/crossSaturation as a constant)
    float crosstalk = 4.0; // controls amount of channel crosstalk
    float saturation = contrast; // full tonal range saturation control
    float crossSaturation = contrast * 16.0; // crosstalk saturation

    float white = 1.0;

    // wrap crosstalk in transform
    ratio = pow(abs(ratio), saturation / crossSaturation);
    ratio = lerp(ratio, white, pow(peak, crosstalk));
    ratio = pow(abs(ratio), crossSaturation);

    // then apply ratio to peak
    color = peak * ratio;
    return color;
}

float3 CalcDirLight(Light light, PixelInputType input)
{
    float3 normalColor = normalTexture.Sample(defaultSampler, input.tex).xyz;
    normalColor = (normalColor * 2.0 - 1.0);
    normalColor = (normalColor.x * input.tangent) + (normalColor.y * input.bitangent) + (normalColor.z * input.normal);
    normalColor = normalize(normalColor);

    float3 lightDir = normalize(light.position - input.worldPos);    
    float3 viewDir = normalize(cameraPosition - input.worldPos);
    float3 reflectDir = reflect(-lightDir, normalColor);
    float3 halfwayDir = normalize(lightDir + viewDir);

    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), 64);
    float3 specular = spec * light.color * light.intensity;
    
    float diff = max(dot(normalColor, lightDir), 0.0);
    float3 diffuse = diff * light.color * light.intensity;
 
    return (diffuse + specular);
}

float3 CalcPointLight(Light light, PixelInputType input)
{
    float3 normalColor = normalTexture.Sample(defaultSampler, input.tex).xyz;
    normalColor = (normalColor * 2.0 - 1.0);
    normalColor = (normalColor.x * input.tangent) + (normalColor.y * input.bitangent) + (normalColor.z * input.normal);
    normalColor = normalize(normalColor);
   
    float distance = length(light.position - input.worldPos);
    float attenuation = 1.0f / (1.0f + light.linearr * distance + light.quadratic * distance * distance);
    
    float3 viewDir = normalize(cameraPosition - input.worldPos);
    float3 lightDir = normalize(light.position - input.worldPos);
    float3 reflectDir = reflect(-lightDir, normalColor);
    float3 halfwayDir = normalize(lightDir + viewDir);

    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), 64);
    float3 specular = spec * light.color * light.intensity;

    float diff = max(dot(normalColor, lightDir), 0.0);
    float3 diffuse = diff * light.color * light.intensity;
    
    return (diffuse + specular) * attenuation;
}

float3 CalcSpotLight(Light light, PixelInputType input)
{
    return float3(1.0f, 1.0f, 1.0f);
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

    float3 textureColor = SRGBToLinear(albedoTexture.Sample(defaultSampler, input.tex).rgb);

    float3 lighting = lightResult;
    float3 finalColor = textureColor * lighting * baseColorFactor.rgb;
    
    float3 mapped = TimothyTonemapper(finalColor) * exposure;

    return float4(SRGBToLinear(mapped), baseColorFactor.a);

}