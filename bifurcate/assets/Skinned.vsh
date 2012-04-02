
cbuffer cb0 : register(cb0)
{
    float4x4 gViewProjection;
};

cbuffer cb1 : register(cb1)
{
    float4x4 gMatrices[256];
};

Buffer<float4> gWeightedPositions : register(t0);
Buffer<uint> gJointIndices : register(t1);

struct vs_input
{
    float2 uv : TEXCOORD0;
    uint firstWeight : TEXCOORD1;
    uint numWeights : TEXCOORD2;
};

struct vs_output
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

vs_output main(in vs_input input) {
    vs_output o;
    o.uv = input.uv;
    float4 position = float4(0, 0, 0, 1);
    for (uint i = input.firstWeight, e = input.firstWeight + input.numWeights; i < e; ++i)
    {
        float4 weightedPosition = gWeightedPositions.Load(i);
        uint jointIndex = gJointIndices.Load(i);
        float4x4 mat = gMatrices[jointIndex];
        float weight = weightedPosition.w;
        position += weight * mul(float4(weightedPosition.xyz, 1), mat);
    }

    o.position = mul(float4(position.xyz, 1), transpose(gViewProjection));
    return o;
}

