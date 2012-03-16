
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
    float4 position : POSITION;
    float2 uv : TEXCOORD;
};

vs_output main(in vs_input input) {
    vs_output o;
    o.uv = input.uv;
    float4 position = float4(0, 0, 0, 1);
    for (uint i = 0; i < input.numWeights; ++i)
    {
        float4 weightedPosition = gWeightedPositions.Load(input.firstWeight + i);
        uint jointIndex = gJointIndices.Load(input.firstWeight + i);
        float4x4 mat = gMatrices[jointIndex];
        float weight = weightedPosition.w;
        position += weight * mul(float4(weightedPosition.xyz, 1), mat);
    }
    o.position = mul(position, gViewProjection);
    return o;
}


/*
int4 numPositions;
sampler gPositionTexture : register(s0);

sampler gJointTexture : register(s1);

float4x4 gViewProjection;
float4x3 gSkinningMatrices[83];

struct vs_input {
    float2 uv : TEXCOORD0;
    uint2 weights : TEXCOORD1;
};

struct vs_output {
    float4 position : POSITION;
    int2 uv : TEXCOORD0;
};

vs_output main(in vs_input input) {
    vs_output o;
    o.uv = input.uv;
    int firstWeight = input.weights.x;
    int numWeights = input.weights.y;

    float3 position = float3(0, 0, 0);
    float invPositions = 1.0f / float(numPositions.x);

    for (int i = 0; i < numWeights; ++i) {
        float s = invPositions * float(i + firstWeight);
        float4 weightedPosition = tex1Dlod(gPositionTexture, float4(s, 0, 0, 0));
        int jointIdx = (int)(tex1Dlod(gJointTexture, float4(s, 0, 0, 0)) * 255.0f);

        float3 inputPosition = weightedPosition.xyz;
        float weight = weightedPosition.w;
        position += mul(float4(inputPosition, 1), gSkinningMatrices[jointIdx]) * weight;
    }

    o.position = mul(float4(position, 1), gViewProjection);
    return o;
}

*/

