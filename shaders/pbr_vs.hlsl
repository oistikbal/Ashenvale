cbuffer CameraBuffer : register(b0)
{
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float3 cameraPosition;
    float padding;
};

struct VertexInputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
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

PixelInputType main(VertexInputType input)
{
    PixelInputType output;
    
    float4 worldPos = mul(float4(input.position, 1.0f), modelMatrix);
    output.worldPos = worldPos.xyz;
    
    output.position = mul(worldPos, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    output.tex = input.tex;
    output.normal = normalize(mul(input.normal, (float3x3) modelMatrix));
    
    output.tangent = normalize(mul(input.tangent, (float3x3) modelMatrix));

    float3 bitangent = cross(input.normal, input.tangent);
    
    output.bitangent = normalize(mul(bitangent, (float3x3) modelMatrix));

    return output;
}