struct GeometryInputType
{
    float4 position : SV_POSITION; // For rasterizer (line end position)
    float3 worldPos : TEXCOORD0; // For math in GS
    float3 normal : NORMAL; // Also for math
};

float4 main(GeometryInputType input) : SV_Target
{
    return float4(1, 1, 0, 1);
}