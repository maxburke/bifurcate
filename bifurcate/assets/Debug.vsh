
cbuffer cb0 : register(cb0)
{
    float4x4 gViewProjection;
};

struct vs_in
{
    float3 pos : POSITION;
};

struct vs_output
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

vs_output main(in vs_in input) {
    vs_output o;
    o.pos = mul(float4(input.pos, 1), transpose(gViewProjection));
    o.uv = float2(0, 0);
    return o;
}
