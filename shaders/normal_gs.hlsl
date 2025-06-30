cbuffer CameraBuffer : register(b0)
{
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float3 cameraPosition;
    float padding;
};

struct GeometryInputType
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : NORMAL;
};


[maxvertexcount(2)]
void main(point GeometryInputType input[1], inout LineStream<GeometryInputType> stream)
{
    float normalLength = 0.1;

    GeometryInputType v0 = input[0];
    GeometryInputType v1 = input[0];
    
    v0.position = mul(float4(v0.worldPos, 1.0), modelMatrix);
    v0.position = mul(v0.position, viewMatrix);
    v0.position = mul(v0.position, projectionMatrix);
    
    v1.worldPos += normalize(v1.normal) * normalLength;
    v1.position = mul(float4(v1.worldPos, 1.0), modelMatrix);
    v1.position = mul(v1.position, viewMatrix);
    v1.position = mul(v1.position, projectionMatrix);
    


    stream.Append(v0);
    stream.Append(v1);
}