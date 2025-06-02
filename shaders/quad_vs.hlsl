struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;
    // Generate UVs: (0,0), (0,1), (1,0), (1,1)
    output.uv = float2((vertexID & 2) >> 1, vertexID & 1);
    // Generate clip-space position: map UVs to (-1,1) to (1,-1)
    output.pos = float4(output.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}