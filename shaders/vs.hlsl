// Vertex Shader: triangle_vs.hlsl

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput main(VSInput input)
{
    PSInput output;
    output.position = float4(input.position, 1.0); // Transform to homogeneous clip space
    output.color = input.color;
    return output;
}
